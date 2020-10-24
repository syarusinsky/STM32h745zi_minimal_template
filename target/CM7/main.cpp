#include "../../lib/STM32h745zi-HAL/llpd/include/LLPD.hpp"

int main(void)
{
	LLPD::gpio_enable_clock( GPIO_PORT::A );
	LLPD::gpio_output_setup( GPIO_PORT::A, GPIO_PIN::PIN_0, GPIO_PUPD::NONE, GPIO_OUTPUT_TYPE::PUSH_PULL, GPIO_OUTPUT_SPEED::LOW );
	LLPD::gpio_output_set( GPIO_PORT::A, GPIO_PIN::PIN_0, true );

	unsigned int someVar = 7;
	while ( true )
	{
		someVar += 3;

		while ( someVar % 2 == 1 )
		{
			someVar /= 2;
		}
	}
}
