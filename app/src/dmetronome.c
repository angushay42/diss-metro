#include "dmetronome.h"
#include "duart.h"
#include "dsystime.h"
#include <libopencm3/stm32/f4/exti.h>

#define ADC_PORT    (GPIOA)
#define TEMPO_PIN   (GPIO0)
#define VOLUME_PIN  (GPIO1)

#define START_STOP_PORT (GPIOC)
#define START_STOP_PIN  (GPIO1)

/*********** prototypes *************/
static error_t dmetro_set_sequence(uint8_t new_sequence[]);
static uint16_t scale_to_bpm(uint16_t reading);
static error_t dmetro_get_tempo_reading(uint16_t *data, uint16_t cycle_timeout);
static void dmetro_toggle_tempo(void);



/***************************** global variables ***************************/

/************************ ADC variables ****************/
/* adc reads groups of channels, in this case we just need one for each reading. */
/* array of channels to read from ADC for the tempo measurement */
static uint8_t tempo_group[] = {ADC_CHANNEL0};
/* array of channels to read from the ADC for volume */
static uint8_t volume_group[] = {ADC_CHANNEL1};


/********************** Metronome configuration variables ***********************/

/* current Beats per Minute of the metronome */
static uint16_t _bpm;

/* actual frequency of the timer used for the metronome, in Hz. */
static double metro_tim_freq;

/* prescaler for timer, set to 2048 by setup */
static volatile uint32_t clk_psc = 0;

/* period in clock ticks of the main beat (crotchets in 4/4) */
static volatile uint32_t beat_period = 0;

/* period in clock ticks of the LED pulse */
static volatile uint32_t pulse_period = 0;

/* period in clock ticks of the half beats (quavers in 4/4) */
static volatile uint32_t half_period = 0;

/* time in ms for the LED to blink flash for */
static uint32_t pulse_period_ms = 100;

/* beat program storage */
static volatile uint8_t beat_sequence[8] = {0U};

/* next beat to be played */
static volatile size_t beat_idx = 0;


/************************* Metronome status variables ***************************/

/* time in ms of last button toggle */
static volatile uint64_t last_toggle = 0;

/* delay in ms to debounce the push button */
static volatile uint64_t debounce_delay = 200; 

/* time in ms of last time tempo was polled */
static volatile uint64_t last_poll = 0;

/* status of the metronome; false is off */
static volatile bool tempo_status = false;

/* flag marking the status of the LED */
static volatile bool led_status = false;


volatile uint64_t beat_stamp = 0;






/*************** ISR handlers ******************/
void exti1_isr(void) {
    /* flag_status is 1 if triggered by selected trigger, 0 otherwise */
    if (exti_get_flag_status(EXTI1)) {
        uint64_t now = get_time(false);
        
        /* check if this interrupt fired < delay seconds ago */
        if (now - last_toggle >= debounce_delay) {
            last_toggle = now;
            dmetro_toggle_tempo();
        }
    }
    // clear flag
    exti_reset_request(EXTI1);
}

/* set OC1 to trigger in 100ms from now to turn the LED off */
void queue_toggle(void) {
    uint32_t temp;
    /* this works for quavers, but won't for anything more complex. */
    temp = pulse_period + ((beat_idx & 1) ? half_period: 0);

    /* critical section, disable interrupts to toggle shared variable */
    nvic_disable_irq(NVIC_TIM4_IRQ);
    led_status = true;
    nvic_enable_irq(NVIC_TIM4_IRQ);

    /* update new output compare value */
    timer_set_oc_value(TIM4, TIM_OC1, temp);
}

