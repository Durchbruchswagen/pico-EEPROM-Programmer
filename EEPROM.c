#include <stdio.h>
#include <stdint.h>
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include "EEPROM.h"

/* EEPROM PARAMETERS AND GPIO MAPPING
 * NAME: AT28C64B
 * SIZE: 64K ( 8192 x 8 )
 * PAGE SIZE: 64 bytes
 * WRITE CYCLE TIME: 10ms
 * WRITE PULSE TIME: 100 ns
 * PINS MAPPING
 * GPIOs 0-12 are mapped to A0-A12
 * GPIOS 13-20 are mapped to I/O0-I/O7
 * GPIO 21 is mappedd to WE
 */

const int writeCycleDelay = 10;
const int writePulseDelay = 100;
const uint32_t gpioAllPinMask = 0x3fffff;
const uint32_t gpioReadOutMask = 0x201fff;
const uint32_t gpioReadInMask = 0x1fe000;

int setup_pins()
{
	printf( "Setup start \n");
	gpio_init_mask( gpioAllPinMask );
	gpio_set_dir_out_masked( gpioAllPinMask );
	gpio_put_masked( gpioAllPinMask, gpioAllPinMask );
	printf( "Setup complete\n" );
	return 0;
}


int setup_pins_read()
{
	printf( "Setup start \n");
	gpio_init_mask( gpioAllPinMask );
	gpio_set_dir_out_masked( gpioReadOutMask );
	gpio_put_masked( gpioReadOutMask, gpioReadOutMask );
	gpio_set_dir_in_masked( gpioReadInMask );
	printf( "Setup complete\n" );
	return 0;
}

uint32_t create_value_mask( uint16_t address, uint8_t data, uint8_t writeEnable )
{
	uint32_t rtval = ( (uint32_t)writeEnable << 21 ) | ( (uint32_t)data << 13 )  | (address & 0x1fff);
	return rtval;
}

void write_byte_to_EEPROM( uint16_t address, uint8_t data )
{
	printf( "Writing %d to %d\n", data, address );

	uint32_t bitsWriteEnableLow = create_value_mask( address, data, 0 );
	uint32_t bitsWriteEnableHigh = create_value_mask( address, data, 1 );
	
	gpio_put_masked( gpioAllPinMask, bitsWriteEnableHigh );
	sleep_us( writePulseDelay );
	gpio_put_masked( gpioAllPinMask, bitsWriteEnableLow );
	sleep_us( writePulseDelay );
	gpio_put_masked( gpioAllPinMask, bitsWriteEnableHigh );
	sleep_ms( writeCycleDelay );
	
	return;
}

void scuffed_read( size_t dataSize)
{
	printf("PIN 13:%d, pin14%d\n", gpio_is_dir_out( 13 ), gpio_is_dir_out(14));
	for( int i = 0; i < dataSize; i++)
	{
		printf("READING ADDRES :%d\n", i);
		gpio_put_masked( gpioReadOutMask, ( i & 0x1fff ));
		sleep_us( 250 );
		printf(" I/O 0:%d I/O 1: %d\n", gpio_get(13),gpio_get(14));
	}
	return ;
}

inline void disable_software_protection()
{
	printf( "Disableing software protection\n" );

	write_byte_to_EEPROM( 0x1555, 0xaa );
	write_byte_to_EEPROM( 0x0aaa, 0x55 );
	write_byte_to_EEPROM( 0x1555, 0x80 );
	write_byte_to_EEPROM( 0x1555, 0xaa );
	write_byte_to_EEPROM( 0x0aaa, 0x55 );
	write_byte_to_EEPROM( 0x1555, 0x20 );
	
	return;
}
	
void write_buffer_to_EEPROM( char *buffer, size_t dataSize )
{
	size_t sizeToWrite = dataSize > 8192 ? 8192 : dataSize;
	disable_software_protection();
	
	printf( "Writing %zu bytes of data to EEPROM\n" ,sizeToWrite );

	for ( uint16_t address = 0; address < sizeToWrite; address++ )
	{
		write_byte_to_EEPROM( address, buffer[address] );
	}
	
	return;
}	
