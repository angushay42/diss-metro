// libopencm3 defines
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/cm3/nvic.h>        // from LowByteProductions
#include <libopencm3/stm32/syscfg.h>
#include <libopencm3/cm3/systick.h>

// other defines
#include <math.h>

// own defines
#include "diss-uart.h"

#define CS_PORT (GPIOA)
#define CS_PIN (GPIO4)

/******************  metronome defines *************/
#define MAX_BPM (40)
#define MIN_BPM (220)
#define MAX_PSC (uint32_t)((1 << 16) - 1)
// stage 1
#define METRONOME_TIMER          (TIM4)      // TIM4 doesn't have an ETR
#define METRONOME_RCC            (RCC_TIM4)
#define METRONOME_CH1_PORT       (GPIOB)
#define METRONOME_CH1_PIN        (GPIO6)

static uint32_t bpm;
static double   hz;


static void rcc_setup(void) {
    rcc_clock_setup_pll(&rcc_hsi_configs[RCC_CLOCK_3V3_84MHZ]);
}

static void gpio_setup(void) {
    rcc_periph_clock_enable(RCC_SPI1);
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(METRONOME_RCC);


    // /********** spi setup ************/
    // // SCLK is A5
    // gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLDOWN, GPIO5);
    // // MISO is A6
    // gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLDOWN, GPIO6);
    // // MOSI is A7
    // gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLDOWN, GPIO7);
    // // CS is A4 
    // gpio_mode_setup(CS_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, CS_PIN);

    // // these are given by datasheet, thankfully all in same AF column
    // gpio_set_af(GPIOA, GPIO_AF5, GPIO5 | GPIO6 | GPIO7);
    // /******** endSPI  *****************/


    /********** UART setup ************/
    gpio_mode_setup(UART_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLDOWN, UART_TX_PIN); 
    gpio_mode_setup(UART_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLDOWN, UART_RX_PIN); 

    gpio_set_af(UART_PORT, UART_AF_MODE, UART_TX_PIN | UART_RX_PIN);
    /******** endUART  *****************/

    /************** TIMER  *****************/

    /* STAGE 1*/
    gpio_mode_setup(
        METRONOME_CH1_PORT, 
        GPIO_MODE_AF, 
        GPIO_PUPD_NONE,     // correct?
        METRONOME_CH1_PIN
    );
    // todo remove
    gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO5);    // LED2
    gpio_set(GPIOA, GPIO5);
    
    gpio_set_af(METRONOME_CH1_PORT, GPIO_AF2, METRONOME_CH1_PIN);   // needed?
    /************** endTIMER  *****************/
}

static void spi_setup(void) {
    spi_init_master(
        SPI1, 
        SPI_CR1_BAUDRATE_FPCLK_DIV_64, 
        SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE,
        SPI_CR1_CPHA_CLK_TRANSITION_1,
        SPI_CR1_DFF_16BIT,
        SPI_CR1_MSBFIRST
    );
    spi_enable_software_slave_management(SPI1);
    spi_set_nss_high(SPI1); // tell stm it is always master
    spi_enable(SPI1);
}

static void spi_rcv(void) {
    // write blank statement to ADC to start transfer (Claude)
    gpio_clear(CS_PORT, CS_PIN);
    spi_xfer(SPI1, 0x0001);
    // set gpio pin high
    gpio_set(CS_PORT, CS_PIN);
}



static void timer_setup(void) {
    // init vars
    int err;
    uint32_t psc;
    
    if (!err) {;}
        // do something? 

    // init clock
    rcc_clock_setup_pll(&rcc_hsi_configs[RCC_CLOCK_3V3_84MHZ]);

    //timer setup
    rcc_periph_clock_enable(METRONOME_RCC);
    nvic_enable_irq(NVIC_TIM4_IRQ);     // stage 1 is TIM4

    timer_set_mode(
        METRONOME_TIMER, 
        TIM_CR1_CKD_CK_INT_MUL_4,   
        TIM_CR1_CMS_EDGE, 
        TIM_CR1_DIR_UP
    );
    
    // 84MHz / 65535 = 1.281KHz
    timer_set_prescaler(METRONOME_TIMER, MAX_PSC);
    bpm = 120;  // todo magic number
    hz = (float) bpm / 60.0;
    // input / output = psc
    psc = (uint32_t) roundf((float) (rcc_apb1_frequency / MAX_PSC) / hz);
    
    timer_disable_preload(METRONOME_TIMER);
    timer_continuous_mode(METRONOME_TIMER);

    timer_set_period(METRONOME_TIMER, psc);

    timer_enable_counter(METRONOME_TIMER);
    timer_enable_irq(METRONOME_TIMER, TIM_DIER_UIE);    // update on full count

}

// todo tim4 here but metronome_timer everywhere else
void tim4_isr(void) {
    if (timer_get_flag(METRONOME_TIMER, TIM_SR_UIF)) {
        timer_clear_flag(METRONOME_TIMER, TIM_SR_UIF);
        gpio_toggle(GPIOA, GPIO5);
    }
}
/* might not need to do this ?*/
static void systick_setup(void) {
    // systick_set_frequency(84000000, );
    
}

void delay_cycles(uint32_t cycles) {
    volatile int i;
    for (i = 0; i < cycles; i++) {
        __asm__ volatile ("NOP");
    }
}


int main(void) {
    rcc_setup();
    gpio_setup();
    spi_setup();
    uart_setup();
    timer_setup();

    uint8_t data[30];

    while (1) {
        // todo
        // uart_write();    
    }
    return 0;
}
