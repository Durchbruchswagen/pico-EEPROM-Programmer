/* XMODEM implementation for Raspberry Pi Pico
 *
 *  Most basic XMODEM implementation
 *  No CRC
 *  128 byte payload
 *
 */

#include "xmodem.h"
#include "pico/stdlib.h"
#include <stdint.h>
#include <stdio.h>
#include "logger.h"
#include "const.h"

const int XMODEMDATASIZE = 128;

inline uint8_t calculate_checksum( uint8_t current_sum, uint8_t byte_to_add)
{
	return current_sum + byte_to_add;
}

int recieve_packets( char *outputBuffer, size_t bufferSize )
{
	char messageBuffer [ 1024 ];
	absolute_time_t dataTimeoutTime;
	int dataTimeouts = 0;
	int recievedBytes = 0;
	int recievedPacketNumber = 0;
	int currentByte = 0;
	int errors = 0;
	char doubleEOT = 0;
	char doubleCAN = 0;
	char timeout = 0;
	char wrongPacketNumber = 0;
	char wrongChecksum = 0;
	char wrongComplement = 0;
	char recievedChar;
	uint8_t checksum = 0;
	uint8_t recievedPackets = 1;
	while( 1 )
	{
		recievedChar = getchar_timeout_us( 1000 );

		if( recievedChar == ( char )PICO_ERROR_TIMEOUT ) {
			return -TIMEOUT;
		}
		
		else if ( recievedChar == EOT )
		{
			log_add( "EOT->\n" );
			if ( doubleEOT == 0 )
			{
				log_add( "<-NAK\n" );
				putchar( NAK );
				doubleEOT = 1;
				continue;
			}
		
			else
			{
				putchar( ACK );
				return recievedBytes;	
			}
		}
		
		else if ( recievedChar == CAN )
		{
			log_add( "CAN->\n" );
			
			if ( doubleCAN == 0 )
			{
				doubleCAN = 1;
				continue;		
			}

			else
			{
				return -TRANSMITERCAN;
			}
		}

		else if ( recievedChar != SOH) 
		{	
			sprintf ( messageBuffer, "UC: %d \n", recievedChar );
			log_add ( messageBuffer );
			continue;
		}

		else if ( recievedBytes + XMODEMDATASIZE > bufferSize )
		{
			while( 1 )
			{
				putchar( CAN );
				putchar( CAN );
				if (getchar_timeout_us( 1000 ) == PICO_ERROR_TIMEOUT) 
				{
					return -RECIEVERCAN; 
				}
			}
		}
		
		doubleEOT = 0;
		doubleCAN = 0;

		sprintf( messageBuffer, "Packet number: %d\n", recievedPackets );
		log_add( messageBuffer);
		log_add( "->SOH\n" );

		currentByte = 0;
		wrongPacketNumber = 0;
		wrongChecksum = 0;
		wrongComplement = 0;
		checksum = 0;
		recievedPacketNumber = 0;
		errors = 0;	
		dataTimeoutTime = make_timeout_time_ms( 10000 );

		while ( currentByte < 2 + XMODEMDATASIZE + 1 )
		{
			if ( absolute_time_diff_us(dataTimeoutTime, get_absolute_time() ) > 0)
			{
				timeout = 1;
				break;
				
			}

			recievedChar = getchar_timeout_us( 1000 );
			if ( recievedChar == PICO_ERROR_TIMEOUT ) continue;

			if( currentByte == 0 )
			{
				recievedPacketNumber = recievedChar;
				sprintf( messageBuffer, "pn: %d\n", recievedChar);
				log_add( messageBuffer );
				wrongPacketNumber = recievedChar != recievedPackets;
			}
			else if( currentByte == 1 )
			{
				wrongPacketNumber = recievedChar != ( ( uint8_t )( 255 - recievedPackets ) | wrongPacketNumber );
				wrongComplement = recievedChar != ( uint8_t )( 255 - recievedPacketNumber );
			}

			else if( currentByte == 2 + XMODEMDATASIZE )
			{
				wrongChecksum = checksum != recievedChar;
				sprintf( messageBuffer, "Checksum: %d, %d\n",recievedChar, checksum);
				log_add( messageBuffer );
			}
			else
			{
				outputBuffer[ recievedBytes + currentByte - 2 ] = recievedChar;
				checksum = calculate_checksum( checksum, recievedChar );
			}

			dataTimeoutTime = make_timeout_time_ms( 10000 );
			currentByte += 1;

		}
		
		if( ( wrongComplement || timeout || wrongChecksum ) && errors == 9 )
		{
			return -TENERRORS;			
		}
		if( wrongComplement )
		{
			log_add( "Wrong complement\n ");
			errors += 1;
			putchar( NAK );
			continue;

		}

		if( wrongPacketNumber )
		{
			log_add( "Wrong packet number\n" );
			if( recievedPacketNumber == recievedPackets - 1 )
			{
				log_add( "<-ACK_P\n" );
				putchar( ACK );
				continue;
			}
			else
			{
				putchar( CAN );
				putchar( CAN );
				return -RECIEVERCAN;
			}

		}


		if( timeout )
		{
			log_add( "Timeout \n" );
			errors += 1;
			putchar( NAK );
			continue;
		}

		if( wrongChecksum )
		{
			log_add(  "Wrong checksum \n" );
			errors += 1;
			putchar( NAK );
			continue;
		}



		recievedBytes += XMODEMDATASIZE; 
		recievedPackets += 1;
		log_add("<-ACK_DATA_PACKET\n");
		putchar( ACK );
	}
	return -OTHER;
}


int xmodem_wait_for_start_of_transmision( char *outputBuffer, size_t bufferSize )
{
	absolute_time_t nakDelay = get_absolute_time();
	char messageBuffer[ 1024 ];
	char receivedChar;
	int rtval;
	log_clear();
	while( 1 )
	{
		if ( absolute_time_diff_us( nakDelay, get_absolute_time() ) > 0)
		{
			putchar( NAK );
			log_add( "Sending Nak\n" );
			nakDelay = make_timeout_time_ms( 10000 );
		}

		rtval=recieve_packets( outputBuffer, bufferSize );
		if ( rtval == -RECIEVERCAN || rtval >= 0 || rtval == -TENERRORS)
		{
			puts("");
			log_dump();
			log_clear();
			printf( "RTVAL:%d\n",rtval );
			return rtval;
		}
		else if (rtval != -TIMEOUT )
		{
			sprintf( messageBuffer, "rtval: %d\n", rtval );
			log_add( messageBuffer );
		}

	}
	return -1;
}
