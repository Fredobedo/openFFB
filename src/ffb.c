#include <unistd.h>
#include <time.h>
#include "ffb.h"
#include "config.h"
#include "debug.h"
#include "device.h"
#include "ffbhelper.h"
#include <time.h>

/* The in packet used to read from Sega FFB Controller */
/* it contains the converted values from serial raw    */
FFBPacket inputPacket;

//int wheelPosition=8192;

static long long millis(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
}

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

FFBStatus tryResynch()
{
	debug(2, "Try resync comm with Sega FFB Controller...\n");
	for(int cp=0; cp < SEGA_FFB_CONTROLLER_PACKET_SIZE; cp++){
		if(rawPacket[cp]>=0x80){
			int bytesRead = readBytes(rawPacket, SEGA_FFB_CONTROLLER_PACKET_SIZE-cp);
			if (bytesRead < 0){
				return FFB_STATUS_ERROR_TIMEOUT;
			}
			else if (bytesRead < SEGA_FFB_CONTROLLER_PACKET_SIZE-cp)
			{
				return FFB_STATUS_ERROR;
			}
			return readPacket();
		}
	}
	return FFB_STATUS_ERROR_SYNCH_REQUIRED;
}

FFBStatus readDebugPacket(int amount)
{
	int bytesRead = readBytes(rawPacket, amount);
	if (bytesRead > 0){
		for(int cp=0;cp<bytesRead;cp++)
			printf("%02X",rawPacket[cp]);
		
		printf("\n");
	}

	return FFB_STATUS_SUCCESS;
}

FFBStatus WriteReplyPacket()
{
	int bytesWritten = writeBytes(replyPacket, SEGA_FFB_CONTROLLER_REPLY_PACKET_SIZE);

	if (bytesWritten < 0){
		return FFB_STATUS_ERROR_TIMEOUT;
	}
	else if (bytesWritten < SEGA_FFB_CONTROLLER_REPLY_PACKET_SIZE)	
	{
		return FFB_STATUS_ERROR;			
	}
	else{
		return FFB_STATUS_SUCCESS;
	}
}
FFBStatus readPacket()
{
	int bytesRead = readBytes(rawPacket, SEGA_FFB_CONTROLLER_PACKET_SIZE);
	if (bytesRead < 0){
		return FFB_STATUS_ERROR_TIMEOUT;
	}
	else if (bytesRead < SEGA_FFB_CONTROLLER_PACKET_SIZE)
	{
		return FFB_STATUS_ERROR;
	}

	return processPacket(rawPacket);
}

