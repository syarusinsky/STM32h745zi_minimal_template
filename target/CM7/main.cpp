#include "../../lib/STM32h745zi-HAL/llpd/include/LLPD.hpp"

#include "SRAM_23K256.hpp"

const int SYS_CLOCK_FREQUENCY = 480000000;

volatile bool keepBlinking = true;
volatile bool ledIsOn = false;
volatile int ledIncr = 0; // should flash led every time this value is equal to ledMax
volatile int ledMax = 20000;

int main(void)
{
	// setup clock
	LLPD::rcc_clock_start_max_cpu1();

	// enable gpio clocks
	LLPD::gpio_enable_clock( GPIO_PORT::A );
	LLPD::gpio_enable_clock( GPIO_PORT::B );
	LLPD::gpio_enable_clock( GPIO_PORT::C );
	LLPD::gpio_enable_clock( GPIO_PORT::D );
	LLPD::gpio_enable_clock( GPIO_PORT::E );
	LLPD::gpio_enable_clock( GPIO_PORT::F );
	LLPD::gpio_enable_clock( GPIO_PORT::G );
	LLPD::gpio_enable_clock( GPIO_PORT::H );

	// USART setup
	LLPD::usart_init( USART_NUM::USART_1, USART_WORD_LENGTH::BITS_8, USART_PARITY::NONE, USART_CONF::TX_AND_RX,
				USART_STOP_BITS::BITS_1, 120000000, 9600 );
	LLPD::usart_log( USART_NUM::USART_1, "Gen_MAX_FX_SYN starting up ----------------------------" );

	// audio timer setup (for 40 kHz sampling rate at 480 MHz timer clock)
	LLPD::tim6_counter_setup( 0, 6000, 40000 );
	LLPD::tim6_counter_enable_interrupts();
	LLPD::usart_log( USART_NUM::USART_1, "tim6 initialized..." );

	// test led setup
	LLPD::gpio_output_setup( GPIO_PORT::A, GPIO_PIN::PIN_0, GPIO_PUPD::NONE, GPIO_OUTPUT_TYPE::PUSH_PULL, GPIO_OUTPUT_SPEED::LOW );
	LLPD::gpio_output_set( GPIO_PORT::A, GPIO_PIN::PIN_0, ledIsOn );

	// spi initialization
	LLPD::spi_master_init( SPI_NUM::SPI_2, SPI_BAUD_RATE::SYSCLK_DIV_BY_256, SPI_CLK_POL::LOW_IDLE, SPI_CLK_PHASE::FIRST,
				SPI_DUPLEX::FULL, SPI_FRAME_FORMAT::MSB_FIRST, SPI_DATA_SIZE::BITS_8 );
	LLPD::usart_log( USART_NUM::USART_3, "sram initialized..." );

	// SRAM setup and test
	LLPD::gpio_output_setup( GPIO_PORT::B, GPIO_PIN::PIN_12, GPIO_PUPD::NONE, GPIO_OUTPUT_TYPE::PUSH_PULL,
				GPIO_OUTPUT_SPEED::HIGH ); // SRAM chip select pin
	LLPD::gpio_output_set( GPIO_PORT::B, GPIO_PIN::PIN_12, true );
	Sram_23K256 sram( SPI_NUM::SPI_2, GPIO_PORT::B, GPIO_PIN::PIN_12 );
	SharedData<uint8_t> sramValsToWrite = SharedData<uint8_t>::MakeSharedData( 3 );
	sramValsToWrite[0] = 25; sramValsToWrite[1] = 16; sramValsToWrite[2] = 8;
	sram.writeToMedia( sramValsToWrite, 45 );
	SharedData<uint8_t> retVals = sram.readFromMedia( 3, 45 );
	if ( retVals[0] == 25 && retVals[1] == 16 && retVals[2] == 8 )
	{
		LLPD::usart_log_int( USART_NUM::USART_3, "SRAM worked! value 0: ", retVals[0] );
		LLPD::usart_log_int( USART_NUM::USART_3, "SRAM worked! value 1: ", retVals[1] );
		LLPD::usart_log_int( USART_NUM::USART_3, "SRAM worked! value 2: ", retVals[2] );
	}
	else
	{
		LLPD::usart_log_int( USART_NUM::USART_3, "SRAM failed! value 0: ", retVals[0] );
		LLPD::usart_log_int( USART_NUM::USART_3, "SRAM failed! value 1: ", retVals[1] );
		LLPD::usart_log_int( USART_NUM::USART_3, "SRAM failed! value 2: ", retVals[2] );
	}

	// audio timer start
	LLPD::tim6_counter_start();
	LLPD::usart_log( USART_NUM::USART_1, "tim6 started..." );

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

extern "C" void USART1_IRQHandler (void)
{
	// loopback test code for usart recieve
	uint16_t data = LLPD::usart_receive( USART_NUM::USART_1 );
	LLPD::usart_transmit( USART_NUM::USART_1, data );
}
