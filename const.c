#include "const.h"

const int TIMEOUT = 0x1;
const int UNKNOWNCHAR = 0x2;
const int RECIEVERCAN = 0x3;
const int TENERRORS = 0x4;
const int TRANSMITERCAN = 0x5;
const int WRONGCOMPLEMENT = 0x6;
const int WRONGCRC = 0x7;
const int WRONGPACKETNUMBER = 0x8;
const int PACKETDUPLICATE = 0x9;
const int OTHER = 0x10;

const char SOH = 0x01;
const char STX = 0x02;
const char EOT = 0x04;
const char ACK = 0x06;
const char NAK = 0x15;
const char ETB = 0x17;
const char CAN = 0x18;
const char C = 0x43;