/* @TODO rewrite so crotchet / quaver are in the same branch */
void tim4_isr(void) {
    /* overflow is every crotchet */
    if (timer_get_flag(TIM4, TIM_SR_UIF)) {
        /* if user has selected this beat to run, do so */
        if (beat_sequence[beat_idx] == 1) {

            /* turn LED on */
            gpio_set(GPIOB, GPIO6);

            /* critical section, udpate shared variable */
            nvic_disable_irq(NVIC_TIM4_IRQ);
            beat_stamp = get_time(true);
            nvic_enable_irq(NVIC_TIM4_IRQ);

            /* queue the timer to turn the LED back off */
            queue_toggle();
        }
        beat_idx = (beat_idx + 1) % 8;
        /* clear flag */
        timer_clear_flag(TIM4, TIM_SR_UIF);
    }
    /* Output Compare 1 is the trigger to turn the LED off after pulse_period time has passed */
    else if (timer_get_flag(TIM4, TIM_SR_CC1IF)) {
        
        /* if LED is currently on, turn it off*/
        if (led_status) {
            /* turn LED off */
            gpio_clear(GPIOB, GPIO6);

            /* critical section */
            nvic_disable_irq(NVIC_TIM4_IRQ);
            led_status = false;
            nvic_enable_irq(NVIC_TIM4_IRQ);
        }
        timer_clear_flag(TIM4, TIM_SR_CC1IF);
    }
    /* output compare 2 is every quaver */
    else if (timer_get_flag(TIM4, TIM_SR_CC2IF)) {
        /* if beat is set to play, turn LED on*/
        if (beat_sequence[beat_idx] == 1) {    

            /* turn LED on */
            gpio_set(GPIOB, GPIO6);

            /* critical section, udpate shared variable */
            nvic_disable_irq(NVIC_TIM4_IRQ);
            beat_stamp = get_time(true);
            nvic_enable_irq(NVIC_TIM4_IRQ);

            /* queue the timer to turn the LED back off */
            queue_toggle();
        }
        beat_idx = (beat_idx + 1) % 8;
        /* clear flag */
        timer_clear_flag(TIM4, TIM_SR_CC2IF);
    }
}

/**
 * Set the metronome beat program to the new desired program.
 * @param new_sequence a list of either 0 or 1
 * Set a character in sequence to 1 if you wish that note to be played.
 *
 * For example, if you want only the main crotchet beats played:
 * sequence = {1, 0, 1, 0, 1, 0, 1, 0}
 */
static error_t dmetro_set_sequence(uint8_t new_sequence[]) {
    /* fill the beat program with new program */
    for (size_t i = 0; i < 8; i++)
        beat_sequence[i] = new_sequence[i];
    return OK;
}

//todo should this reject invalid readings?
static uint16_t scale_to_bpm(uint16_t reading) {
    float temp;  
    uint16_t res;
    
    temp = (float) reading;

    // https://stats.stackexchange.com/questions/281162/scale-a-number-between-a-range
    // m = ((m - r_min) / (r_max - r_min)) * (t_max - t_min) + t_min
    temp = temp / (float) (1 << 12);
    temp = temp * (float) (MAX_BPM - MIN_BPM);
    temp = temp + (float) MIN_BPM;

    // original scale is 0-4096 (1<<12)
    // temp = ((temp - 0) / (float) (1 << 12)) * (MAX_BPM - MIN_BPM) + MIN_BPM;
    // scale reading into total range

    res = (uint16_t) temp;
    // clamp reading
    if (res > MAX_BPM)
        res = MAX_BPM;
    else if (res < MIN_BPM)
        res = MIN_BPM;
    return res;
}

/* read tempo measurement from ADC. Returns 0 if successful */
static error_t dmetro_get_tempo_reading(uint16_t *data, uint16_t cycle_timeout) {
    uint32_t i;
    adc_set_regular_sequence(ADC1, 1, tempo_group);
    adc_start_conversion_regular(ADC1);
    for (i = 0; !(adc_eoc(ADC1)); i++)
        if (cycle_timeout > 0 && i > cycle_timeout) {
            *data = 1 << 14;
            return DADC_TIMEOUT;
        }

    *data = scale_to_bpm(adc_read_regular(ADC1));

    return OK;
}

/* packet used for sending tempo over UART */
struct packet tempo_pack = {
    .id = "TEMPO",
    .is_signed =false,
    .size = 4,
    .len = 1,
};

