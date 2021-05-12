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
	int bytesRead = readBytes(rawPacket, 6);
	if (bytesRead < 0)
		return FFB_STATUS_ERROR_TIMEOUT;
	else if (bytesRead < 6)
		return FFB_STATUS_ERROR;


	return processPacket(rawPacket);
}

FFBStatus processPacket(unsigned char* packet)
{
	debug(0, "processPacket (hex): %02X %02X %02X %02X %02X %02X\n", 
					packet[0],
					packet[1],
					packet[2],
					packet[3],
					packet[4],
					packet[5]
			);
	debug(0, "processPacket (dec): %u %u %u %u %u %u\n", 
					packet[0],
					packet[1],
					packet[2],
					packet[3],
					packet[4],
					packet[5]
			);

	inputPacket.startByte 		= packet[0];
	inputPacket.spring          = ((double)packet[1]+1)/128;
	inputPacket.friction        = ((double)packet[2]+1)/128;
	inputPacket.torqueDirection = packet[3];
	inputPacket.torquePower     = ((double)packet[4]+1)/128;
	inputPacket.crc             = packet[5];
	
	debug(0, "\nstored in struct:\n-----------------\nstartByte:       %d\nspring:          %f\nfriction:        %f\ntorqueDirection: %d\ntorquePower:     %f\ncrc:             %d\n", 
				inputPacket.startByte,
				inputPacket.spring, 
				inputPacket.friction, 
				inputPacket.torqueDirection, 
				inputPacket.torquePower,
				inputPacket.crc
			);

	/* --- (D1^D2^D3^D4)&0x7F --- */
	unsigned char checksum = (packet[1]^packet[2]^packet[3]^packet[4])&0x7F;
	if(checksum!=inputPacket.crc) {
		debug(0, "\nWarning, checksum error\n");
		return FFB_STATUS_ERROR_CHECKSUM;
	}
	debug(0, "\nWill try to execute effect...\n");
	/* --- spring            from 0x00 to 0x7F -> 128 levels --- */
	TriggerSpringEffect(inputPacket.spring);
    /* --- friction          from 0x00 to 0x7F -> 128 levels --- */
	TriggerFrictionEffect(inputPacket.friction);
    /* --- torqueDirection   0x00 = Left, 0x01  = Right      --- */
    /* --- torquePower       from 0x00 to 0xFF -> 128 levels --- */
	TriggerConstantEffect(inputPacket.torqueDirection, inputPacket.torquePower);
 
	return FFB_STATUS_SUCCESS;
}
