/*
 * PS1CardLink PlayStation Memory Card reader (based on MemCARDuino)
 * Shendo 2014-2023
 */

#include <psx.h>
#include <stdio.h>
#include <memcard.h>
#include <string.h>

/*Font related*/
#include "include/fontspace.h"
#include "include/font.h"

/*Images*/
#include "include/bluecard.h"

#include "include/ports.h"

/*Application info*/
#define SOFTWARE_TITLE		"PS1CardLink 1.1\n2023-01-16"

//#define DEBUG

/*PS1CardLink software version (Major.Minor)*/
#define VERSION 0x11

/*Commands*/
#define GETID	0xA0		/*Get identifier*/
#define GETVER	0xA1		/*Get firmware version*/
#define MCREAD	0xA2		/*Memory Card Read (frame)*/
#define MCWRITE	0xA3		/*Memory Card Write (frame)*/
#define MCPORT	0xA4		/*Set Memory Card port*/

/*Responses*/
#define ERROR 0xE0         /*Invalid command received (error)*/

/*Memory Card Responses*/
//0x47 - Good
//0x4E - BadChecksum
//0xFF - BadSector

unsigned int PrimList[0x4000];

int i, j;

/*Global variables*/
GsImage TempImage;
GsSprite CardSprite;
GsRectangle TopRect;

unsigned char ReadByte = 0;

char *ActiveString;
char PrintedString[100];
char CounterMode = 0;		/*0 - Reading Pad, 1 - Reading Memory Card*/
char ActiveSlot = 0;			/*Active Memory Card port*/

unsigned short Pad1Buffer = 0;

/*128 byte frame data*/
unsigned char CardFrame[128];
unsigned char FrameMSB = 0;
unsigned char FrameLSB = 0;
unsigned char RwResult = 0;
unsigned char XorData = 0;

/*Memory Card Formatted status*/
int CardFormatted = 0;

/*Print a string at the specified coordinates in color*/
void GsPrintString(int x, int y, char Red, char Green, char Blue, char *string)
{
	GsSprite CharSprite;
	char CharOffset;

	CharSprite.x = x;
	CharSprite.y = y;
	CharSprite.w = 8;
	CharSprite.h = 8;
	CharSprite.r = Red;
	CharSprite.g = Green;
	CharSprite.b = Blue;
	CharSprite.cx = 320;
	CharSprite.cy = 240;
	CharSprite.tpage = 5;
	CharSprite.attribute = COLORMODE(COLORMODE_8BPP);

	while(*string)
	{
		/*Check if this is a printable character*/
		if(*string >= 0x20 && *string <= 0x7F)
		{
			/*Get char offset*/
			CharOffset = *string - 0x20;

			CharSprite.u = (CharOffset%0x20) * 8;
			CharSprite.v = (CharOffset/0x20) * 8;

			/*Place sprite in the drawing list*/
			GsSortSimpleSprite(&CharSprite);
			
			/*Increase X offset*/
			CharSprite.x += (FontSpace[CharOffset] + 1);
		}
		
		/*Check if this is a newline character*/
		if(*string == '\n')
		{
			string++;
			
			CharSprite.x = x;
			CharSprite.y += 10;
		}
		else string++;
	}
}

/*Critical system error message*/
void SysError(char *message)
{
	GsSortCls(0, 0, 174);							/*BSOD*/
	
	GsPrintString(16, 16, 128, 128, 128, "Critical system error.");
	GsPrintString(16, 36, 128, 128, 128, message);
	GsPrintString(16, 66, 128, 128, 128, "Machine reboot required.");
	
	GsDrawList();									/*Draw primitives from the list*/
	while(GsIsDrawing());						/*Wait for GPU to finish drawing*/
	
	/*Endless loop*/
	while(1);
}


/*Initialise blue card sprite*/
void InitSelectorSprite()
{
	CardSprite.x = 220;
	CardSprite.y = 140;
	CardSprite.w = 84;
	CardSprite.h = 84;
	CardSprite.u = 0;
	CardSprite.v = 24;
	CardSprite.r = 128;
	CardSprite.g = 128;
	CardSprite.b = 128;
	CardSprite.cx = 320;
	CardSprite.cy = 241;
	CardSprite.tpage = 5;
	CardSprite.attribute = COLORMODE(COLORMODE_8BPP);
}

