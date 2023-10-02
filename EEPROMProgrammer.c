#include <stdio.h>
#include "pico/stdlib.h"
#include "xmodem.h"
#include "ymodem.h"
#include "EEPROM.h"
#include <stddef.h>
#include "tusb.h"

int main()
{
	size_t bufferSize = 65536;
	char buffer[ bufferSize ];
	int rtval = 0;
	char protocol;
	setup_pins();
	stdio_init_all();
	while( 1 )
	{
		while( !tud_cdc_connected())
		{
			sleep_ms( 100 );
		}
		printf( "SELECT PROTOCOL \n x - Xmodem\n y - Ymoedem\n" );
		while( 1 )
		{
			protocol = getchar_timeout_us( 1000 );
			if ( protocol == 'x' || protocol == 'y' ) break;	
		}
		printf( "START\n" );
		if( protocol == 'y')
		{
			printf("YMODEM\n");
			rtval = ymodem_wait_for_start_of_transmision( buffer, bufferSize );
		}
		if( protocol == 'x')
		{
			printf("XMODEM\n");
			rtval = xmodem_wait_for_start_of_transmision( buffer, bufferSize );
		}
		if ( rtval <= 0 ) continue;
		write_buffer_to_EEPROM( buffer, rtval );
		printf( "END\n" );	
	}
}
