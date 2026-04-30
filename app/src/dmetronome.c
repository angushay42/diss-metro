#include "dmetronome.h"
#include "duart.h"

static uint32_t _psc;
static uint16_t _bpm;

#define ADC_PORT    (GPIOA)
#define TEMPO_PIN   (GPIO0)
#define VOLUME_PIN  (GPIO1)

static uint8_t tempo_group[] = {ADC_CHANNEL0};
static uint8_t volume_group[] = {ADC_CHANNEL1};

static uint16_t scale_to_bpm(uint16_t reading);

/**
 * Setup ADC1 to recieve two analog inputs:
 * volume and tempo.
 */
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
    //todo tear down interrupts if needed.
    rcc_periph_clock_disable(RCC_ADC1);
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
extern error_t dmetro_get_tempo_reading(uint16_t *data, uint16_t cycle_timeout) {
    uint32_t i;
    adc_set_regular_sequence(ADC1, 1, tempo_group);
    adc_start_conversion_regular(ADC1);
    for (i = 0; !(adc_eoc(ADC1)); i++)
        if (i > cycle_timeout) {
            *data = 1 << 14;
            return DADC_TIMEOUT;
        }

    *data = scale_to_bpm(adc_read_regular(ADC1));

    return OK;
}

/* read volume measurement from ADC. Returns 0 if successful */
extern error_t dmetro_get_volume_reading(uint16_t *data) {
    adc_set_regular_sequence(ADC1, 1, volume_group);
    adc_start_conversion_regular(ADC1);
    while (!(adc_eoc(ADC1))); 

    *data = adc_read_regular(ADC1);
    return OK;
}

/* get current tempo*/
extern uint16_t dmetro_get_tempo(void) {
    // safe?
    return _bpm;
}

/* sets _psc */
static error_t convert_to_psc(uint16_t bpm) {
    double input, output, psc;
    uint32_t temp;
    if (bpm < MIN_BPM || bpm > MAX_BPM)
        return DMETRO_INVALID_TEMPO;

    // input / output = psc
    input = (double) (rcc_apb1_frequency * 2) / (double) MAX_PSC;
    output = (double) bpm / 60.0;
    psc = roundf(input / output);
    temp = (uint32_t) psc;
    if (temp >= MAX_PSC)
        return DMETRO_INVALID_PSC;
    _psc = (uint32_t) psc;
    return OK;
}

/* Set bpm of metronome. Uses defined min and max bpm */
extern error_t dmetro_set_tempo(uint16_t bpm) {
    error_t err;
    if (bpm < MIN_BPM || bpm > MAX_BPM)
        return (err = DMETRO_INVALID_TEMPO);   // error
    
    
    if ((err = convert_to_psc(bpm)))
        return err;
    _bpm = bpm;

    // does it need to be -1?
    timer_set_period(TIM4, _psc);
    // force an update
    if (timer_get_counter(TIM4) > _psc) {
        // timer_generate_event(TIM4, TIM_CR2_MMS_UPDATE)
        timer_set_counter(TIM4, 0);     // todo gives it a delay
    }
    return OK;
}

extern void dmetro_start(void) {
    timer_enable_counter(TIM4);
    // todo trigger pulse ?
    // read manual, update might occur if something is set already
}

extern void dmetro_stop(void) {
    timer_disable_counter(TIM4);
}

extern error_t dmetro_setup(void) {
    error_t err;
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

    timer_enable_irq(TIM4, TIM_DIER_UIE);    // update on full count

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

extern void tim4_isr(void) {
    // duart_write_bytes("before isr check\n");
    // gpio_toggle(TEST_LED_PORT, TEST_LED_PIN);
    if (timer_get_flag(TIM4, TIM_SR_UIF)) {
        // duart_write_bytes("after isr check\n");

        gpio_set(METRONOME_CH1_PORT, METRONOME_CH1_PIN);
        delay_ms(100);
        gpio_clear(METRONOME_CH1_PORT, METRONOME_CH1_PIN);

        timer_clear_flag(TIM4, TIM_SR_UIF);

        // duart_write_bytes("after isr clear\n");
    }
}