/*GUI of the application*/
void UpdateStatus(char *string)
{
	/*If status is already set stop updating*/
	if(strcmp(ActiveString, string) == 0)return;
	
	/*Set active string for the next loop*/
	ActiveString = string;
	
	//GsSortCls(48, 48, 48);							/*Clear screen*/

	GsSortCls(0, 0, 0);							/*Clear screen*/
	
	/*Draw top rectangle*/
	TopRect.x = 0;
	TopRect.y = 0;
	TopRect.w = 320;
	TopRect.h = 40;
	TopRect.r = 0;
	TopRect.g = 76;
	TopRect.b = 163;
	
	GsSortRectangle(&TopRect);
	
	GsSortSimpleSprite(&CardSprite);
		
	GsPrintString(16, 16, 128, 128, 128, SOFTWARE_TITLE);
	
	GsPrintString(16, 155, 128, 128, 128, 	"Coded by Shendo\n"
															"PSXSDK by Tails92\n\n"
															"Beta testers: Alex.d93,\n"
															"Carmax91, Danhans42,\n"
															"PsxFan107, Spipis\nand Type 79");
	
	sprintf(PrintedString, "Status: %s\nActive slot: < %d >", string, ActiveSlot + 1);
	
	GsPrintString(16, 55, 128, 128, 128, PrintedString);
	
	GsDrawList();									/*Draw primitives from the list*/
}

