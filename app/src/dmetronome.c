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
static uint16_t scale_to_bpm(uint16_t reading);



/*********** global variables *************/
/* private reference to the current timer prescaler value */
static uint32_t _psc;
/* private reference to the current bpm of the metronome */
static uint16_t _bpm;

/* adc reads groups of channels, in this case we just need one for each reading. */
/* array of channels to read from ADC for the tempo measurement */
static uint8_t tempo_group[] = {ADC_CHANNEL0};
/* array of channels to read from the ADC for volume */
static uint8_t volume_group[] = {ADC_CHANNEL1};

/* flags used for stopping and starting the metronome with a push button */
bool tempo_toggle, tempo_started = false;

/* time in ms of last button toggle*/
uint64_t last_toggle, debounce_delay = 200; 

/* most recent timestamp (in ms) of the tempo pulse. */
volatile uint64_t beat_stamp;

/* time in ms of last time tempo was polled */
uint64_t last_poll;
/* flag for starting polling */
bool poll_started = false;

/** input frequency of the timer peripheral, divided by 65535 (max allowable prescaler). 
 * This is to help create very low frequency clocks 
 */
static const double input = (double) (84000000) / (double) MAX_PSC;

/* time in ms for LED pulse */
static const uint32_t pulse_period = 100;

/* Prescaler value for the LED pulse */
static const uint32_t pulse_psc = (uint32_t) (input / (1.0 / ((double) pulse_period / 1000.0)));


/*************** ISR handlers ******************/
void exti1_isr(void) {
    // flag_status is 1 if triggered by selected trigger, 0 otherwise,
    // so only continue if we want to
    if (exti_get_flag_status(EXTI1)) {
        uint64_t now = get_time(false);
        /* if first time, initialise last_toggle */
        if (!tempo_started) {
            last_toggle = get_time(false);
            tempo_started = true;
        }
        
        /* check if this interrupt fired < delay seconds ago */
        if (now - last_toggle >= debounce_delay) {
            last_toggle = now;
            tempo_toggle = !tempo_toggle;
            if (tempo_toggle)
                dmetro_start();
            else 
                dmetro_stop();
        }
    }
    // clear flag
    exti_reset_request(EXTI1);
}


