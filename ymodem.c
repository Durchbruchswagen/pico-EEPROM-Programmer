/* YMODEM implementation for Raspberry Pi Pico
 */

#include "xmodem.h"
#include "pico/stdlib.h"
#include <stdint.h>
#include <stdio.h>
#include "logger.h"
#include "const.h"

const size_t YMODEMDATASIZE_SMALL = 128;
const size_t YMODEMDATASIZE_LARGE = 1024;

const int POLYNOMIAL = 0x1021;

char payloadBuffer[2 + 1024 + 2];
char messageBuffer[2024];

uint32_t compute_crc16( char* dataPtr, size_t dataSize )
{
	uint32_t crc = 0;
	while( dataSize > 0 )
	{
		crc = crc ^ ( int )*dataPtr++ << 8;

		for( int i = 0; i < 8 ; ++i )
		{
			if( crc & 0x8000 )
			{
				crc = crc << 1 ^ POLYNOMIAL;
			}
			else
			{
				crc = crc << 1;
			}
		}
		dataSize -= 1;
	}

	return ( crc & 0xFFFF );
}

int zero_packet_handler( size_t YmodemPacketSize )
{
	uint32_t crc = 0;
	char nullPathname = 0;
	
	if( (int)payloadBuffer[0] != 0 )
	{
		sprintf( messageBuffer, "File packet number: %d\n", payloadBuffer[0]);
		log_add( messageBuffer );
		putchar( NAK );
		return -WRONGPACKETNUMBER;
	}
		
	if( payloadBuffer[0] != 255 - payloadBuffer[1] )
	{
		log_add( "Wrong complement for file packet 0\n" );
		putchar( NAK );
		return -WRONGCOMPLEMENT;
	}
	
	crc = compute_crc16( payloadBuffer + 2, YmodemPacketSize);
	
	if( (char)( crc >> 8 ) != payloadBuffer[2 + YmodemPacketSize] || (char)crc != payloadBuffer[2 + YmodemPacketSize + 1] )
	{
		sprintf(messageBuffer, "Wrong checksum on packet 0. E:%d, p1:%d, p2:%d\n", crc, payloadBuffer[2 + YmodemPacketSize], payloadBuffer[2 + YmodemPacketSize +1]);
		log_add(messageBuffer);
		putchar( NAK );
		return -WRONGCRC;
	}

	if( payloadBuffer[ 2 ] == 0 )
	{	
		nullPathname = 1;
		for( int i = 0; i < YmodemPacketSize; i++)
		{
			if( payloadBuffer[ 2 + i ] != 0 )
			{
				nullPathname = 0;
				break;
			}
		}
	}
	
	if( nullPathname )
	{
		putchar( ACK );
		return 1;
	}

	log_add( "Sending New File " );
	log_add( payloadBuffer + 2 );
	log_add( "\n" );
	
	putchar( ACK );
	putchar( C );
	return 0;
}

int file_content_packet_handler( char *outputBuffer, size_t YmodemPacketSize, uint8_t expectedPacketNumber )
{
	uint32_t crc = 0;

	if( payloadBuffer[0] != 255 - payloadBuffer[1] )
	{
		log_add( "Wrong complement\n" );
		putchar( NAK );
		return -WRONGCOMPLEMENT;
	}

	if( payloadBuffer[0] != expectedPacketNumber )
	{
		if( payloadBuffer[0] != expectedPacketNumber - 1)
		{
			log_add("DESYNCHRONIZATION!!!\n");
			putchar( CAN );
			putchar( CAN );
			if (getchar_timeout_us( 1000 ) == PICO_ERROR_TIMEOUT) 
			{
				return -RECIEVERCAN; 
			}
			return -RECIEVERCAN;
		}
		putchar( ACK );
		return -PACKETDUPLICATE;
	}

	  
	crc = compute_crc16( payloadBuffer + 2, YmodemPacketSize);
	
	if( (char)( crc >> 8 ) != payloadBuffer[2 + YmodemPacketSize] || (char)crc != payloadBuffer[2 + YmodemPacketSize + 1] )
	{
		sprintf( messageBuffer, "Wrong checksum on packet %d. E:%d, p1:%d, p2:%d\n", expectedPacketNumber, crc, payloadBuffer[2 + YmodemPacketSize], payloadBuffer[2 + YmodemPacketSize +1] );
		log_add( messageBuffer );
		putchar( NAK );
		return -WRONGCRC;
	}
	for( int i = 0; i < YmodemPacketSize; i++ )
	{
		*( outputBuffer + i ) = payloadBuffer[2 + i];
	}
	putchar( ACK );
	return 0;
}

