#ifndef PORTS_H
#define PORTS_H

#define PADSIO_DATA(x)      *((unsigned char*)(0x1f801040 + (x<<4)))
#define PADSIO_STATUS(x)    *((unsigned short*)(0x1f801044 + (x<<4)))
#define PADSIO_MODE(x)      *((unsigned short*)(0x1f801048 + (x<<4)))
#define PADSIO_CTRL(x)      *((unsigned short*)(0x1f80104a + (x<<4)))
#define PADSIO_BAUD(x)      *((unsigned short*)(0x1f80104e + (x<<4)))

void InitPad();
void BusyLoop(int count);

#endif