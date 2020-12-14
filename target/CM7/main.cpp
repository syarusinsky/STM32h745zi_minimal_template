#include "../../lib/STM32h745zi-HAL/llpd/include/LLPD.hpp"

#include "SRAM_23K256.hpp"

const int SYS_CLOCK_FREQUENCY = 16000000;

volatile bool keepBlinking = true;
volatile bool ledIsOn = false;
volatile int ledIncr = 0; // should flash led every time this value is equal to ledMax
volatile int ledMax = 20000;

int main(void)
{
	// setup clock
	// LLPD::rcc_pll_setup( RCC_CLOCK_SOURCE::HSE, PLL_INPUT_FREQ_RANGE::BETWEEN_8_AND_16_MHZ, 1, 10 );
	LLPD::rcc_clock_setup( CORE::CM7, RCC_CLOCK_SOURCE::HSE, RCC_CLOCK_SOURCE_PRESCALER::DISABLED, false );
	// LLPD::rcc_clock_setup( CORE::CM4, RCC_CLOCK_SOURCE::HSE, RCC_CLOCK_SOURCE_PRESCALER::DIV_BY_2, false );
	// TODO need to do way more work with clock and make sure it's right

	// enable gpio clocks
	LLPD::gpio_enable_clock( GPIO_PORT::A );
	LLPD::gpio_enable_clock( GPIO_PORT::B );
	LLPD::gpio_enable_clock( GPIO_PORT::C );
	LLPD::gpio_enable_clock( GPIO_PORT::D );
	LLPD::gpio_enable_clock( GPIO_PORT::E );
	LLPD::gpio_enable_clock( GPIO_PORT::F );
	LLPD::gpio_enable_clock( GPIO_PORT::G );
	LLPD::gpio_enable_clock( GPIO_PORT::H );

	// audio timer setup (for 40 kHz sampling rate at 16 MHz system clock)
	LLPD::tim6_counter_setup( 1, 400, 40000 );
	LLPD::tim6_counter_enable_interrupts();

	// test led setup
	LLPD::gpio_output_setup( GPIO_PORT::A, GPIO_PIN::PIN_0, GPIO_PUPD::NONE, GPIO_OUTPUT_TYPE::PUSH_PULL, GPIO_OUTPUT_SPEED::LOW );
	LLPD::gpio_output_set( GPIO_PORT::A, GPIO_PIN::PIN_0, ledIsOn );

	// spi initialization
	LLPD::spi_master_init( SPI_NUM::SPI_2, SPI_BAUD_RATE::SYSCLK_DIV_BY_256, SPI_CLK_POL::LOW_IDLE, SPI_CLK_PHASE::FIRST,
				SPI_DUPLEX::FULL, SPI_FRAME_FORMAT::MSB_FIRST, SPI_DATA_SIZE::BITS_8 );

	// audio timer start
	LLPD::tim6_counter_start();

	// SRAM setup and test
	LLPD::gpio_output_setup( GPIO_PORT::B, GPIO_PIN::PIN_12, GPIO_PUPD::NONE, GPIO_OUTPUT_TYPE::PUSH_PULL,
				GPIO_OUTPUT_SPEED::HIGH ); // SRAM chip select pin
	LLPD::gpio_output_set( GPIO_PORT::B, GPIO_PIN::PIN_12, true );
	Sram_23K256 sram( SPI_NUM::SPI_2, GPIO_PORT::B, GPIO_PIN::PIN_12 );
	SharedData<uint8_t> sramValsToWrite = SharedData<uint8_t>::MakeSharedData( 3 );
	sramValsToWrite[0] = 25; sramValsToWrite[1] = 16; sramValsToWrite[2] = 8;
	sram.writeToMedia( sramValsToWrite, 45 );
	SharedData<uint8_t> retVals2 = sram.readFromMedia( 3, 45 );
	if ( retVals2[0] == 25 && retVals2[1] == 16 && retVals2[2] == 8 )
	{
		// LLPD::usart_log_int( USART_NUM::USART_3, "SRAM worked! value 0: ", retVals2[0] );
		// LLPD::usart_log_int( USART_NUM::USART_3, "SRAM worked! value 1: ", retVals2[1] );
		// LLPD::usart_log_int( USART_NUM::USART_3, "SRAM worked! value 2: ", retVals2[2] );
	}
	else
	{
		keepBlinking = false;
	}

	while ( true )
	{
	}
}

extern "C" void TIM6_DAC_IRQHandler (void)
{
	if ( ! LLPD::tim6_isr_handle_delay() ) // if not currently in a delay function,...
	{
		if ( keepBlinking )
		{
			if ( ledIncr < ledMax )
			{
				ledIncr++;
			}
			else
			{
				ledIsOn = !ledIsOn;
				LLPD::gpio_output_set( GPIO_PORT::A, GPIO_PIN::PIN_0, ledIsOn );

				ledIncr = 0;
			}
		}
	}

	LLPD::tim6_counter_clear_interrupt_flag();
}