int ymodem_recieve_packets( char *outputBuffer, size_t bufferSize )
{
	absolute_time_t dataTimeoutTime;
	size_t dataSize;
	int rtval = 0;
	int dataTimeouts = 0;
	int recievedBytes = 0;
	int currentByte = 0;
	int errors = 0;
	char expectedZeroPacket = 1;
	char doubleEOT=0;
	char doubleCAN=0;
	char timeout = 0;
	char recievedChar;
	uint8_t recievedPackets = 1;
	while( 1 )
	{
		if( errors >= 10)
		{
			putchar( CAN );
			putchar( CAN );
			log_add("<-CAN\n<-CAN");
			if (getchar_timeout_us( 1000 ) == PICO_ERROR_TIMEOUT) 
			{
				return -RECIEVERCAN; 
			}
			return -RECIEVERCAN;
		}
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
				putchar( C );
				expectedZeroPacket = 1;
				continue;
			}
		}
		
		else if ( recievedChar == CAN )
		{
			log_add( "CAN->\n" );
			
			if ( doubleCAN == 0 )
			{
				doubleCAN = 1;
				putchar( NAK );
				continue;		
			}

			else
			{
				return -TRANSMITERCAN;
			}
		}

		else if ( recievedChar != SOH && recievedChar != STX ) 
		{	
			sprintf ( messageBuffer, "UC: %d \n", recievedChar );
			log_add ( messageBuffer );
			continue;
		}

		dataSize = recievedChar == SOH ? YMODEMDATASIZE_SMALL : YMODEMDATASIZE_LARGE;

		if ( recievedBytes + dataSize > bufferSize )
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
		log_add( messageBuffer );
		log_add( "->SOH\n" );

		currentByte = 0;

		while ( currentByte < 2 + dataSize + 2 )
		{
			if ( absolute_time_diff_us(dataTimeoutTime, get_absolute_time() ) > 0)
			{
				timeout = 1;
				break;
				
			}

			recievedChar = getchar_timeout_us( 1000 );
			if ( recievedChar == PICO_ERROR_TIMEOUT ) continue;
			dataTimeoutTime = make_timeout_time_ms( 10000 );
			payloadBuffer[currentByte] = recievedChar;
			currentByte += 1;

		}
		
		if( timeout )
		{
			log_add( "Timeout \n" );
			errors += 1;
			putchar( NAK );
			continue;
		}
		
		if( expectedZeroPacket )
		{
			rtval = zero_packet_handler( dataSize );
			if ( rtval < 0 )
			{
				errors += 1;
				continue;
			}
			else if ( rtval == 1)
			{
				return recievedBytes;
			}
			else if ( rtval == 0 )
			{
				recievedPackets = 1;
				expectedZeroPacket = 0;
				log_add("<-ACK_0\n");
			}
		}
		else
		{
			rtval = file_content_packet_handler( outputBuffer + recievedBytes, dataSize, recievedPackets );
			if( rtval == -RECIEVERCAN )
			{
				log_add("<-CAN\n<-CAN\n");
				return rtval;
			}
			else if( rtval == -PACKETDUPLICATE )
			{
				log_add("Packet duplicate\n");
				continue;
			}
			else if( rtval == 0)
			{
				recievedBytes += dataSize;
				recievedPackets += 1;
				errors = 0;
				log_add("<-ACK_DATA\n");
			}
			else
			{
				errors += 1;
				continue;
			}
		}
	}
	return -OTHER;
}


int ymodem_wait_for_start_of_transmision( char *outputBuffer, size_t bufferSize )
{
	absolute_time_t nakDelay = get_absolute_time();
	char receivedChar;
	int rtval;
	log_clear();
	while( 1 )
	{
		if ( absolute_time_diff_us( nakDelay, get_absolute_time() ) > 0)
		{
			putchar( C );
			log_add( "Sending C\n" );
			nakDelay = make_timeout_time_ms( 10000 );
		}

		rtval=ymodem_recieve_packets( outputBuffer, bufferSize );
		if ( rtval == -RECIEVERCAN || rtval >= 0 || rtval == -TENERRORS)
		{
			puts("");
			log_dump();
			log_clear();
			printf( "RTVAL:%d\n",rtval );
			return rtval;
		}
		else if ( rtval != -TIMEOUT )
		{
			sprintf( messageBuffer, "rtval: %d\n", rtval );
			log_add( messageBuffer );
		}

	}
	return -1;
}
