#include "common.h"

// Four LEDs connected to GPIO1_21-24
#define LED_MASK (0xF << 21)
#define LED_PATTERN	 (0xA << 21)

static void init_led_output(void)
{
	// Enable the GPIO module - CM_PER_GPIO1_CLKCTRL
	*(volatile uint32_t *)(0x44e00000 + 0xAC) = (0x1 << 18) | (0x2 << 0);

	// Enable output - GPIO1_OE
	*(volatile uint32_t *)(0x4804C000 + 0x134) &= ~LED_MASK;
}

static void heartbeat_forever(void)
{
	for (;;)
	{
		// On - GPIO1_DATAOUT
		*(volatile uint32_t *)(0x4804C000 + 0x13C) |= LED_PATTERN;
		delay(500000);

		// Off - GPIO1_DATAOUT
		*(volatile uint32_t *)(0x4804C000 + 0x13C) &= ~LED_PATTERN;
		delay(380000);

		// On
		*(volatile uint32_t *)(0x4804C000 + 0x13C) |= LED_PATTERN;
		delay(240000);

		// Off
		*(volatile uint32_t *)(0x4804C000 + 0x13C) &= ~LED_PATTERN;
		delay(2400000);
	}
}

int c_entry(void)
{
	init_led_output();
	heartbeat_forever();

	return 0;
}