FFBStatus processPacket(unsigned char* packet)
{
	if(getConfig()->debugLevel==3)
		printf("debug3:%02X%02X%02X%02X%02X%02X%02X%02X\n",packet[0],packet[1],packet[2],packet[3],packet[4],packet[5],packet[6],packet[7]);
	
	if(packet[0]<0x80)
		return FFB_STATUS_ERROR_SYNCH_REQUIRED;

	/* --- (D1^D2^D3^D4^D5^D6)&0x7F --- */
	unsigned char checksum = (packet[1]^packet[2]^packet[3]^packet[4]^packet[5]^packet[6])&0x7F;
	if(checksum!=packet[7]) {
		debug(2, "\nWarning, checksum error\n");
		return FFB_STATUS_ERROR_CHECKSUM;
	}

	//SPECIAL COMMANDS SENT BY SEGA FFB CONTROLLER
	if(packet[0]==0xFD)
	{
		switch (packet[1])
		{
		//0x01
		case GET_WHEEL_POSITION:
			replyPacket[0]=0x90;

			int tempPosition=GetWheelPosition();

			if (tempPosition==-1)
				tempPosition=GetWheelPositionIOCTL();

			if (getConfig()->SendWheelPositionToMidi==1)
			{
				int FinalwheelPosition;

				if(getConfig()->InvertedWheelPosition==1)
					FinalwheelPosition=16384 - tempPosition;
				else
					FinalwheelPosition=tempPosition;

				if(FinalwheelPosition<1500)
					FinalwheelPosition=0;
				else if (FinalwheelPosition>15500)
					FinalwheelPosition=16383;

				debug(1, "%lu: GET_POS: %d\n", millis(),FinalwheelPosition);

				replyPacket[1]=(FinalwheelPosition >> 7) & 0x7f;
				replyPacket[2]=FinalwheelPosition & 0x7f;
				replyPacket[3]=(replyPacket[0] ^ replyPacket[1] ^ replyPacket[2]) & 0x7f;
			}
			else
			{
				replyPacket[1]=0;
				replyPacket[2]=0;
				replyPacket[3]=0;
			}

			WriteReplyPacket();
			break;
		//0xA0
		case SET_CENTER:
			FFBSetGlobalAutoCenter(40,1500);
			break;
		//0xA1
		case SET_MAX_RIGHT:
			FFBTriggerConstantEffect(true,0.2);
			break;
		//0xA2
		case SET_MAX_LEFT:
			FFBTriggerConstantEffect(true,-0.2);
			break;
		//0x02
		case GET_POWER_LINE:
			//TO DO
			break;
		//0xFE
		case NOT_READY:
			//nothing to do
			break;
		//0xFF
		case RESET_DEVICE:
			FFBStopAllEffects();
			break;
		default:
			break;
		}

	}
	//NORMAL COMMAND PACKET FOR FORCE FEEDBACK EFFECTS
	else
	{
		inputPacket.startByte 		= packet[0];
		inputPacket.spring          = ((double)packet[1]+1)/128;
		inputPacket.friction        = ((double)packet[2]+1)/128;
		inputPacket.torqueDirection = packet[3];
		inputPacket.torquePower     = ((double)packet[4]+1)/128;
		inputPacket.sineFrequency   = ((double)packet[5]+1)/128;
		inputPacket.sineIntensity   = ((double)packet[6]+1)/128;
		inputPacket.crc             = packet[7];

		/* --- spring            from 0x00 to 0x7F -> 128 levels --- */
		if(packet[1]==0x0)
			FFBStopEffect(ffb_effects[spring_effect_idx].id);
		else
			FFBTriggerSpringEffect(previous_rawpacket[1]!=packet[1], inputPacket.spring);

		/* --- friction          from 0x00 to 0x7F -> 128 levels                                                --- */
		/* --- For now on, I will use Sine effect instead as I can't control the strengh of a froction effect ? --- */
		if(packet[2]==0x0)
			FFBStopEffect(ffb_effects[friction_effect_idx].id);
		else
			FFBTriggerFrictionEffect(previous_rawpacket[2]!=packet[2], inputPacket.friction);	

		/* --- torqueDirection   0x00 = Left, 0x01  = Right                     --- */
		/* --- torquePower       from 0x00 to 0x7F -> 128 levels                --- */
		/* note that torqueDirection is where the wheel is turning                  */
		if(packet[4]==0x0 ){
			FFBStopEffect(ffb_effects[constant_effect_idx].id);
		}
		else{
			if(inputPacket.torqueDirection==0)
				FFBTriggerConstantEffect(previous_rawpacket[3]!=packet[3]|| previous_rawpacket[4]!=packet[4], -inputPacket.torquePower);
			else
				FFBTriggerConstantEffect(previous_rawpacket[3]!=packet[3]|| previous_rawpacket[4]!=packet[4], inputPacket.torquePower);
		}

		if(packet[6]==0x0 || packet[5]==0x0)
			FFBStopEffect(ffb_effects[sine_effect_idx].id);
		else
			FFBTriggerSineEffect(previous_rawpacket[5]!=packet[5]|| previous_rawpacket[6]!=packet[6], inputPacket.sineFrequency, inputPacket.sineIntensity);

			
		/* only copy if there is a diff */
		if(memcmp(previous_rawpacket, packet, 8)!=0){
			if(getConfig()->debugLevel==1)
				printf("%02X%02X%02X%02X%02X%02X%02X%02X\n",packet[0],packet[1],packet[2],packet[3],packet[4],packet[5],packet[6],packet[7]);

			memcpy(previous_rawpacket, packet, 8);
		}
	}
	return FFB_STATUS_SUCCESS;
}

void playCOMInitEffect()
{
	debug(2, "playCOMInitEffect:\n  ");
	FFBTriggerConstantEffect(true, -0.70);
	usleep(40 * 1000);
	debug(2, "\n  - ");
	FFBTriggerConstantEffect(true, 0.0);
	usleep(20 * 1000);
	debug(2, "\n  - ");
	FFBTriggerConstantEffect(true, 0.70);
	usleep(40 * 1000);
	debug(2, "\n  - ");
	FFBTriggerConstantEffect(true, 0.0);
	usleep(40 * 1000);
	debug(2, "\n");
	debug(2, "playCOMInitEffect finished.\n");
}

void playCOMEndEffect()
{
	debug(2,"playCOMEndEffect!!!\n");	
	FFBTriggerConstantEffect(true, 0.80);
	usleep(70 * 1000);
	FFBTriggerConstantEffect(true, 0.0);
	FFBSetGlobalAutoCenter(40,1000); 
}