void tim4_isr(void) {
    /* this interrupt should trigger ON the pulse. */
    if (timer_get_flag(TIM4, TIM_SR_UIF)) {
        beat_stamp = get_time(true);
        
        /* turn LED on*/
        gpio_set(METRONOME_CH1_PORT, METRONOME_CH1_PIN);

        /* enable output compare for pulsing the LED */
        timer_enable_oc_output(TIM4, TIM_OC1);

        // /* use Output Compare to turn the LED off after pulse_period ms */
        timer_set_oc_value(TIM4, TIM_OC1, pulse_psc);

        /* clear flag */
        timer_clear_flag(TIM4, TIM_SR_UIF);
    }
    /* this interrupt should trigger 100ms AFTER the pulse. */
    else if (timer_get_flag(TIM4, TIM_SR_CC1IF)) {
        /* turn LED off */
        gpio_clear(METRONOME_CH1_PORT, METRONOME_CH1_PIN);
        
        /* disable output compare */
        timer_disable_oc_output(TIM4, TIM_OC1);

        timer_clear_flag(TIM4, TIM_SR_CC1IF);
    }

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
extern error_t dmetro_get_tempo_reading(uint16_t *data, uint16_t cycle_timeout) {
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
    /* if this is first loop, initialise last poll */
    if (!poll_started) {
        last_poll = now;
        poll_started = true;
    }
        
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

/* get current tempo*/
extern uint16_t dmetro_get_tempo(void) {
    // safe?
    return _bpm;
}



/**
 * Converts bpm into a prescaler value for the timer. 
 * Psc will be the value that the timer counts up to (thus dividing the frequency).
 * Assigns the new value into the value pointed at by psc_ptr
 * Returns 0 if successful.
 */
static error_t convert_to_psc(uint16_t bpm, uint32_t *psc_ptr) {
    if (bpm < MIN_BPM || bpm > MAX_BPM)
        return DMETRO_INVALID_TEMPO;
    uint32_t temp; 
    
    temp = (uint32_t) roundf(input / ((double) bpm / 60.0));
    if (temp >= MAX_PSC)
        return DMETRO_INVALID_PSC;
    *psc_ptr = temp;
    return OK;
}

/* Set bpm of metronome. Uses defined min and max bpm */
extern error_t dmetro_set_tempo(uint16_t bpm) {
    error_t err;
    uint32_t counter, prev_period;
    if (bpm < MIN_BPM || bpm > MAX_BPM)
        return (err = DMETRO_INVALID_TEMPO);   // error
    
    /* save previous period */
    prev_period = _psc;
    /* convert to a value the timer can use */
    if ((err = convert_to_psc(bpm, &_psc)))
        return err;
    /* if no error, we can safely set the new bpm */
    _bpm = bpm;

    /* update the period of the timer */
    timer_set_period(TIM4, _psc);
    /* if the counter slips past the new prescale value, we need to reset it*/
    if ((counter = timer_get_counter(TIM4)) > _psc) {
        /* scale the counter according to the previous period */
        timer_set_counter(TIM4, (uint32_t) (counter / prev_period * _psc));
    }
    return OK;
}

/******************* metronome interaction ****************/
extern void dmetro_start(void) {
    timer_enable_counter(TIM4);
}

extern void dmetro_stop(void) {
    timer_disable_counter(TIM4);
}

/******************* init functions ****************/
extern error_t dmetro_setup(void) {
    error_t err;

    /* play button setup */
    // enable gpio
    // enable irq and clock the peripheral
    rcc_periph_clock_enable(RCC_GPIOC);
    rcc_periph_clock_enable(RCC_SYSCFG);
    
    exti_set_trigger(EXTI1, EXTI_TRIGGER_RISING);
    exti_select_source(EXTI1, GPIOC);
    exti_enable_request(EXTI1);
    nvic_enable_irq(NVIC_EXTI1_IRQ);
    gpio_mode_setup(START_STOP_PORT, GPIO_MODE_INPUT, GPIO_PUPD_PULLDOWN, START_STOP_PIN);

    //timer setup
    rcc_periph_clock_enable(RCC_TIM4);
    rcc_periph_clock_enable(RCC_GPIOB);
    nvic_enable_irq(NVIC_TIM4_IRQ);

    timer_disable_counter(TIM4);
    timer_set_mode(
        TIM4, 
        TIM_CR1_CKD_CK_INT_MUL_4,   
        TIM_CR1_CMS_EDGE, 
        TIM_CR1_DIR_UP
    );
    
    // 84MHz / 65535 = 1.281KHz
    // APB frequency is 42MHz, so it has broken over time.
    // can't change the clock div, so it will be 84MHz...
    timer_set_prescaler(TIM4, MAX_PSC);
    
    // disabling preload means that the new period will be effective immediately
    // preload enabled would mean each period would reset, which is not desirable.
    timer_disable_preload(TIM4);
    timer_continuous_mode(TIM4);

    if ((err = dmetro_set_tempo(BPM_START)))
        return err; 

    /* update interupt is overflow */
    timer_enable_irq(TIM4, TIM_DIER_UIE);

    /* cc is compare/capture, in this case output compare. */
    timer_enable_irq(TIM4, TIM_DIER_CC1IE);
    

    gpio_mode_setup(
        METRONOME_CH1_PORT, 
        GPIO_MODE_OUTPUT, 
        GPIO_PUPD_PULLDOWN,
        METRONOME_CH1_PIN
    );


    timer_enable_counter(TIM4);
    // timer_update_on_overflow(TIM4);
    return (err = dadc_setup());
}

extern error_t dmetro_teardown(void) {
    timer_disable_irq(TIM4, TIM_DIER_UIE);
    timer_disable_counter(TIM4);

    rcc_periph_clock_disable(RCC_TIM4);
    nvic_disable_irq(NVIC_TIM4_IRQ);
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