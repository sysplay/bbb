#include "bbb.h"
#include "leds.h"

#define LED_GPIO_START_BIT 21
#define LED_MASK (0xF << LED_GPIO_START_BIT)

void leds_init(void)
{
	// Enable the GPIO1 clock
	CM_PER_GPIO1_CLKCTRL = (0x1 << 18) | (0x2 << 0);

	// Enable output
	GPIO1_OE &= ~LED_MASK;
}
void leds_shut(void)
{
	// Disable output
	GPIO1_OE |= LED_MASK;

	// Disable the GPIO1 clock
	//CM_PER_GPIO1_CLKCTRL = 0; // Problem in case others using GPIO1
}

void leds_on(uint32_t led_no)
{
	GPIO1_DATAOUT |= (1 << (LED_GPIO_START_BIT + led_no));
}
void leds_off(uint32_t led_no)
{
	GPIO1_DATAOUT &= ~(1 << (LED_GPIO_START_BIT + led_no));
}
void leds_toggle(uint32_t led_no)
{
	GPIO1_DATAOUT ^= (1 << (LED_GPIO_START_BIT + led_no));
}
