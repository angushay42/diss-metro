#include "metronome.h"

/* Set bpm of metronome. Uses defined min and max bpm */
static int metro_set_pulse(uint32_t bpm) {
    if (!(MIN_BPM <= bpm <= MAX_BPM))
        return 1;   // error
    
    // input / output = psc
    psc = (uint32_t) roundf((float) (rcc_apb1_frequency / MAX_PSC) / (bpm / 60.0));
    
    timer_set_period(TIM4, psc);
    return 0;
}

// todo start/stop etc

extern void metro_setup(void) {
    uint32_t psc;

    // init clock
    rcc_clock_setup_pll(&rcc_hsi_configs[RCC_CLOCK_3V3_84MHZ]);

    //timer setup
    rcc_periph_clock_enable(RCC_TIM4);
    nvic_enable_irq(NVIC_TIM4_IRQ);     // stage 1 is TIM4

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

    metro_set_pulse(BPM_START);

    timer_enable_counter(TIM4);
    timer_enable_irq(TIM4, TIM_DIER_UIE);    // update on full count


    /************** TIMER  *****************/

    /* STAGE 1*/
    gpio_mode_setup(
        METRONOME_CH1_PORT, 
        GPIO_MODE_OUTPUT, 
        GPIO_PUPD_PULLDOWN,
        METRONOME_CH1_PIN
    );
    
    /************** endTIMER  *****************/

}

// TODO 
extern void tim4_isr(void) {
    if (timer_get_flag(TIM4, TIM_SR_UIF)) {
        timer_clear_flag(TIM4, TIM_SR_UIF);
        // avoids toggling each clock tick
        gpio_toggle(METRONOME_CH1_PORT, METRONOME_CH1_PIN);
    }
}