#include <stddef.h>

int setup_pins();

int setup_pins_read();

void write_buffer_to_EEPROM( char *buffer, size_t dataSize );

void scuffed_read( size_t dataSize );
