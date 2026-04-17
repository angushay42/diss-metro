#include "metronome.h"

static uint32_t _psc;
static uint32_t _bpm;

extern uint32_t metro_get_tempo(void) {
    // safe?
    return _bpm;
}

/* Set bpm of metronome. Uses defined min and max bpm */
extern int metro_set_tempo(uint32_t bpm) {
    if (!((MIN_BPM <= bpm) && (bpm <= MAX_BPM)))
        return 1;   // error
    
    // input / output = psc
    _psc = (uint32_t) roundf((float) (rcc_apb1_frequency / MAX_PSC) / (bpm / 60.0));
    _bpm = bpm;

    timer_set_period(TIM4, _psc - 1);
    return 0;
}

extern void metro_start(void) {
    timer_enable_counter(TIM4);
    // todo trigger pulse ?
    // read manual, update might occur if something is set already
}

extern void metro_stop(void) {
    timer_disable_counter(TIM4);
}

extern int metro_setup(void) {
    //timer setup
    rcc_periph_clock_enable(RCC_TIM4);
    rcc_periph_clock_enable(RCC_GPIOB);
    nvic_enable_irq(NVIC_TIM4_IRQ);

    timer_set_mode(
        TIM4, 
        TIM_CR1_CKD_CK_INT_MUL_4,   
        TIM_CR1_CMS_EDGE, 
        TIM_CR1_DIR_UP
    );
    
    // 84MHz / 65535 = 1.281KHz
    timer_set_prescaler(TIM4, MAX_PSC);
    
    // disabling preload means that the new period will be effective immediately
    // preload enabled would mean each period would reset, which is not desirable.
    timer_disable_preload(TIM4);
    timer_continuous_mode(TIM4);

    if (metro_set_tempo(BPM_START))
        return 1; 

    timer_enable_counter(TIM4);
    timer_enable_irq(TIM4, TIM_DIER_UIE);    // update on full count

    gpio_mode_setup(
        METRONOME_CH1_PORT, 
        GPIO_MODE_OUTPUT, 
        GPIO_PUPD_PULLDOWN,
        METRONOME_CH1_PIN
    );

    // gpio_set_af(METRONOME_CH1_PORT, GPIO_AF2, METRONOME_CH1_PIN);
    //todo remove
    rcc_periph_clock_enable(RCC_GPIOA);
    gpio_mode_setup(TEST_LED_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, TEST_LED_PIN);
    gpio_set(TEST_LED_PORT, TEST_LED_PIN);
    gpio_set(METRONOME_CH1_PORT, METRONOME_CH1_PIN);
    
    return 0;   // OK
}

// TODO 
extern void tim4_isr(void) {
    
    // gpio_toggle(TEST_LED_PORT, TEST_LED_PIN);
    if (timer_get_flag(TIM4, TIM_SR_UIF)) {
        timer_clear_flag(TIM4, TIM_SR_UIF);
        gpio_toggle(TEST_LED_PORT, TEST_LED_PIN);
        gpio_toggle(METRONOME_CH1_PORT, METRONOME_CH1_PIN);
    }
}