/*All SIO logi*/
void HandleSIO()
{
	/*Check if there is any data waiting in SIO buffer*/
	if (!SIOCheckInBuffer())
	{
		/*Check if pad reading mode or timeout mode is active*/
		if(CounterMode == 1)
		{
			if (GetRCnt(RCntCNT1) > 14400)
			{
				UpdateStatus("Waiting for PC");
				CounterMode = 0;
			}
		}
		else
		{
			/*Read gamepad every 240 HBlanks*/
			if (GetRCnt(RCntCNT1) > 240)
			{
				PSX_ReadPad(&Pad1Buffer, NULL);
				SetRCnt(RCntCNT1, 0, RCntNotar);
				
				/*Check if left is pressed*/
				if((Pad1Buffer & PAD_LEFT) && ActiveSlot == 1)
				{
					ActiveString = NULL;
					ActiveSlot = 0;
					UpdateStatus("Waiting for PC");
				}
				
				/*Check if right is pressed*/
				if((Pad1Buffer & PAD_RIGHT) && ActiveSlot == 0)
				{
					ActiveString = NULL;
					ActiveSlot = 1;
					UpdateStatus("Waiting for PC");
				}
			}
		}
		
		return;
	}
	
	ReadByte = SIOReadByte();
	
	SetRCnt(RCntCNT1, 0, RCntNotar);
	CounterMode = 1;
					
	switch(ReadByte)
	{
		default:
			while(!SIOCheckOutBuffer());
			SIOSendByte(ERROR);
				break;
        
			case GETID:
				while(!SIOCheckOutBuffer()); SIOSendByte('P');
				while(!SIOCheckOutBuffer()); SIOSendByte('S');
				while(!SIOCheckOutBuffer()); SIOSendByte('1');
				while(!SIOCheckOutBuffer()); SIOSendByte('C');
				while(!SIOCheckOutBuffer()); SIOSendByte('L');
				while(!SIOCheckOutBuffer()); SIOSendByte('N');
				while(!SIOCheckOutBuffer()); SIOSendByte('K');
					break;
        
				case GETVER:
				while(!SIOCheckOutBuffer());
				SIOSendByte(VERSION);
					break;
        
				case MCPORT:
					while(!SIOCheckInBuffer());
					ActiveSlot = SIOReadByte();
					if(ActiveSlot > 1) ActiveSlot == 1;
					break;

				case MCREAD:
				
					RwResult = 0;
					FrameMSB = 0;
					FrameLSB = 0;
					XorData = 0;
					memset(&CardFrame[0], 0, 128);
				
					/*Get frame index to read*/
					while(!SIOCheckInBuffer());
					FrameMSB = SIOReadByte();
				
					while(!SIOCheckInBuffer());
					FrameLSB = SIOReadByte();
				
					/*Read 128 byte sector from a Memory Card*/
					RwResult = McReadSector(ActiveSlot, FrameLSB | (FrameMSB << 8), CardFrame);
				
					XorData = FrameMSB ^ FrameLSB;
				
					/*Send read data to serial port*/
					for(i = 0; i < 128; i++)
					{
						while(!SIOCheckOutBuffer()); SIOSendByte(CardFrame[i]);
						
						/*Calculate XOR checksum*/
						XorData ^= CardFrame[i];
					}
				
					/*Send XOR checksum*/
					while(!SIOCheckOutBuffer()); SIOSendByte(XorData);
				
					/*Send RW status byte*/
					while(!SIOCheckOutBuffer()); SIOSendByte(RwResult);
					
					UpdateStatus("Reading Memory Card");
					SetRCnt(RCntCNT1, 0, RCntNotar);
					CounterMode = 1;
					
					break;
        
				case MCWRITE:
				
					RwResult = 0;
					FrameMSB = 0;
					FrameLSB = 0;
					XorData = 0;
					memset(&CardFrame[0], 0, 128);
				
					/*Get frame index to write*/
					while(!SIOCheckInBuffer());
					FrameMSB = SIOReadByte();
				
					while(!SIOCheckInBuffer());
					FrameLSB = SIOReadByte();

					XorData = FrameMSB ^ FrameLSB;
										
					/*Read 128 byte data from a serial port*/
					for(i = 0; i < 128; i++)
					{
						while(!SIOCheckInBuffer());
						CardFrame[i] = SIOReadByte();
						
						XorData ^= CardFrame[i];
					}
					
					/*Check if data transferred correctly*/
					while(!SIOCheckInBuffer());					
					
					if(SIOReadByte() == XorData)
					{
						/*Write 128 byte sector to Memory Card*/
						RwResult = McWriteSector(ActiveSlot, FrameLSB | (FrameMSB << 8), CardFrame);
					}
					else
					{
						/*Bad checksum, transfer error*/
						RwResult = 0x4E;
					}
					
					/*Send RW status byte*/
					while(!SIOCheckOutBuffer()); SIOSendByte(RwResult);
					
					UpdateStatus("Writing Memory Card");
					SetRCnt(RCntCNT1, 0, RCntNotar);
					CounterMode = 1;
					
					break;

#ifdef DEBUG
					/*Return to loader (unirom) if 'x' is sent trough serial port*/
					case 'x':
						__asm__("j 0x801B0000");
					break;
#endif
	}
}

int main()
{
	GsInit();					/*Init GPU*/
	GsSetList(PrimList);
	GsClearMem();
	
	/*Set video mode based on the console's region*/
	if(*(char *)0xbfc7ff52 == 'E')	GsSetVideoMode(320, 240, VMODE_PAL);
	else GsSetVideoMode(320, 240, VMODE_NTSC);
		
	/*Load a custom font and upload it to VRAM*/
	GsImageFromTim(&TempImage, FontTimData);
	GsUploadImage(&TempImage);
	
	/*Load Blue Memory Card image and load it to VRAM*/
	GsImageFromTim(&TempImage, BlueCard);
	GsUploadImage(&TempImage);
	InitSelectorSprite();
	
	InitPad();

	/*Start SIO communication*/
	SIOStart(115200);

	GsSetDispEnvSimple(0, 0);
	GsSetDrawEnvSimple(0, 0, 320, 240);
	
	/*Set up HBlank counter*/
	if(!SetRCnt(RCntCNT1, 0, RCntNotar))SysError("HBlank root counter unavailable\n0x000001");
	
	UpdateStatus("Waiting for PC");
	
	while(1)
	{
		HandleSIO();
	}
	
	return 0;
}
