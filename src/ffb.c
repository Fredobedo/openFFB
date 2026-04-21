#include <unistd.h>
#include <time.h>
#include "ffb.h"
#include "config.h"
#include "debug.h"
#include "device.h"
#include "ffbhelper.h"
//#include <linux/time.h>
#include <pthread.h>
#include <stdatomic.h>

/* The in packet used to read from Sega FFB Controller */
/* it contains the converted values from serial raw    */
FFBPacket inputPacket;

static long long millis(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (long long)ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
}

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

FFBStatus tryResynch()
{
	debug(2, "Try resync comm with Sega FFB Controller...\n");
	for (int cp = 0; cp < SEGA_FFB_CONTROLLER_PACKET_SIZE; cp++)
	{
		if (rawPacket[cp] >= 0x80)
		{
			int bytesRead = readBytes(rawPacket, SEGA_FFB_CONTROLLER_PACKET_SIZE - cp);
			if (bytesRead < 0)
			{
				return FFB_STATUS_ERROR_TIMEOUT;
			}
			else if (bytesRead < SEGA_FFB_CONTROLLER_PACKET_SIZE - cp)
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
	if (bytesRead > 0)
	{
		for (int cp = 0; cp < bytesRead; cp++)
			printf("%02X", rawPacket[cp]);

		printf("\n");
	}

	return FFB_STATUS_SUCCESS;
}

FFBStatus WriteReplyPacket()
{
	int bytesWritten = writeBytes(replyPacket, SEGA_FFB_CONTROLLER_REPLY_PACKET_SIZE);

	if (bytesWritten < 0)
	{
		return FFB_STATUS_ERROR_TIMEOUT;
	}
	else if (bytesWritten < SEGA_FFB_CONTROLLER_REPLY_PACKET_SIZE)
	{
		return FFB_STATUS_ERROR;
	}
	else
	{
		return FFB_STATUS_SUCCESS;
	}
}
FFBStatus readPacket()
{
	int bytesRead = readBytes(rawPacket, SEGA_FFB_CONTROLLER_PACKET_SIZE);
	if (bytesRead < 0)
	{
		return FFB_STATUS_ERROR_TIMEOUT;
	}
	else if (bytesRead < SEGA_FFB_CONTROLLER_PACKET_SIZE)
	{
		return FFB_STATUS_ERROR;
	}

	return processPacket(rawPacket);
}

FFBStatus processPacket(unsigned char *packet)
{
	if (getConfig()->debugLevel > 3)
	 	printf("debug4:%02X%02X%02X%02X%02X%02X%02X%02X\n", packet[0], packet[1], packet[2], packet[3], packet[4], packet[5], packet[6], packet[7]);

	if (packet[0] < 0x80)
		return FFB_STATUS_ERROR_SYNCH_REQUIRED;

	/* --- (D1^D2^D3^D4^D5^D6)&0x7F --- */
	unsigned char checksum = (packet[1] ^ packet[2] ^ packet[3] ^ packet[4] ^ packet[5] ^ packet[6]) & 0x7F;
	if (checksum != packet[7])
	{
		debug(2, "\nWarning, checksum error\n");
		return FFB_STATUS_ERROR_CHECKSUM;
	}

	// SPECIAL COMMANDS SENT BY SEGA FFB CONTROLLER
	if (packet[0] == OPENFFB_GENERIC_CMD)
	{
		switch (packet[1])
		{
		// 0x01
		case OPENFFB_GET_WHEEL_POSITION_SUB_CMD: // Synchronous command, reply directly
			replyPacket[0] = OPENFFB_WHEEL_POSITION_REPLY_CMD;

			int wheelPosition = GetCachedWheelPosition();
			debug(1, "GET_POS: %d\n", wheelPosition);
			//debug(1, "%lu: GET_POS: %d\n", millis(),wheelPosition);
			replyPacket[1] = (wheelPosition >> 7) & 0x7f;
			replyPacket[2] = wheelPosition & 0x7f;
			replyPacket[3] = (replyPacket[0] ^ replyPacket[1] ^ replyPacket[2]) & 0x7f;

			WriteReplyPacket();
			break;
		// 0xA0
		case OPENFFB_SET_CENTER_SUB_CMD: 	// Asynchronous command, reply will be sent by Worker thread
			ThreadParams *centerParams = malloc(sizeof(ThreadParams));
			*centerParams = (ThreadParams){0, 0.90, 20, 3500};
			startWorkerAsync(WorkerSetCenter, centerParams);
			break;
		// 0xA1
		case OPENFFB_SET_MAX_RIGHT_SUB_CMD: // Asynchronous command, reply will be sent by Worker thread
			ThreadParams *maxRightParams = malloc(sizeof(ThreadParams));
			*maxRightParams = (ThreadParams){0, 0.15, 0, 60000};
			startWorkerAsync(WorkerSetPosition, maxRightParams);
			break;
		// 0xA2
		case OPENFFB_SET_MAX_LEFT_SUB_CMD: 	// Asynchronous command, reply will be sent by Worker thread
			ThreadParams *maxLeftParams = malloc(sizeof(ThreadParams));
			*maxLeftParams = (ThreadParams){16383, 0.20, 0, 60000};
			startWorkerAsync(WorkerSetPosition, maxLeftParams);
			break;
		// 0x02
		case OPENFFB_GET_POWER_LINE_SUB_CMD:
			// TO DO
			break;
		// 0xFE
		case OPENFFB_NOT_READY_SUB_CMD:
			SegaFFBControllerState = OPENFFB_NOT_READY_SUB_CMD;
			// nothing to do
			break;
		case OPENFFB_READY_SUB_CMD:
			SegaFFBControllerState = OPENFFB_READY_SUB_CMD;
			// nothing to do
			break;			
		// 0xFF
		case OPENFFB_RESET_DEVICE_SUB_CMD:
			FFBStopAllEffects();
			break;
		default:
			break;
		}
	}
	// NORMAL COMMAND PACKET FOR BULCK FORCE FEEDBACK EFFECTS FROM AGANYTE
	// No reply from openFFB, the Sega FFB Controller will take care of it (Send reply to Sega System).
	else
	{
		inputPacket.startByte = packet[0];
		inputPacket.spring = ((double)packet[1] + 1)   / 128;
		inputPacket.friction = ((double)packet[2] + 1) / 128;
		inputPacket.torqueDirection = packet[3];
		inputPacket.torquePower = ((double)packet[4])  / 128;
		inputPacket.sineFrequency = ((double)packet[5])     / 2;
		inputPacket.sineIntensity = ((double)packet[6] + 1) / 128;

		/* --- spring            from 0x00 to 0x7F -> 128 levels --- */
		// if (packet[1] == 0x0)
		// 	FFBStopEffect(ffb_effects[spring_effect_idx].id);
		// else 
			if(previous_rawpacket[1] != packet[1])
				FFBTriggerSpringEffect(previous_rawpacket[1] != packet[1], inputPacket.spring, false);

		/* --- friction          from 0x00 to 0x7F -> 128 levels                                                --- */
		/* --- For now on, I will use Sine effect instead as I can't control the strengh of a froction effect ? --- */
		// if (packet[2] == 0x0)
		// 	FFBStopEffect(ffb_effects[friction_effect_idx].id);
		// else 
			if(previous_rawpacket[2] != packet[2])
				FFBTriggerFrictionEffect(previous_rawpacket[2] != packet[2], inputPacket.friction);

		/* ---                --- */
		// if (packet[4] == 0x0)
		// 	FFBStopEffect(ffb_effects[constant_effect_idx].id);
		// else
		// {
			if(previous_rawpacket[3] != packet[3] || previous_rawpacket[4] != packet[4])
			{
				// DIRECTION:
				//  - 0 = Right = Negative value for Linux FFB Effect
				//  - 1 = Left  = Positive value for Linux FFB Effect 
				if (inputPacket.torqueDirection == 0)
					FFBTriggerConstantEffect(previous_rawpacket[3] != packet[3] || previous_rawpacket[4] != packet[4], -inputPacket.torquePower, false);
				else
					FFBTriggerConstantEffect(previous_rawpacket[3] != packet[3] || previous_rawpacket[4] != packet[4], inputPacket.torquePower, false);
			}
		// }

		// if (packet[6] == 0x0 || packet[5] == 0x0)
		// 	FFBStopEffect(ffb_effects[sine_effect_idx].id);
		// else 
			if(previous_rawpacket[5] != packet[5] || previous_rawpacket[6] != packet[6])
				FFBTriggerSineEffect(previous_rawpacket[5] != packet[5] || previous_rawpacket[6] != packet[6], inputPacket.sineFrequency, inputPacket.sineIntensity);

		/* only copy if there is a diff */
		if (memcmp(previous_rawpacket, packet, 8) != 0)
		{
			if (getConfig()->debugLevel == 1)
				printf("%02X%02X%02X%02X%02X%02X%02X%02X\n", packet[0], packet[1], packet[2], packet[3], packet[4], packet[5], packet[6], packet[7]);

			memcpy(previous_rawpacket, packet, 8);
		}
	}
	return FFB_STATUS_SUCCESS;
}

void playCOMInitEffect()
{
	debug(2, "playCOMInitEffect:\n  - ");
	FFBTriggerConstantEffect(true, -0.70, false);
	usleep(40 * 1000);
	debug(2, "\n  - ");
	FFBTriggerConstantEffect(true, 0.0, false);
	usleep(20 * 1000);
	debug(2, "\n  - ");
	FFBTriggerConstantEffect(true, 0.70, false);
	usleep(40 * 1000);
	debug(2, "\n  - ");
	FFBTriggerConstantEffect(true, 0.0, false);
	usleep(40 * 1000);
	debug(2, "\n");
	debug(2, "playCOMInitEffect finished.\n");
}

void playCOMEndEffect()
{
	debug(2, "playCOMEndEffect!!!\n");
	FFBTriggerConstantEffect(true, 0.70, false);
	usleep(70 * 1000);
	FFBTriggerConstantEffect(true, 0.0, false);
	FFBSetGlobalAutoCenter(40, 1000);
}
