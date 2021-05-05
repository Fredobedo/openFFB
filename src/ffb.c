#include "ffb.h"
#include "config.h"
#include "debug.h"
#include "device.h"
#include <time.h>
#include "sdlhelper.h"

/* The in packet used to read from Sega FFB Controller */
FFBPacket inputPacket;
unsigned char rawPacket[6];

int initFFB(char *devicePath)
{
	/* Init the connection to the Aganyte's Sega FFB Controller */
	if (!initDevice(devicePath))
		return 0;

	return 1;
}

int disconnectFFB()
{
	return closeDevice();
}

FFBStatus readPacket()
{
	int bytesRead = readBytes(rawPacket, sizeof(FFBPacket));
	if (bytesRead < 0)
		return FFB_STATUS_ERROR_TIMEOUT;
	else if (bytesRead < sizeof(FFBPacket))
		return FFB_STATUS_ERROR;


	return processPacket(rawPacket);
}

FFBStatus processPacket(unsigned char* packet)
{
	memcpy(&inputPacket, &packet, sizeof(inputPacket));

	/* --- (D1^D2^D3^D4)&0x7F --- */
	unsigned char checksum = (inputPacket.spring ^ inputPacket.friction ^ inputPacket.torqueDirection ^ inputPacket.torquePower) & 0x7F;
	if(checksum!=inputPacket.crc)
		return FFB_STATUS_ERROR_CHECKSUM;

	/* --- spring            from 0x00 to 0x7F -> 128 levels --- */
	TriggerSpringEffect(((double)inputPacket.spring) / 128);
    /* --- friction          from 0x00 to 0x7F -> 128 levels --- */
	TriggerFrictionEffect(((double)inputPacket.friction) / 128);
    /* --- torqueDirection   0x00 = Left, 0x01  = Right      --- */
    /* --- torquePower       from 0x00 to 0xFF -> 128 levels --- */
	TriggerConstantEffect(inputPacket.torqueDirection, ((double)inputPacket.torquePower) / 128);
 
	return FFB_STATUS_SUCCESS;
}
