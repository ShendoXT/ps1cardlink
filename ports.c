#include <psx.h>
#include <stdio.h>
#include <string.h>
#include "include/ports.h"

static unsigned char card_cmd[140];
static unsigned char arr[140];

void InitPad(){
	PADSIO_CTRL(0) = 0x40;
	PADSIO_BAUD(0) = 0x88;
	PADSIO_MODE(0) = 13;
	PADSIO_CTRL(0) = 0;
	BusyLoop(10);
	PADSIO_CTRL(0) = 2;
	BusyLoop(10);
	PADSIO_CTRL(0) = 0x2002;
	BusyLoop(10);
	PADSIO_CTRL(0) = 0;
}

void BusyLoop(int count){
	volatile int cycles = count;
	while (cycles--);
}
