#include "ffb.h"
#include "config.h"
#include "debug.h"
#include "device.h"
#include <time.h>

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

FFBStatus processPacket()
{
	return FFB_STATUS_SUCCESS;
}

FFBStatus readPacket(FFBPacket *packet)
{
	return FFB_STATUS_SUCCESS;
}
