#include "../../lib/STM32h745zi-HAL/llpd/include/LLPD.hpp"

int main(void)
{
	LLPD::rcc_clock_start_max_cpu2();

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