/* poll ADC every poll_period (ms) and update tempo if mismatch. */
extern error_t dmetro_poll_update(uint64_t poll_period) {
    error_t err;
    uint64_t now;
    uint16_t reading;

    /* get current time */
    now = get_time(false);
        
    // if time since last poll is exceeded, poll again
    if (now - last_poll >= poll_period) {
        last_poll = now;
        /* get reading from ADC with timeout of 40 cycles */
        if ((err = dmetro_get_tempo_reading(&reading, 40))) 
            return error_handle(err);
        
            /* update tempo if necessary */
        if (reading != dmetro_get_tempo()) {
            dmetro_set_tempo(reading);
            // duart_send_packet(&tempo_pack);
        }
    }
    return OK;
}

/** 
 * Get current tempo of metronome, in BPM
 */
extern uint16_t dmetro_get_tempo(void) {
    return _bpm;
}

/**
 * Set tempo of the metronome.
 * @param bpm the desired beats per minute of the metronome to be set
 * 
 * @exception Invalid tempo; if bpm is not in the range [40, 220]
 * @exception Invalid prescaler; if the desired prescaler is not in the range [0, 65535]
 * 
 */
extern error_t dmetro_set_tempo(uint16_t bpm) {
     if (!(bpm >= MIN_BPM && bpm <= MAX_BPM))
        return DMETRO_INVALID_TEMPO;
    /* bpm in Hz */
    double fbpm;

    /* new period of metronome, in clock ticks */
    uint32_t new_period; 

    /* temp variable for storing counter*/
    uint32_t counter;

    /* convert bpm to frequency in Hz */
    fbpm = (double) bpm / 60.0;

    /* input / psc = output; so, input / output = psc */
    new_period = (uint32_t) round(metro_tim_freq / fbpm);

    /* if somehow invalid, throw error */
    if (!(new_period >= 1 && new_period < MAX_PSC))
        return DMETRO_INVALID_PSC;
        
    /* we need to change variables that could be altered in other threads, */
    /* so disable interrupts for this critical section */
    nvic_disable_irq(NVIC_TIM4_IRQ);
    
    /* if the counter slips past the new prescale value, we need to reset it */
    if ((counter = timer_get_counter(TIM4)) > beat_period) {
        /* scale the counter according to the previous period */
        timer_set_counter(TIM4, (uint32_t) ((counter * beat_period) / new_period));
    }
    /* now update period */
    beat_period = new_period;
    timer_set_period(TIM4, beat_period);

    /* set new value for quaver */
    half_period = (uint32_t) round((double) beat_period / 2.0);
    /* use Output Compare channel 2 (OC2) for quavers */
    timer_set_oc_value(TIM4, TIM_OC2, half_period);

    /* reset sequence */
    beat_idx = 0;

    /* exit critical section */
    nvic_enable_irq(NVIC_TIM4_IRQ);

    return OK;
}

/* Start metronome functionality. This will trigger beat 1 */
extern void dmetro_start(void) {
    /* reset beat index */
    beat_idx = 0;

    led_status = false;

    /* reset counter */
    timer_set_counter(TIM4, 0U);

    /* pend overflow (beat 1) */
    timer_generate_event(TIM4, TIM_EGR_UG);

    /* mark tempo as started */
    tempo_status = true;

    /* start counter */
    timer_enable_counter(TIM4);

    /* enable interrupts */
    nvic_enable_irq(NVIC_TIM4_IRQ);
}

/* stop metronome and reset back to beat 1. */
extern void dmetro_stop(void) {
    /* disable interrupts */
    nvic_disable_irq(NVIC_TIM4_IRQ);

    /* stop counter */
    timer_disable_counter(TIM4);

    /* mark status of tmepo */
    tempo_status = false;

    /* reset flags */
    timer_clear_flag(TIM4, TIM_SR_UIF);
    timer_clear_flag(TIM4, TIM_SR_CC1IF);
    timer_clear_flag(TIM4, TIM_SR_CC2IF);

    led_status = false;
}

/* push button functionality */
static void dmetro_toggle_tempo(void) {
    tempo_status = !tempo_status;
    if (tempo_status)
        dmetro_start();
    else 
        dmetro_stop();
}

