#include "ffb.h"
#include "config.h"
#include "debug.h"
#include "device.h"
#include <time.h>
#include "sdlhelper.h"
#include "ffbhelper.h"
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

FFBStatus readPacketSDL()
{
	int bytesRead = readBytes(rawPacket, 6);
	if (bytesRead < 0)
		return FFB_STATUS_ERROR_TIMEOUT;
	else if (bytesRead < 6)
		return FFB_STATUS_ERROR;


	return processPacketSDL(rawPacket);
}

FFBStatus readPacketC()
{
	int bytesRead = readBytes(rawPacket, 6);
	if (bytesRead < 0)
		return FFB_STATUS_ERROR_TIMEOUT;
	else if (bytesRead < 6)
		return FFB_STATUS_ERROR;


	return processPacketC(rawPacket);
}

FFBStatus processPacketSDL(unsigned char* packet)
{
	
	debug(2, "\nprocessPacket (hex): %02X %02X %02X %02X %02X %02X\n", 
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

	/* --- (D1^D2^D3^D4)&0x7F --- */
	unsigned char checksum = (packet[1]^packet[2]^packet[3]^packet[4])&0x7F;
	if(checksum!=inputPacket.crc) {
		debug(2, "\nWarning, checksum error\n");
		return FFB_STATUS_ERROR_CHECKSUM;
	}
	//debug(2, "\nWill try to execute effect...\n");
	/* --- spring            from 0x00 to 0x7F -> 128 levels --- */
	//debug(2, "previous_rawpacket[1]=%02X, packet[1]=%02X\n", previous_rawpacket[1], packet[1]);
	debug(2, "previous_rawpacket[1]=%02X, packet[1]=%02X\n",previous_rawpacket[1],packet[1]);	
	if(previous_rawpacket[1]!=packet[1])
		TriggerSpringEffect(inputPacket.spring);

    /* --- friction          from 0x00 to 0x7F -> 128 levels --- */
	debug(2, "previous_rawpacket[2]=%02X, packet[2]=%02X\n",previous_rawpacket[2],packet[2]);
	if(previous_rawpacket[2]!=packet[2])
		TriggerFrictionEffect(inputPacket.friction);		

    /* --- torqueDirection   0x00 = Left, 0x01  = Right                     --- */
    /* --- torquePower       from 0x00 to 0xFF -> 128 levels                --- */
	/* note that torqueDirection is where the wheel is turning                  */
	/* this is the oposite of SDL direction, which is where the force comes     */
	/* for SDL left=1, right=-1                                                 */
	//debug(0, "previous_rawpacket[3]=%02X, packet[3]=%02X, direction=%d\n",previous_rawpacket[3],packet[3], inputPacket.torqueDirection);	
	if(previous_rawpacket[3]!=packet[3]|| previous_rawpacket[4]!=packet[4] ){
		if(inputPacket.torqueDirection==0)
			inputPacket.torqueDirection=1;
		else
			inputPacket.torqueDirection=-1;
		TriggerConstantEffect(inputPacket.torqueDirection, inputPacket.torquePower);
	}

	/* only copy if there is a diff */
	if(memcmp(previous_rawpacket, packet,6)!=0){
		debug(0, "%02X %02X %02X %02X %02X %02X\n", 
					packet[0],
					packet[1],
					packet[2],
					packet[3],
					packet[4],
					packet[5]
			);
		memcpy(previous_rawpacket, packet, 6);
	}

	return FFB_STATUS_SUCCESS;
}

FFBStatus processPacketC(unsigned char* packet)
{
	
	debug(2, "\nprocessPacket (hex): %02X %02X %02X %02X %02X %02X\n", 
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

	/* --- (D1^D2^D3^D4)&0x7F --- */
	unsigned char checksum = (packet[1]^packet[2]^packet[3]^packet[4])&0x7F;
	if(checksum!=inputPacket.crc) {
		debug(2, "\nWarning, checksum error\n");
		return FFB_STATUS_ERROR_CHECKSUM;
	}
	//debug(2, "\nWill try to execute effect...\n");
	/* --- spring            from 0x00 to 0x7F -> 128 levels --- */
	//debug(2, "previous_rawpacket[1]=%02X, packet[1]=%02X\n", previous_rawpacket[1], packet[1]);
	debug(2, "previous_rawpacket[1]=%02X, packet[1]=%02X\n",previous_rawpacket[1],packet[1]);	
	if(previous_rawpacket[1]!=packet[1])
		TriggerSpringEffect(inputPacket.spring);

    /* --- friction          from 0x00 to 0x7F -> 128 levels --- */
	debug(2, "previous_rawpacket[2]=%02X, packet[2]=%02X\n",previous_rawpacket[2],packet[2]);
	if(previous_rawpacket[2]!=packet[2])
		TriggerFrictionEffect(inputPacket.friction);		

    /* --- torqueDirection   0x00 = Left, 0x01  = Right                     --- */
    /* --- torquePower       from 0x00 to 0xFF -> 128 levels                --- */
	/* note that torqueDirection is where the wheel is turning                  */
	/* this is the oposite of SDL direction, which is where the force comes     */
	/* for SDL left=1, right=-1                                                 */
	//debug(0, "previous_rawpacket[3]=%02X, packet[3]=%02X, direction=%d\n",previous_rawpacket[3],packet[3], inputPacket.torqueDirection);	
	if(previous_rawpacket[3]!=packet[3]|| previous_rawpacket[4]!=packet[4] ){
		if(inputPacket.torqueDirection==0)
			inputPacket.torqueDirection=1;
		else
			inputPacket.torqueDirection=-1;
		TriggerConstantEffect(inputPacket.torqueDirection, inputPacket.torquePower);
	}

	/* only copy if there is a diff */
	if(memcmp(previous_rawpacket, packet,6)!=0){
		debug(0, "%02X %02X %02X %02X %02X %02X\n", 
					packet[0],
					packet[1],
					packet[2],
					packet[3],
					packet[4],
					packet[5]
			);
		memcpy(previous_rawpacket, packet, 6);
	}

	return FFB_STATUS_SUCCESS;
}

void playCOMInitEffect()
{
	TriggerRumbleEffectDefault(1.0);
	usleep(500);
	TriggerRumbleEffectDefault(1.0);
}

void playCOMEndEffect()
{
	TriggerRumbleEffect(1.0, 2000);
}
