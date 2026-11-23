#include "board.h"
#include "stm32f1xx.h"

#define LED (1U << 0)
#define BUZZER (1U << 1)
#define RELAY (1U << 10)
#define RX_MASK 127U

static volatile uint8_t rx_data[RX_MASK + 1U];
static volatile unsigned rx_head, rx_tail;
static volatile bool rx_error;

static void gpio_mode(GPIO_TypeDef *gpio, unsigned pin, unsigned mode)
{
    volatile uint32_t *reg = pin < 8U ? &gpio->CRL : &gpio->CRH;
    unsigned shift = (pin % 8U) * 4U;
    *reg = (*reg & ~(15U << shift)) | (mode << shift);
}

void board_wait_us(uint16_t us)
{
    uint16_t start = (uint16_t)TIM2->CNT;
    while ((uint16_t)((uint16_t)TIM2->CNT - start) < us) {}
}

void board_init(void)
{
    // Reset clock configuration: HSI 8 MHz, AHB/APB1/APB2 undivided.
    SCB->VTOR = FLASH_BASE;
    __DSB();
    __ISB();

    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN |
                    RCC_APB2ENR_ADC1EN | RCC_APB2ENR_USART1EN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    (void)RCC->APB2ENR;
    (void)RCC->APB1ENR;

    // Preload the OFF level before switching the pins to outputs.
    board_outputs(false, false);
    gpio_mode(GPIOB, 0, 2);
    gpio_mode(GPIOB, 1, 2);
    gpio_mode(GPIOB, 10, 2);

    GPIOA->BSRR = 1U << 1;
    GPIOB->BSRR = (1U << 12) | (1U << 13);
    gpio_mode(GPIOA, 0, 0);
    gpio_mode(GPIOA, 1, 8);
    gpio_mode(GPIOB, 12, 8);
    gpio_mode(GPIOB, 13, 8);

    TIM2->PSC = 7;
    TIM2->ARR = 65535;
    TIM2->EGR = TIM_EGR_UG;
    TIM2->CR1 = TIM_CR1_CEN;

    ADC1->SMPR2 = 7U; // CH0, 239.5 ADC cycles.
    ADC1->CR2 = ADC_CR2_ADON;
    board_wait_us(10);
    ADC1->CR2 |= ADC_CR2_RSTCAL;
    while (ADC1->CR2 & ADC_CR2_RSTCAL) {}
    ADC1->CR2 |= ADC_CR2_CAL;
    while (ADC1->CR2 & ADC_CR2_CAL) {}
    ADC1->CR2 |= ADC_CR2_EXTTRIG | ADC_CR2_EXTSEL;

    gpio_mode(GPIOA, 9, 10);  // USART1 TX, alternate push-pull.
    GPIOA->BSRR = 1U << 10;
    gpio_mode(GPIOA, 10, 8);  // USART1 RX, pull-up.
    USART1->BRR = (8000000U + 57600U) / 115200U;
    USART1->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE;
    NVIC_SetPriority(USART1_IRQn, 6);
    NVIC_EnableIRQ(USART1_IRQn);
}

unsigned board_mq_adc(void)
{
    ADC1->CR2 |= ADC_CR2_SWSTART;
    while (!(ADC1->SR & ADC_SR_EOC)) {}
    return ADC1->DR & 4095U;
}

bool board_flame(void) { return !(GPIOA->IDR & (1U << 1)); }
bool board_button(unsigned index) { return !(GPIOB->IDR & (1U << (12U + index))); }

void board_outputs(bool alarm, bool fan)
{
    bool buzzer_pin_high = alarm == (BUZZER_ACTIVE_HIGH != 0);
    uint32_t high = (alarm ? LED : 0U) |
                    (buzzer_pin_high ? BUZZER : 0U) |
                    (fan ? RELAY : 0U);
    GPIOB->BSRR = high | (((LED | BUZZER | RELAY) & ~high) << 16);
}

void board_led(bool on) { GPIOB->BSRR = on ? LED : LED << 16; }

void USART1_IRQHandler(void)
{
    uint32_t flags = USART1->SR;
    if (!(flags & (USART_SR_RXNE | USART_SR_ORE | USART_SR_FE | USART_SR_NE))) return;
    uint8_t byte = (uint8_t)USART1->DR;
    unsigned next = (rx_head + 1U) & RX_MASK;
    if (next == rx_tail || (flags & (USART_SR_ORE | USART_SR_FE | USART_SR_NE))) {
        rx_error = true;
    } else {
        rx_data[rx_head] = byte;
        rx_head = next;
    }
}

int board_uart_get(void)
{
    int result = -1;
    NVIC_DisableIRQ(USART1_IRQn);
    if (rx_error) {
        rx_error = false;
        rx_tail = rx_head;
        result = -2;
    } else if (rx_tail != rx_head) {
        result = rx_data[rx_tail];
        rx_tail = (rx_tail + 1U) & RX_MASK;
    }
    NVIC_EnableIRQ(USART1_IRQn);
    return result;
}

void board_uart_put(char ch)
{
    while (!(USART1->SR & USART_SR_TXE)) {}
    USART1->DR = (uint8_t)ch;
}
