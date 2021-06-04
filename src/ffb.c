#include <unistd.h>
#include <time.h>
#include "ffb.h"
#include "config.h"
#include "debug.h"
#include "device.h"
#include "ffbhelper.h"

/* The in packet used to read from Sega FFB Controller */
FFBPacket inputPacket;
unsigned char rawPacket[6];

int initFFB(char *devicePath)
{
	/* Init the connection to the Aganyte's Sega FFB Controller */
	//int rc=initDevice(devicePath);
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
	if (bytesRead < 0){
		return FFB_STATUS_ERROR_TIMEOUT;
	}
	else if (bytesRead < 6)
	{
		return FFB_STATUS_ERROR;
	}

	return processPacket(rawPacket);
}

FFBStatus processPacket(unsigned char* packet)
{
	if(getConfig()->debugLevel==3)
		printf("%02X%02X%02X%02X%02X%02X\n", packet[0],	packet[1], packet[2], packet[3], packet[4], packet[5]);

	inputPacket.startByte 		= packet[0];
	inputPacket.spring          = ((double)packet[1]+1)/128;
	inputPacket.friction        = ((double)packet[2]+1)/128;
	inputPacket.torqueDirection = packet[3];
	inputPacket.torquePower     = ((double)packet[4]+1)/128;
	inputPacket.crc             = packet[5];

	/* --- (D1^D2^D3^D4)&0x7F --- */
	unsigned char checksum = (packet[1]^packet[2]^packet[3]^packet[4])&0x7F;
	if(checksum!=inputPacket.crc) {
		debug(2, "\nWarning, checksum error\n");
		return FFB_STATUS_ERROR_CHECKSUM;
	}

	/* --- spring            from 0x00 to 0x7F -> 128 levels --- */
	if(previous_rawpacket[1]!=packet[1])
		FFBTriggerSpringEffect(inputPacket.spring);

    /* --- friction          from 0x00 to 0x7F -> 128 levels                                                --- */
	/* --- For now on, I will use Sine effect instead as I can't control the strengh of a froction effect ? --- */
	if(previous_rawpacket[2]!=packet[2])
		FFBTriggerFrictionEffect(inputPacket.friction);	

    /* --- torqueDirection   0x00 = Left, 0x01  = Right                     --- */
    /* --- torquePower       from 0x00 to 0x7F -> 128 levels                --- */
	/* note that torqueDirection is where the wheel is turning                  */
	if(previous_rawpacket[3]!=packet[3]|| previous_rawpacket[4]!=packet[4] ){
		if(inputPacket.torqueDirection==0)
			FFBTriggerConstantEffect(-inputPacket.torquePower);
		else
			FFBTriggerConstantEffect(inputPacket.torquePower);
		
	}

	/* only copy if there is a diff */
	if(memcmp(previous_rawpacket, packet,6)!=0){
		if(getConfig()->debugLevel==1)
			printf("%02X%02X%02X%02X%02X%02X\n", packet[0],	packet[1], packet[2], packet[3], packet[4], packet[5]);

		memcpy(previous_rawpacket, packet, 6);
	}

	return FFB_STATUS_SUCCESS;
}

void playCOMInitEffect()
{
	debug(2, "playCOMInitEffect!!!\n");
	FFBTriggerConstantEffect(-0.70);
	usleep(40 * 1000);
	FFBTriggerConstantEffect(0.0);
	usleep(20 * 1000);
	FFBTriggerConstantEffect(0.70);
	usleep(40 * 1000);
	FFBTriggerConstantEffect(0.0);
}

void playCOMEndEffect()
{
	debug(2,"playCOMEndEffect!!!\n");	
	FFBTriggerConstantEffect(0.80);
	usleep(70 * 1000);
	FFBTriggerConstantEffect(0.0);
}