/******************* init functions ****************/
extern error_t dmetro_setup(void) {
    error_t err;
    
    /* initialise global variables */
    clk_psc = 2048U;
    metro_tim_freq = (84000000.0 / (double) clk_psc);  
    pulse_period = (uint32_t) (metro_tim_freq / (1.0 / ((double) pulse_period_ms / 1000.0)));
    
    /* only crotchets to start with */
    uint8_t seq[] = {1, 0, 1, 0, 1, 0, 1, 0};

    /* play button setup */
    /* clock the gpio for the button and the internal sys config (to setup EXTI)*/
    rcc_periph_clock_enable(RCC_GPIOC);
    rcc_periph_clock_enable(RCC_SYSCFG);
    
    /* configure EXTI settings */
    exti_set_trigger(EXTI1, EXTI_TRIGGER_RISING);
    exti_select_source(EXTI1, GPIOC);
    exti_enable_request(EXTI1);
    nvic_enable_irq(NVIC_EXTI1_IRQ);
    gpio_mode_setup(START_STOP_PORT, GPIO_MODE_INPUT, GPIO_PUPD_PULLDOWN, START_STOP_PIN);

    /* clock the timer peripheral and the led output */
    rcc_periph_clock_enable(RCC_TIM4);
    rcc_periph_clock_enable(RCC_GPIOB);

    /* set up LED pin */
    gpio_mode_setup(
        METRONOME_CH1_PORT, 
        GPIO_MODE_OUTPUT, 
        GPIO_PUPD_PULLDOWN,
        METRONOME_CH1_PIN
    );

    /* disable counter before configuring */
    timer_disable_counter(TIM4);
    
    /* configure initial settings */
    timer_set_mode(
        TIM4, 
        TIM_CR1_CKD_CK_INT_MUL_4,   
        TIM_CR1_CMS_EDGE, 
        TIM_CR1_DIR_UP
    );

    /* set initial prescaler for the timer */
    timer_set_prescaler(TIM4, clk_psc);
    
    // disabling preload means that the new period will be effective immediately
    // preload enabled would mean each period would reset, which is not desirable.
    timer_disable_preload(TIM4);
    timer_continuous_mode(TIM4);

    /* set initial tempo */
    if ((err = dmetro_set_tempo(BPM_START)))
        return err; 
    /* set initial sequence */
    if ((err = dmetro_set_sequence(seq)))
        return err;

    /* update interupt is overflow, occurs on main beat (crotchet) */
    timer_enable_irq(TIM4, TIM_DIER_UIE);

    /* Output compare 1 is toggle */
    timer_enable_irq(TIM4, TIM_DIER_CC1IE);
    
    /* output compare 2 is offbeat (quaver) */
    timer_enable_irq(TIM4, TIM_DIER_CC2IE);
    /* enable from the NVIC */
    nvic_enable_irq(NVIC_TIM4_IRQ);
    
    /* don't enable metronome, wait for user to start it. */

    /* initialise ADC for reading tempo measurement */
    return (err = dadc_setup());
}

extern error_t dmetro_teardown(void) {
    nvic_disable_irq(NVIC_TIM4_IRQ);
    timer_disable_irq(TIM4, TIM_DIER_UIE);
    timer_disable_counter(TIM4);

    rcc_periph_clock_disable(RCC_TIM4);
    return OK;
}

extern error_t dadc_setup(void) {
    rcc_periph_clock_enable(RCC_ADC1);
    rcc_periph_clock_enable(RCC_GPIOA);

    // analog or AF?
    gpio_mode_setup(ADC_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, TEMPO_PIN | VOLUME_PIN);

    adc_power_off(ADC1);
    
    adc_disable_external_trigger_regular(ADC1);
    adc_set_right_aligned(ADC1);
    adc_set_clk_prescale((uint32_t) 1);

    adc_set_single_conversion_mode(ADC1);
    adc_power_on(ADC1);                              
    
    adc_clear_flag(ADC1, ADC_SR_EOC);       // ensure flag is cleared
    // todo set up interrupts if needed

    return OK;
}

extern error_t dadc_teardown(void) {
    adc_power_off(ADC1);
    rcc_periph_clock_disable(RCC_ADC1);
    return OK;
}