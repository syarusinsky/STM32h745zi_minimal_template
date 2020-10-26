#include "../../lib/STM32h745zi-HAL/llpd/include/LLPD.hpp"

const int SYS_CLOCK_FREQUENCY = 64000000;

volatile bool ledIsOn = false;

int main(void)
{
	// audio timer setup (for 40 kHz sampling rate at 64 MHz system clock)
	LLPD::tim6_counter_setup( 2, 800, 40000 );
	LLPD::tim6_counter_enable_interrupts();

	// test led setup
	LLPD::gpio_enable_clock( GPIO_PORT::A );
	LLPD::gpio_output_setup( GPIO_PORT::A, GPIO_PIN::PIN_0, GPIO_PUPD::NONE, GPIO_OUTPUT_TYPE::PUSH_PULL, GPIO_OUTPUT_SPEED::LOW );
	LLPD::gpio_output_set( GPIO_PORT::A, GPIO_PIN::PIN_0, ledIsOn );

	// audio timer start
	LLPD::tim6_counter_start();

	unsigned int someVar = 7;
	while ( true )
	{
		LLPD::tim6_delay( 500000 );

		ledIsOn = !ledIsOn;
		LLPD::gpio_output_set( GPIO_PORT::A, GPIO_PIN::PIN_0, ledIsOn );

		someVar += 3;

		while ( someVar % 2 == 1 )
		{
			someVar /= 2;
		}
	}
}

extern "C" void TIM6_DAC_IRQHandler (void)
{
	if ( ! LLPD::tim6_isr_handle_delay() ) // if not currently in a delay function,...
	{
	}

	LLPD::tim6_counter_clear_interrupt_flag();
}
