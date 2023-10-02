#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include "logger.h"

size_t LOGBUFFERSIZE = 65536;
size_t logPosition;
char logBuffer[ 65536 ];


int copy_to_log_secure(char *destBuffer, char *srcBuffer, size_t srcBufferSize)
{
	size_t pos = 0;

	if ( strlen(srcBuffer) < srcBufferSize )
	{
		return -1;
	}

	if ( logPosition + srcBufferSize + 1 >= LOGBUFFERSIZE)
	{
		return -1;
	}

	for ( ; pos < srcBufferSize ; pos++ )
	{
		destBuffer[ pos ] = srcBuffer[ pos ];
	}
	destBuffer[ pos ] = 0;
	return pos;
}

void log_clear()
{
	logPosition = 0;
	logBuffer[ 0 ] = 0;
}

int log_add(char *message)
{
	size_t message_size = strlen( message );
	int rtval;

	if ( logPosition + message_size + 1 >= LOGBUFFERSIZE)
	{
		return -1;
	}

	rtval = copy_to_log_secure( logBuffer + logPosition, message, message_size );
	
	if ( rtval < 0 )
	{
		return rtval;
	}

	logPosition += rtval;
	return 0;
}

void log_dump()
{
	puts( logBuffer );
}
