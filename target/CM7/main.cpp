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
	LLPD::gpio_output_setup( GPIO_PORT::A, GPIO_PIN::PIN_1, GPIO_PUPD::NONE, GPIO_OUTPUT_TYPE::PUSH_PULL, GPIO_OUTPUT_SPEED::LOW );
	LLPD::gpio_output_set( GPIO_PORT::A, GPIO_PIN::PIN_1, ledIsOn );

	// spi initialization
	LLPD::spi_master_init( SPI_NUM::SPI_2, SPI_BAUD_RATE::SYSCLK_DIV_BY_256, SPI_CLK_POL::LOW_IDLE, SPI_CLK_PHASE::FIRST,
				SPI_DUPLEX::FULL, SPI_FRAME_FORMAT::MSB_FIRST, SPI_DATA_SIZE::BITS_8 );
	LLPD::usart_log( USART_NUM::USART_3, "sram initialized..." );

	// i2c initialization
	LLPD::i2c_master_setup( I2C_NUM::I2C_1, 0x308075AE );
	LLPD::usart_log( USART_NUM::USART_3, "i2c initialized..." );

	// sram cs setup
	LLPD::gpio_output_setup( GPIO_PORT::B, GPIO_PIN::PIN_12, GPIO_PUPD::PULL_UP, GPIO_OUTPUT_TYPE::PUSH_PULL,
				GPIO_OUTPUT_SPEED::HIGH ); // SRAM chip select pin
	LLPD::gpio_output_set( GPIO_PORT::B, GPIO_PIN::PIN_12, true );

	// audio timer start
	LLPD::tim6_counter_start();
	LLPD::usart_log( USART_NUM::USART_1, "tim6 started..." );

	// quick spi test
	LLPD::tim6_delay( 10000 ); // TODO arbitrary delay since if we immediately use the sram it can be wrong
	LLPD::gpio_output_set( GPIO_PORT::B, GPIO_PIN::PIN_12, false );
	LLPD::spi_master_send_and_recieve( SPI_NUM::SPI_2, 0b00000010 );
	LLPD::spi_master_send_and_recieve( SPI_NUM::SPI_2, 0b00000000 );
	LLPD::spi_master_send_and_recieve( SPI_NUM::SPI_2, 0b00000000 );
	LLPD::spi_master_send_and_recieve( SPI_NUM::SPI_2, 45 );
	LLPD::gpio_output_set( GPIO_PORT::B, GPIO_PIN::PIN_12, true );

	LLPD::gpio_output_set( GPIO_PORT::B, GPIO_PIN::PIN_12, false );
	LLPD::spi_master_send_and_recieve( SPI_NUM::SPI_2, 0b00000011 );
	LLPD::spi_master_send_and_recieve( SPI_NUM::SPI_2, 0b00000000 );
	LLPD::spi_master_send_and_recieve( SPI_NUM::SPI_2, 0b00000000 );
	uint8_t spiRetVal = LLPD::spi_master_send_and_recieve( SPI_NUM::SPI_2, 0b00000000 );
	LLPD::gpio_output_set( GPIO_PORT::B, GPIO_PIN::PIN_12, true );
	if ( spiRetVal == 45 )
	{
		LLPD::usart_log( USART_NUM::USART_1, "sram verified..." );
	}
	else
	{
		LLPD::usart_log( USART_NUM::USART_1, "unable to verify sram..." );
	}

	// quick i2c test
	LLPD::i2c_master_set_slave_address( I2C_NUM::I2C_1, I2C_ADDR_MODE::BITS_7, 0b1010000 );
	LLPD::i2c_master_write( I2C_NUM::I2C_1, true, 3, 0, 0, 24 );
	LLPD::i2c_master_write( I2C_NUM::I2C_1, false, 2, 0, 0 );
	uint8_t i2cRetVal = 0;
	LLPD::i2c_master_read( I2C_NUM::I2C_1, true, 1, &i2cRetVal );
	if ( i2cRetVal == 24 )
	{
		LLPD::usart_log( USART_NUM::USART_1, "eeprom verified..." );
	}
	else
	{
		LLPD::usart_log( USART_NUM::USART_1, "unable to verify eeprom..." );
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
				LLPD::gpio_output_set( GPIO_PORT::A, GPIO_PIN::PIN_1, ledIsOn );

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
