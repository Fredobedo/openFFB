    
#include "debug.h"
#include "ffbhelper.h"
#include "config.h"
#include <time.h>
#include <math.h>
	
#include<stdio.h>
#include<unistd.h>
#include<string.h>

#include <linux/input.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <ctype.h>
#include <pthread.h>

#define CLAMP(x, lo, hi) ((x) > (hi) ? (hi) : ((x) < (lo) ? (lo) : (x)))

int device_handle = -1;
bool LogitechWheelDetected=false;
atomic_int storedWheelPosition = 8192;
static bool constantPlaying[2];
static pthread_t calibrationThread;
static bool calibrationActive;
static atomic_bool calibrationCancelled;

unsigned int WheelEffectiveMaxPosition=16383;
	
/*
This code is super highly based on work
Thank you again for having shared your work and knowledge on this topic, it was a great help to get started and understand how to use the linux force feedback API.

some links: 
	linux/input.h    : lots of information in this header file
	https://www.kernel.org/doc/html/latest/input/ff.html
	https://github.com/Eliasvan/Linux-Force-Feedback
	https://www.youtube.com/watch?v=pCq01LHaIVg
	https://www.linuxjournal.com/article/6429
	https://github.com/Wiladams/LJIT2RPi/blob/master/tests/test_input.c
	fftest          : https://github.com/flosse/linuxconsole/blob/master/utils/fftest.c
	openjvs         : https://github.com/OpenJVS/OpenJVS/blob/bobby/ffb/src/ffb.c
	ff-memless-next : https://github.com/chrisboyle/G940-linux/blob/main/drivers/input/ff-memless-next.c

	sudo strace -e ioctl -p 8792 -s 999 -v -o openffb_upload_ubuntu.log
*/
#define LONG_BITS (sizeof(long) * 8)

struct ff_effect effect;

 unsigned char ahex2bin(unsigned char MSB, unsigned char LSB) {  
    if (MSB > '9') MSB -= 7;          // Convert MSB value to a contiguous range (0x30..0x3F)  
    if (LSB > '9') LSB -= 7;          // Convert LSB value to a contiguous range (0x30..0x3F)  
     return (MSB <<4) | (LSB & 0x0F); // Make a result byte  using only low nibbles of MSB and LSB thus neglecting the input register case
 }  
 
// Generic async worker thread
// Fire-end-forget mode
int startWorkerAsync(WorkerFunc func, void* arg) 
{
    pthread_t tid;
    int ret = pthread_create(&tid, NULL, func, arg);
    if (ret == 0) pthread_detach(tid);
    return ret;
}

// Main-thread ownership: cancellation joins before another job or stop can
// touch the calibration slots. Signal handlers only change `running`.
void FFBCancelCalibration(void)
{
    atomic_store(&calibrationCancelled, true);
    if (calibrationActive) {
        pthread_join(calibrationThread, NULL);
        calibrationActive = false;
    }
}

int FFBStartCalibration(WorkerFunc func, ThreadParams *params)
{
    FFBCancelCalibration();
    atomic_store(&calibrationCancelled, false);
    int result = pthread_create(&calibrationThread, NULL, func, params);
    calibrationActive = result == 0;
    if (result) free(params);
    return result;
}

static bool calibrationWait(int duration_ms)
{
    for (int elapsed = 0; elapsed < duration_ms && running && !atomic_load(&calibrationCancelled); ) {
        int slice = duration_ms - elapsed < 20 ? duration_ms - elapsed : 20;
        usleep(slice * 1000);
        elapsed += slice;
    }
    return running && !atomic_load(&calibrationCancelled);
}

void* WorkerSetCenter(void* arg)
{
    ThreadParams *params = arg;
    if (!params) return NULL;
    if (calibrationWait(params->wait_before_start_ms)) {
        FFBTriggerSpringEffect(true, params->strength, true);
        calibrationWait(params->duration_ms);
        FFBStopEffect(ffb_effects[spring_effect_async_idx].id);
    }
    free(params);
    return NULL;
}

void* WorkerSetPosition(void* arg)
{
    ThreadParams *params = arg;
    if (!params) return NULL;
    if (calibrationWait(params->wait_before_start_ms)) {
        double strength = params->position > GetCachedWheelPosition()
            ? fabs(params->strength) : -fabs(params->strength);
        FFBTriggerConstantEffect(true, strength, true);
        for (int elapsed = 0; elapsed < params->duration_ms && GetCachedWheelPosition() != params->position; elapsed += 20)
            if (!calibrationWait(20)) break;
        FFBStopEffect(ffb_effects[constant_effect_async_idx].id);
        // Never report successful endpoint calibration after a timeout/cancel.
        if (running && !atomic_load(&calibrationCancelled) && GetCachedWheelPosition() == params->position) {
            unsigned char reply[4] = {params->position == 0 ? OPENFFB_MAX_RIGHT_REPLY_CMD : OPENFFB_MAX_LEFT_REPLY_CMD, 0, 0, 0};
            reply[3] = reply[0] & 0x7f;
            SendReplyPacket(reply);
        }
    }
    free(params);
    return NULL;
}

  void initCOMSegaFFBController()
  {
	FFBConfig *localConfig = getConfig(); 
    debug(1, "Connecting to Sega FFB Controller...");
    while(!initFFB(localConfig->segaFFBControllerPath) && running)
    {
      debug(1, ".");
      fflush(stdout);
      sleep(1);
    }
    debug(1, "Done!\n");
  }

  
void* WorkerUpdateCachedWheelPosition(void* arg)
{
	while (running)
	{

		int FinalwheelPosition;
		int tempPosition = GetNewWheelPositionIOCTL() / (int)(WheelEffectiveMaxPosition / 16383); // scale to 0-16383 range

		if (tempPosition >= 0)
		{
			// if (getConfig()->InvertedWheelPosition == 1)
			FinalwheelPosition = 16384 - tempPosition;
			// else
			// 	FinalwheelPosition = tempPosition;

			if (FinalwheelPosition < 1500)
				FinalwheelPosition = 0;
			else if (FinalwheelPosition > 15500)
				FinalwheelPosition = 16383;

			atomic_store(&storedWheelPosition, FinalwheelPosition);
		}
		usleep(8 * 1000); 
	}
}



int GetCachedWheelPosition()
{
	return atomic_load(&storedWheelPosition);
}

bool IsLogitechWheel()
{
	return LogitechWheelDetected;
}	

bool FFBGetDeviceName(int handle, char *deviceName)
{
	int version;
	if (!ioctl(handle, EVIOCGNAME(256), deviceName)) 
	{
		debug(1," Error in evdev ioctl for FFBGetDeviceName (%s) [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
    	return false;
	}
	else
		return true;
}

bool FFBGetDeviceDriverVersion(int handle, char *deviceVersion)
{
	int version;
	if (ioctl(handle, EVIOCGVERSION, &version)) 
	{
		debug(1," Error in evdev ioctl for FFBGetDeviceDriverVersion (%s) [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
    	return false;
	}
	else{
		snprintf(deviceVersion, 20, "%d.%d.%d", version >> 16, (version >> 8) & 0xff, version & 0xff);
		return true;
	}
}

bool FFBGetDeviceVendorProductVersion(int handle, char *deviceVendor, char *deviceProduct, char *deviceVersion)
{
	struct input_id device_info;

	if (ioctl(handle, EVIOCGID, &device_info)) 
	{
		debug(1," Error in evdev ioctl for FFBGetDeviceVendorProductVersion (%s) [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
    	return false;
	}
	else{
		snprintf(deviceVendor,  5,"%04hx", device_info.vendor);
		snprintf(deviceProduct, 5,"%04hx", device_info.product);
		snprintf(deviceVersion, 5,"%04hx", device_info.version);				
		return true;
	}
}

bool FFBCheckIfFFBDevice(int handle)
{
	/* Now get some information about force feedback */
	memset(ff_bits, 0, sizeof(ff_bits));
	if (ioctl(handle, EVIOCGBIT(EV_FF, sizeof(ff_bits)), ff_bits) < 0)
	{
		fprintf(stderr, "ERROR: can not get ff bits (%s) [%s:%d]\n",
				strerror(errno), __FILE__, __LINE__);
		return false;
	}

	/* force feedback supported? */
	if (testBit(FF_RUMBLE, ff_bits) || testBit(FF_CONSTANT, ff_bits))
		return true;
	else
		return false;
}

char* FFBGetHapticSimplifiedName(const char* name)
{
	char* simplifiedName=malloc (sizeof (char) * 128);;
	strcpy(simplifiedName, name);

	for (int i = 0; i < (int)strlen(simplifiedName); i++)
	{
		simplifiedName[i] = tolower(simplifiedName[i]);
		if (simplifiedName[i] == ' ' || simplifiedName[i] == '/' || simplifiedName[i] == '(' || simplifiedName[i] == ')')
			simplifiedName[i] = '-';
	}
	return simplifiedName;
}

int FFBisEventDevice(const struct dirent *dir)
{
    return strncmp("event", dir->d_name, 5) == 0;
}

int FFBGetAllDevices()
{
	memset(devices, 0, sizeof(devices));
	struct dirent **namelist;
	int NbrOfdc=0;
	NbrOfDevices=0;
    if ((NbrOfdc = scandir(DEV_INPUT_EVENT, &namelist, FFBisEventDevice, alphasort)) < 1)
    {
        debug(0, "Error: No devices found\n");
        return 0;
    }

	for (int i = 0; i < MAX_DEVICE && i < NbrOfdc; i++)
    {
        int fd = -1;
        char name[256] = {0};
		char fname[512];

        snprintf(fname, sizeof(fname), "%s/%s", DEV_INPUT_EVENT, namelist[i]->d_name);
		debug(2, "testing device %s\n", fname);
        if ((fd = open(fname, O_RDONLY)) > -1)
        {
			if(FFBCheckIfFFBDevice(fd)){
				debug(2, "this is a ffb device\n");
				strcpy(devices[NbrOfDevices].path, fname);
				FFBGetDeviceName(fd, devices[NbrOfDevices].realName);
				strcpy(devices[NbrOfDevices].simplifiedName, FFBGetHapticSimplifiedName(devices[NbrOfDevices].realName));

				FFBGetDeviceVendorProductVersion(fd, 
												devices[NbrOfDevices].vendor, 
												devices[NbrOfDevices].Product, 
												devices[NbrOfDevices].Version);
				FFBGetDeviceDriverVersion(fd, devices[NbrOfDevices].driverVersion);
				
				NbrOfDevices++;
			}
			else
				debug(2, "not a ffb device\n");
			close(fd);
        }
		else
		{
			debug(2, "Warning, can not open dc\n");
		}
    }
    free(namelist);

	return NbrOfDevices;

}

void FFBDumpAvailableDevices()
{
	if(FFBGetAllDevices()>0)
	{
		debug(0, "----------------------------------------------------\n");
		debug(0, "Available Devices:\n");

		for (int i = 0; i < NbrOfDevices; i++) 
		{
		    debug(0, "----------------------------------------------------\n");
			if(devices[i].path){
				debug(0, "Device[%d] Simplified Name        : %s\n", i, devices[i].simplifiedName);
				debug(0, "Device[%d] Real Name              : %s\n", i, devices[i].realName);
				debug(0, "Device[%d] Vendor/Product/Version : %s/%s/%s\n", i, devices[i].vendor, 
																			  devices[i].Product, 
																			  devices[i].Version);
				debug(0, "Device[%d] Driver Version         : %s\n", i, devices[i].driverVersion);
				debug(0, "Device[%d] path                   : %s\n", i, devices[i].path);
			}
		}
	}
	else
		debug(0, "Warning, no haptic device found!\n");
}

int FFBGetDeviceIdx(char* device_name)
{
	int idxDevice=-1;

	char *end;
	long lnum = strtol(device_name, &end, 10); 
	int num = (int) lnum;

	if (end != device_name && NbrOfDevices > num) {
		idxDevice=num;
	}
	else{
		/* Try to find matching device */
		for (int i = 0; i < NbrOfDevices; i++) {
			if (strcmp(devices[i].simplifiedName, device_name) == 0){
				idxDevice=i;
				break;
			}
		}
	}

	if (idxDevice ==-1) {
		debug(0, "Unable to find a device matching '%s', aborting.\n", device_name);
		return 0;
	}
	else
		return idxDevice;
}

int GetNewWheelPositionIOCTL()
{
	int wheelPosition=-1;
	struct input_absinfo absinfo;

	if (ioctl(device_handle, EVIOCGABS(ABS_X), &absinfo) >= 0) {
		wheelPosition = absinfo.value;
	}

	return wheelPosition;
}

// /*
// ABS_X: max right = 16382
//       mmax_left  = 0 
// */
// int GetWheelPositionABSX()
// {
// 	struct input_event event;
//     fd_set file_descriptor;
//     struct timeval tv;

// 	FD_ZERO(&file_descriptor);
//     FD_SET(device_handle, &file_descriptor);

//     tv.tv_sec = 0;
//     tv.tv_usec = 2 * 1000;

//     if (select(device_handle + 1, &file_descriptor, NULL, NULL, &tv) < 1)
//         return -1;

// 	int bytesRead = read(device_handle, &event, sizeof(struct input_event));

// 	if (bytesRead == sizeof(struct input_event)) {
// 		if (event.type == EV_ABS && event.code == ABS_X) {
// 			return event.value;
// 		}
// 	}
// 	return -1; // Indicate an error or no position available
// }

bool FFBInitHaptic(char* device_name)
{
	if(FFBGetAllDevices()) {
		int idxDevice=FFBGetDeviceIdx(device_name);

		if(idxDevice> -1 && devices[idxDevice].path){
			unsigned char ffb_supported[1 + FF_MAX/8/sizeof(unsigned char)];

			device_handle = open(devices[idxDevice].path, O_RDWR|O_NONBLOCK);

			if (device_handle > -1) {
				debug(2, "Using device %s.\n\n", device_name);

				GetWheelEffectiveMaxPosition();
				/*-- test if it's a logitech Racing wheel (ID_VENDOR=046d) --*/
				if(strcmp(devices[idxDevice].vendor,"046d")==0)
				{
					if(SetLogitechSysFsDirectory(idxDevice))
					{
						LogitechWheelDetected=true;

						debug(2, "SetLogitechSysFsDirectory success\n");
						
						debug(2, "SetLogitechSteeringRange ... \n");
						SetSYSFSEntry("range", getConfig()->logitechSteeringRange);
					}
					else
					{
						debug(2, "SetLogitechSysFsDirectory failed\n");
					}
				}
				FFBCreateAllHapticEffects();


				return true;
			}
			else {
				debug(0, "ERROR: can not open %s (%s)\n", device_name, strerror(errno));
				return false;
			}
		}
		else
			return false;
	}
	else
		return false;
}

void GetWheelEffectiveMaxPosition()
{
	struct input_absinfo absinfo;

	if (ioctl(device_handle, EVIOCGABS(ABS_X), &absinfo) >= 0) {
		WheelEffectiveMaxPosition = absinfo.maximum;
		//debug(0, "Wheel Effective Max Position: %d\n", WheelEffectiveMaxPosition);
	}
}

void GetWheelEffectiveMinStrength()
{
	debug(3, "Starting GetWheelEffectiveMinStrength()...\n");
	minimumStrength = 0.0;
	if (!running || getConfig()->globalGain <= 0) return;

	bool MoveDetected=false;

	ThreadParams *centerParams = malloc(sizeof(ThreadParams));
	*centerParams = (ThreadParams){0, 0.90, 0, 3000};
	WorkerSetCenter(centerParams);


	int wheelPosition = GetCachedWheelPosition();
	double step = 0.02; // Adjust the step size as needed

	while(!MoveDetected && running && minimumStrength <= 0.5)
	{
		ThreadParams *maxRightParams = malloc(sizeof(ThreadParams));

		*maxRightParams = (ThreadParams){0, minimumStrength, 0, 10};
		WorkerSetPosition(maxRightParams);

		if(wheelPosition==GetCachedWheelPosition())
			minimumStrength += step;
		else{
			MoveDetected=true;
			debug(3, "Wheel Effective Min Strength: %f\n", minimumStrength);
		}
	}
	if (!MoveDetected) {
		minimumStrength = 0.0;
		if (running)
			debug(0, "Minimum-force calibration found no movement; automatic boost disabled.\n");
	}
}

void FFBCreateHapticConstantEffects()
{
	/* --- FF_CONSTANT --- */
	struct ff_effect* effect=&ffb_effects[constant_effect_idx];
	memset(effect,0,sizeof(ffb_effects[0]));
	
	effect->id = -1;
    effect->trigger.button = 0;
    effect->trigger.interval = 0;
    effect->replay.length = 0;
    effect->replay.delay = 0;
    effect->direction = 0x4000;

	effect->type = FF_CONSTANT;

	effect->u.constant.level  = 0x6000;
	effect->u.constant.envelope.attack_length = 0;
	effect->u.constant.envelope.attack_level = 0;		
	effect->u.constant.envelope.fade_length = 0;
	effect->u.constant.envelope.fade_level = 0;			

	if(ioctl(device_handle, EVIOCSFF, effect))
		debug(2," Error creating FF_CONSTANT synceffect (%s) [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
	else{
		supportedFeatures|=FF_CONSTANT_LOADED;
		debug(2, "FF_CONSTANT sync Effect id=%d\n", effect->id);
	}

	/* --- FF_CONSTANT ASYNC --- */
	struct ff_effect* effectAsync=&ffb_effects[constant_effect_async_idx];
	memcpy(effectAsync, effect, sizeof(ffb_effects[0]));

	effectAsync->id = -1;

	if(ioctl(device_handle, EVIOCSFF, effectAsync))
		debug(2," Error creating FF_CONSTANT async effect (%s) [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
	else{
		supportedFeatures|=FF_CONSTANT_ASYNC_LOADED;
		debug(2, "FF_CONSTANT async Effect id=%d\n", effectAsync->id);
	}
}

void FFBCreateHapticSineEffect()
{
	/* --- FF_SINE --- */
	struct ff_effect* effect=&ffb_effects[sine_effect_idx];
	memset(effect,0,sizeof(ffb_effects[0]));
	
    effect->id = -1;
    effect->trigger.button = 0;
    effect->trigger.interval = 0;
    effect->replay.length = 0;
    effect->replay.delay = 0;
    effect->direction = 0x4000;

	effect->type = FF_PERIODIC;
	effect->u.periodic.waveform = FF_SINE;	

	effect->u.periodic.period = 1000;
	effect->u.periodic.magnitude = 0x6000;
	effect->u.periodic.offset = 0;
	effect->u.periodic.phase = 0;
	effect->u.periodic.envelope.attack_length = 0;
	effect->u.periodic.envelope.attack_level = 0;
	effect->u.periodic.envelope.fade_length = 0;
	effect->u.periodic.envelope.fade_level = 0;



	if(ioctl(device_handle, EVIOCSFF, effect) < 0)
		debug(2," Error creating FF_PERIODIC FF_SINE effect (%s) [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
	else
	{
		supportedFeatures|=FF_SINE_LOADED;
		debug(2, "FF_SINE Effect     id=%d\n", effect->id);
	}
}

void FFBCreateHapticFrictionEffect()
{
	/* --- FF_FRICTION --- */
	struct ff_effect* effect=&ffb_effects[friction_effect_idx];
	memset(effect,0,sizeof(ffb_effects[0]));
	
    effect->id = -1;
    effect->trigger.button = 0;
    effect->trigger.interval = 0;
    effect->replay.length = 0;
    effect->replay.delay = 0;
    effect->direction = 0x4000;

	effect->type = FF_FRICTION;

	effect->u.condition[0].left_saturation = 0xffff;
	effect->u.condition[0].right_saturation = 0xffff;
	effect->u.condition[0].left_coeff = 0x4000;
	effect->u.condition[0].right_coeff = 0x4000;
	effect->u.condition[0].deadband = 0;
	effect->u.condition[0].center = 0;

	if(ioctl(device_handle, EVIOCSFF, effect))
		debug(2," Error creating FF_FRICTION  effect (%s) [%s:%d]\n", strerror(errno), __FILE__, __LINE__);	
	else{
		supportedFeatures|=FF_FRICTION_LOADED;
		debug(2, "FF_FRICTION Effect id=%d\n", effect->id);		
	}
}

void  FFBCreateHapticDamperEffect()
{
	/* --- FF_DAMPER --- */
	struct ff_effect* effect=&ffb_effects[damper_effect_idx];
	memset(effect,0,sizeof(ffb_effects[0]));
	
	effect->id = -1;
    effect->trigger.button = 0;
    effect->trigger.interval = 0;
    effect->replay.length = 0;
    effect->replay.delay = 0;
    effect->direction = 0x4000; // 0x4000; // 0x8000 -> left, 0xC000-> right

	effect->type = FF_DAMPER;

	effect->u.condition[0].left_saturation = 0xffff;
	effect->u.condition[0].right_saturation = 0xffff;
	effect->u.condition[0].left_coeff = 0x4000;
	effect->u.condition[0].right_coeff = 0x4000;
	effect->u.condition[0].deadband = 0;
	effect->u.condition[0].center = 0;

	if(ioctl(device_handle, EVIOCSFF, effect))
		debug(2," Error creating FF_DAMPER  effect (%s) [%s:%d]\n", strerror(errno), __FILE__, __LINE__);	
	else{
		supportedFeatures|=FF_DAMPER_LOADED;
		debug(2, "FF_DAMPER Effect   id=%d\n", effect->id);	
	}
}

void FFBCreateHapticSpringEffects()
{
	/* --- FF_SPRING --- */
	struct ff_effect* effect=&ffb_effects[spring_effect_idx];
	memset(effect,0,sizeof(ffb_effects[0]));
	
    effect->id = -1;
    effect->trigger.button = 0;
    effect->trigger.interval = 0;
    effect->replay.length = 0;
    effect->replay.delay = 0;
    effect->direction = 0x4000;

	effect->type = FF_SPRING;

	effect->u.condition[0].left_saturation = 0xffff;
	effect->u.condition[0].right_saturation = 0xffff;
	effect->u.condition[0].left_coeff = 0x4000;
	effect->u.condition[0].right_coeff = 0x4000;
	effect->u.condition[0].deadband = 0;
	effect->u.condition[0].center = 0;

	if(ioctl(device_handle, EVIOCSFF, effect))
		debug(2," Error creating FF_SPRING sync effect (%s) [%s:%d]\n", strerror(errno), __FILE__, __LINE__);		
	else{
		supportedFeatures|=FF_SPRING_LOADED;
		debug(2, "FF_SPRING Effect sync id=%d\n", effect->id);	
	}

	/* --- FF_SPRING ASYNC --- */
	struct ff_effect* effectAsync=&ffb_effects[spring_effect_async_idx];
	memset(effectAsync,0,sizeof(ffb_effects[0]));
	memcpy(effectAsync, effect, sizeof(ffb_effects[0]));

	effectAsync->id = -1;

	if(ioctl(device_handle, EVIOCSFF, effectAsync))
		debug(2," Error creating FF_SPRING async effect (%s) [%s:%d]\n", strerror(errno), __FILE__, __LINE__);		
	else{
		supportedFeatures|=FF_SPRING_ASYNC_LOADED;
		debug(2, "FF_SPRING Effect async id=%d\n", effectAsync->id);	
	}

}

void  FFBCreateHapticRumbleEffect()
{
	/* --- FF_RUMBLE --- */
	struct ff_effect* effect=&ffb_effects[rumble_effect_idx];
	memset(effect,0,sizeof(ffb_effects[0]));
	
    effect->id = -1;
    effect->trigger.button = 0;
    effect->trigger.interval = 0;
    effect->replay.length = 0;
    effect->replay.delay = 0;
    effect->direction = 0x4000;

	effect->type = FF_RUMBLE;

	effect->u.rumble.strong_magnitude = 0x6000; 
	effect->u.rumble.weak_magnitude = 0x2000;   

	if(ioctl(device_handle, EVIOCSFF, effect))
		debug(2," Error creating FF_RUMBLE  effect (%s) [%s:%d]\n", strerror(errno), __FILE__, __LINE__);		
	else{
		supportedFeatures|=FF_RUMBLE_LOADED;
		debug(2, "FF_RUMBLE Effect   id=%d\n", effect->id);	
	}
}

void FFBCreateHapticInertiaEffect()
{
	/* --- FF_INERTIA --- */
	struct ff_effect* effect = &ffb_effects[inertia_effect_idx];
	memset(effect, 0, sizeof(ffb_effects[0]));

    effect->id = -1;
    effect->trigger.button = 0;
    effect->trigger.interval = 0;
    effect->replay.length = 0;
    effect->replay.delay = 0;
    effect->direction = 0x4000;

	effect->type = FF_INERTIA;

	effect->u.condition[0].left_saturation = 0xffff;
	effect->u.condition[0].right_saturation = 0xffff;
	effect->u.condition[0].left_coeff = 0x4000;
	effect->u.condition[0].right_coeff = 0x4000;
	effect->u.condition[0].deadband = 0;
	effect->u.condition[0].center = 0;

	if (ioctl(device_handle, EVIOCSFF, effect))
		debug(2, " Error creating FF_INERTIA effect (%s) [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
	else {
		supportedFeatures |= FF_INERTIA_LOADED;
		debug(2, "FF_INERTIA Effect id=%d\n", effect->id);
	}
}

void FFBCreateHapticRampEffect()
{
	/* --- FF_RAMP --- */
	struct ff_effect* effect = &ffb_effects[ramp_effect_idx];
	memset(effect, 0, sizeof(ffb_effects[0]));

	effect->id = -1;
	effect->type = FF_RAMP;

	effect->trigger.button = 0;
	effect->trigger.interval = 0;
	effect->replay.length = 1000;
	effect->replay.delay = 0;
	effect->direction = 0x4000; /* along X axis */

	/* default ramp from 0 to 0 (will be updated when uploaded) */
    effect->u.ramp.start_level = 0x0000;
    effect->u.ramp.end_level = 0x6000;
    effect->u.ramp.envelope.attack_length = 0;
    effect->u.ramp.envelope.attack_level = 0;
    effect->u.ramp.envelope.fade_length = 0;
    effect->u.ramp.envelope.fade_level = 0;

	if (ioctl(device_handle, EVIOCSFF, effect))
		debug(2, " Error creating FF_RAMP effect (%s) [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
	else {
		supportedFeatures |= FF_RAMP_LOADED;
		debug(2, "FF_RAMP Effect id=%d\n", effect->id);
	}
}

void FFBCreateHapticSquareEffect()
{
	/* --- FF_SQUARE (periodic) --- */
	struct ff_effect* effect = &ffb_effects[square_effect_idx];
	memset(effect, 0, sizeof(ffb_effects[0]));

	effect->id = -1;
	effect->type = FF_PERIODIC;

	effect->trigger.button = 0;
	effect->trigger.interval = 0;
	effect->replay.length = 1000;
	effect->replay.delay = 0;
	effect->direction = 0x4000; /* along X axis */

	effect->u.periodic.waveform = FF_SQUARE;
	effect->u.periodic.period = 100;       /* 100 ms default */
	effect->u.periodic.magnitude = 0x6000; /* default magnitude */
	effect->u.periodic.offset = 0;
	effect->u.periodic.phase = 0;
	effect->u.periodic.envelope.attack_length = 0;
	effect->u.periodic.envelope.fade_length = 0;
	effect->u.periodic.envelope.attack_level = 0;
	effect->u.periodic.envelope.fade_level = 0;

	if (ioctl(device_handle, EVIOCSFF, effect))
		debug(2, " Error creating FF_PERIODIC FF_SQUARE effect (%s) [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
	else {
		supportedFeatures |= FF_SQUARE_LOADED;
		debug(2, "FF_SQUARE Effect id=%d\n", effect->id);
	}
}

void FFBCreateHapticTriangleEffect()
{
	/* --- FF_TRIANGLE (periodic) --- */
	struct ff_effect* effect = &ffb_effects[triangle_effect_idx];
	memset(effect, 0, sizeof(ffb_effects[0]));

	effect->id = -1;
	effect->type = FF_PERIODIC;

	effect->trigger.button = 0;
	effect->trigger.interval = 0;
	effect->replay.length = 1000;
	effect->replay.delay = 0;
	effect->direction = 0x4000; /* along X axis */

	effect->u.periodic.waveform = FF_TRIANGLE;
	effect->u.periodic.period = 100;       /* 100 ms default */
	effect->u.periodic.magnitude = 0x6000; /* default magnitude */
	effect->u.periodic.offset = 0;
	effect->u.periodic.phase = 0;
	effect->u.periodic.envelope.attack_length = 0;
	effect->u.periodic.envelope.fade_length = 0;
	effect->u.periodic.envelope.attack_level = 0;
	effect->u.periodic.envelope.fade_level = 0;

	if (ioctl(device_handle, EVIOCSFF, effect))
		debug(2, " Error creating FF_PERIODIC FF_TRIANGLE effect (%s) [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
	else {
		supportedFeatures |= FF_TRIANGLE_LOADED;
		debug(2, "FF_TRIANGLE Effect id=%d\n", effect->id);
	}
}

void FFBCreateHapticSawUpEffect()
{
	/* --- FF_SAW_UP (periodic) --- */
	struct ff_effect* effect = &ffb_effects[sawtoothup_effect_idx];
	memset(effect, 0, sizeof(ffb_effects[0]));

	effect->id = -1;
	effect->type = FF_PERIODIC;

	effect->trigger.button = 0;
	effect->trigger.interval = 0;
	effect->replay.length = 1000;
	effect->replay.delay = 0;
	effect->direction = 0x4000; /* along X axis */

	effect->u.periodic.waveform = FF_SAW_UP;
	effect->u.periodic.period = 100;       /* 100 ms default */
	effect->u.periodic.magnitude = 0x6000; /* default magnitude */
	effect->u.periodic.offset = 0;
	effect->u.periodic.phase = 0;
	effect->u.periodic.envelope.attack_length = 0;
	effect->u.periodic.envelope.fade_length = 0;
	effect->u.periodic.envelope.attack_level = 0;
	effect->u.periodic.envelope.fade_level = 0;

	if (ioctl(device_handle, EVIOCSFF, effect))
		debug(2, " Error creating FF_PERIODIC FF_SAW_UP effect (%s) [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
	else {
		supportedFeatures |= FF_SAW_UP_LOADED;
		debug(2, "FF_SAW_UP Effect id=%d\n", effect->id);
	}
}

void FFBCreateHapticSawDownEffect()
{
	/* --- FF_SAW_DOWN (periodic) --- */
	struct ff_effect* effect = &ffb_effects[sawtoothdown_effect_idx];
	memset(effect, 0, sizeof(ffb_effects[0]));

	effect->id = -1;
	effect->type = FF_PERIODIC;

	effect->trigger.button = 0;
	effect->trigger.interval = 0;
	effect->replay.length = 1000;
	effect->replay.delay = 0;
	effect->direction = 0x4000; /* along X axis */

	effect->u.periodic.waveform = FF_SAW_DOWN;
	effect->u.periodic.period = 100;       /* 100 ms default */
	effect->u.periodic.magnitude = 0x6000; /* default magnitude */
	effect->u.periodic.offset = 0;
	effect->u.periodic.phase = 0;
	effect->u.periodic.envelope.attack_length = 0;
	effect->u.periodic.envelope.fade_length = 0;
	effect->u.periodic.envelope.attack_level = 0;
	effect->u.periodic.envelope.fade_level = 0;

	if (ioctl(device_handle, EVIOCSFF, effect))
		debug(2, " Error creating FF_PERIODIC FF_SAW_DOWN effect (%s) [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
	else {
		supportedFeatures |= FF_SAW_DOWN_LOADED;
		debug(2, "FF_SAW_DOWN Effect id=%d\n", effect->id);
	}
}

void FFBCreateAllHapticEffects()
{
    for (int i = 0; i < MAX_EFFECTS; ++i) ffb_effects[i].id = -1;
	FFBSetGlobalGain(getConfig()->globalGain);
	FFBSetGlobalAutoCenter(0, 0);

	FFBCreateHapticConstantEffects();
	FFBCreateHapticSineEffect();
	FFBCreateHapticFrictionEffect();
	FFBCreateHapticDamperEffect();
	FFBCreateHapticSpringEffects();
	FFBCreateHapticRumbleEffect();

	FFBCreateHapticInertiaEffect();
	FFBCreateHapticRampEffect();
	FFBCreateHapticSquareEffect();
	FFBCreateHapticTriangleEffect();
	FFBCreateHapticSawUpEffect();
	FFBCreateHapticSawDownEffect();
}

void FFBAbortExecution(void)
{
    running = 0;
    FFBStopAllEffects();
    if (device_handle >= 0) {
        close(device_handle);
        device_handle = -1;
    }
}

char* FFBCheckEffect(unsigned int check)
{
	if (check==(supportedFeatures & check)) 
        return GREEN "-> OK <-" RESET "\n";
    else
        return RED "Not supported" RESET "\n";
}

void FFBDumpSupportedFeatures()
{
	if(IsLogitechWheel())
	{
		debug(0, "------------------------------------------------------------------\n");
        debug(0, "-- Logitech wheel detected, sysfs entries:\n");
        debug(0, "------------------------------------------------------------------\n");
		debug(0, "      - range: %u\n",GetSYSFSEntry("range"));
		debug(0, "      - gain: %u\n",GetSYSFSEntry("gain"));
		debug(0, "      - spring_level: %u\n",GetSYSFSEntry("spring_level"));
		debug(0, "      - friction_level: %u\n",GetSYSFSEntry("friction_level"));
		debug(0, "      - autocenter: %u\n",GetSYSFSEntry("autocenter"));
		debug(0, "      - damper_level: %u\n",GetSYSFSEntry("damper_level"));
		debug(0, "      - peak_ffb_level: %u\n",GetSYSFSEntry("peak_ffb_level"));
		debug(0, "      - alternate_modes: %u\n",GetSYSFSEntry("alternate_modes"));	
		debug(0, "      - combine_pedals: %u\n",GetSYSFSEntry("combine_pedals"));
		debug(0, "      - ffb_leds: %u\n",GetSYSFSEntry("ffb_leds"));
		debug(0, "\n");
	}

    debug(0, "------------------------------------------------------------------\n");
    debug(0, "-- Checking capabilities:\n");
    debug(0, "------------------------------------------------------------------\n");
	debug(0, "   Wheel range: 0-%d\n", WheelEffectiveMaxPosition);
	debug(0, "\n");
	int n_effects;	
	if(ioctl(device_handle, EVIOCGEFFECTS, &n_effects))
		debug(1," Error getting Nbr of programmable effects (%s) [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
	else
    	debug(0, "   Nbr of programmable effects for this device: %d\n", n_effects);

	if(ioctl(device_handle, EVIOCGEFFECTS, &n_effects))
		debug(1," Error getting Nbr simultaneous effects (%s) [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
	else
    	debug(0, "   Nbr of simultaneous effects the device can play: %d\n",n_effects);

    debug(0, "\n");
    debug(0, "     ffb_supported constant effect:\n");
    debug(0, "      - constant:     %s", FFBCheckEffect(FF_CONSTANT_LOADED));
    debug(0, "\n");
	debug(0, "     ffb_supported periodic effects:\n");
    debug(0, "      - sine:         %s", FFBCheckEffect(FF_SINE_LOADED));
    debug(0, "      - square:       %s", FFBCheckEffect(FF_SQUARE_LOADED));
    debug(0, "      - triangle:     %s", FFBCheckEffect(FF_TRIANGLE_LOADED));
    debug(0, "      - saw_up:       %s", FFBCheckEffect(FF_SAW_UP_LOADED));
    debug(0, "      - saw_down:     %s", FFBCheckEffect(FF_SAW_DOWN_LOADED));
    debug(0, "\n");
    debug(0, "     ffb_supported condition effects:\n");
    debug(0, "      - spring:       %s", FFBCheckEffect(FF_SPRING_LOADED));
	debug(0, "      - damper:       %s", FFBCheckEffect(FF_DAMPER_LOADED));
    debug(0, "      - friction:     %s", FFBCheckEffect(FF_FRICTION_LOADED));
	debug(0, "      - inertia:      %s", FFBCheckEffect(FF_INERTIA_LOADED));
    debug(0, "\n");
	debug(0, "     ffb_supported RAMP effect:\n");
	debug(0, "      - ramp:         %s", FFBCheckEffect(FF_RAMP_LOADED));
    debug(0, "\n");
    debug(0, "     ffb_supported global features:\n");
    debug(0, "      - gain:        %s", FFBCheckEffect(FF_GAIN_LOADED));
    debug(0, "\n");
	debug(0, "     Rumble ffb_supported:\n");
    debug(0, "      - rumble:      %s", FFBCheckEffect(FF_RUMBLE_LOADED));
}

void FFBStopEffect(int effect_id)
{
    if (effect_id < 0 || device_handle < 0) return;
    struct input_event command = {.type = EV_FF, .code = effect_id, .value = 0};
    if (write(device_handle, &command, sizeof(command)) != sizeof(command))
        debug(0, "ERROR: stopping effect %d failed (%s)\n", effect_id, strerror(errno));
    if (effect_id == ffb_effects[constant_effect_idx].id) constantPlaying[0] = false;
    if (effect_id == ffb_effects[constant_effect_async_idx].id) constantPlaying[1] = false;
}

void FFBStopAllEffects()
{
    FFBCancelCalibration();
    if (device_handle >= 0) {
        for (int i = 0; i < MAX_EFFECTS; ++i) FFBStopEffect(ffb_effects[i].id);
        FFBSetGlobalAutoCenter(0, 0);
    }
    // The next identical bulk packet must be able to restart stopped effects.
    memset(previous_rawpacket, 0, sizeof(previous_rawpacket));
}

void FFBRemoveEffect(int effect_id)
{
    if (ioctl(device_handle, EVIOCRMFF, effect_id) < 0)
        debug(1, "ERROR: removing effect failed (%s) [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
}

void FFBRemoveAllEffects()
{
	if(device_handle)
	{
		for(int cp=0; cp < sizeof(ffb_effects)/sizeof(struct ff_effect); cp++)
			FFBRemoveEffect(ffb_effects[cp].id);
	}
}


// FFBTriggerSineEffect applies or updates a force feedback sine wave effect on a device, 
// configuring its frequency and intensity, and uploading the effect if supported. 
// The function manages effect parameters, uploads them to the device, and handles starting or stopping the effect as needed.
void FFBTriggerSineEffect(bool upload, float frequency, float intensity)
{
    struct input_event event;
	debug(1, "FFBTriggerSineEffect");
	if(FF_SINE_LOADED==(supportedFeatures & FF_SINE_LOADED)) 
	{
		debug(2, " -> arg_frequency: %.2f, arg_intensity: %.2f\n", frequency, intensity);

		struct ff_effect* sineEffect=&ffb_effects[sine_effect_idx];
		if(upload)
		{
			sineEffect->u.periodic.period = frequency > 0 ? (unsigned short)CLAMP(4000.0f / frequency, 1, 65535) : 1; // period in milliseconds

			debug(3, " -> frequency converted to period: %ums\n", sineEffect->u.periodic.period);

			if (intensity < -1.0f) intensity = -1.0f;
			if (intensity > 1.0f) intensity = 1.0f;

			short MinIntensity = (short)(getConfig()->minIntensity / 100.0 * 32767.0);
			short MaxIntensity = (short)(getConfig()->maxIntensity / 100.0 * 32767.0);

			signed short magnitude;
			if (intensity == 0.0) {
				magnitude = 0;
			} else {
				double absStrength = fabs(intensity);
				magnitude = (short)((absStrength * (MaxIntensity - MinIntensity)) + MinIntensity);
				magnitude = (intensity > 0.0) ? magnitude : -magnitude;
			}

			sineEffect->u.periodic.magnitude = magnitude;
			debug(3, " -> intensity converted to magnitude: %d\n", sineEffect->u.periodic.magnitude);             

			/* update effect */
			if (ioctl(device_handle, EVIOCSFF, sineEffect) < 0)
				debug(1, "ERROR: uploading effect failed (%s) [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
		}

		
		memset(&event, 0, sizeof(event));
		event.type = EV_FF;
		event.code = sineEffect->id;

		//STOP PREVIOUS EFFECT
		event.value = 0;
		bool rs=write(device_handle, &event, sizeof(event));
		fsync(device_handle);

		if(frequency!=0.0 && intensity!=0.0)
		{
			//START EFFECT
			event.value = 1;
			if (write(device_handle, &event, sizeof(event)) != sizeof(event))
				fprintf(stderr, "ERROR: starting FF_SINE effect failed (%s) [%s:%d]\n",	strerror(errno), __FILE__, __LINE__);

			fsync(device_handle);
		}
	}
	else
	{
		debug(1, " -> sine effect not supported.\n");
	}
}

// FFBTriggerSpringEffect applies or updates a force feedback spring effect on a device, 
// configuring its strength and uploading the effect if supported; if not supported, it falls back to a default rumble effect. 
// The function manages effect parameters, uploads them to the device, and handles starting or stopping the effect as needed.
void FFBTriggerSpringEffect(bool upload, double strength, bool async)
{
    struct input_event event;
	debug(1, "FFBTriggerSpringEffect\n");
	if(FF_SPRING_LOADED==(supportedFeatures & FF_SPRING_LOADED)) 
	{
		debug(2, " -> arg_strength: %.2f\n async: %d", strength, async);


		struct ff_effect* springEffect = async
			? &ffb_effects[spring_effect_async_idx]
			: &ffb_effects[spring_effect_idx];

		if(upload)
		{
			unsigned short minForce = (unsigned short)((getConfig()->minSpring / 100.0) * 65535.0); 
			unsigned short maxForce = (unsigned short)((getConfig()->maxSpring / 100.0) * 65535.0);
			unsigned short range = maxForce - minForce;

			unsigned short saturation = (unsigned short)((strength * range) + minForce);

			springEffect->u.condition[0].right_coeff = 16384;
			springEffect->u.condition[0].left_coeff =  16384;
			
			springEffect->u.condition[0].right_saturation = saturation;
			springEffect->u.condition[0].left_saturation =  saturation;

			debug(3, " -> right_coeff: %d, left_coeff: %d\n", springEffect->u.condition[0].right_coeff, springEffect->u.condition[0].left_coeff);
			debug(3, " -> right_saturation: %d, left_saturation: %d\n", springEffect->u.condition[0].right_saturation, springEffect->u.condition[0].left_saturation);

			springEffect->u.condition[1] = springEffect->u.condition[0];

			/* update effect */
			if (ioctl(device_handle, EVIOCSFF, springEffect) < 0)
				debug(1, "ERROR: uploading effect failed (%s) [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
		}

		
		memset(&event, 0, sizeof(event));
		event.type = EV_FF;
		event.code = springEffect->id;

		//STOP PREVIOUS EFFECT
		event.value = 0;
		bool rs=write(device_handle, &event, sizeof(event));
		fsync(device_handle);

		if(strength!=0.0)
		{
			//START EFFECT
			event.value = 1;

			if (write(device_handle, &event, sizeof(event)) != sizeof(event))
				fprintf(stderr, "ERROR: starting FF_CONSTANT effect failed (%s) [%s:%d]\n",	strerror(errno), __FILE__, __LINE__);

			fsync(device_handle);
		}
	}
	else
	{
		debug(1, " -> spring effect not supported.\n");
	}
}

/**
 * Generic force-feedback effect envelope (struct ff_envelope):
 *   @attack_length: duration of the attack (ms)
 *   @attack_level:  level at the beginning of the attack
 *   @fade_length:   duration of fade (ms)
 *   @fade_level:    level at the end of fade
 *  Constant:
 *   @level:         strength of the effect; may be negative
 */
/* A streamed constant is one continuously playing effect. EVIOCSFF updates it
 * in place; restarting on every sample inserts gaps and resets driver timing. */
void FFBTriggerConstantEffect(bool upload, double strength, bool async)
{
    unsigned feature = async ? FF_CONSTANT_ASYNC_LOADED : FF_CONSTANT_LOADED;
    if (!(supportedFeatures & feature)) return;
    struct ff_effect *constant = &ffb_effects[async ? constant_effect_async_idx : constant_effect_idx];
    int slot = async ? 1 : 0;
    strength = isfinite(strength) ? CLAMP(strength, -1.0, 1.0) : 0.0;

    // The board's initialization must overcome physical stiction. Keep that
    // compensation out of the normal, already-rendered Virtual Wheel stream.
    if (!getConfig()->autoMinForce && SegaFFBControllerState != OPENFFB_READY_SUB_CMD && strength != 0.0) {
        double floor = CLAMP(getConfig()->calibrationConstant, 0, 100) / 100.0;
        if (fabs(strength) < floor) strength = strength > 0 ? floor : -floor;
    }

    // Stop before uploading zero: even a failed upload must release old force.
    if (strength == 0.0) FFBStopEffect(constant->id);
    if (upload || !constantPlaying[slot]) {
        int minimum = CLAMP(getConfig()->minConstant, 0, 100);
        int maximum = CLAMP(getConfig()->maxConstant, minimum, 100);
        int minForce = minimum * 32767 / 100;
        int maxForce = maximum * 32767 / 100;
        int magnitude = (int)(fabs(strength) * (maxForce - minForce) + minForce);
        constant->u.constant.level = strength == 0.0 ? 0 : (strength > 0 ? magnitude : -magnitude);
        memset(&constant->u.constant.envelope, 0, sizeof(constant->u.constant.envelope));
        if (ioctl(device_handle, EVIOCSFF, constant) < 0) {
            debug(0, "ERROR: updating constant effect failed (%s)\n", strerror(errno));
            FFBStopEffect(constant->id);
            return;
        }
    }
    if (strength != 0.0 && !constantPlaying[slot]) {
        struct input_event command = {.type = EV_FF, .code = constant->id, .value = 1};
        if (write(device_handle, &command, sizeof(command)) == sizeof(command))
            constantPlaying[slot] = true;
        else
            debug(0, "ERROR: starting constant effect failed (%s)\n", strerror(errno));
    }
}

/**
 * @right_saturation: maximum level when joystick moved all way to the right
 * @left_saturation:  same for the left side
 * @right_coeff:      controls how fast the force grows when the joystick moves
 * @left_coeff:       same for the left side 
 *  Direction of the effect is encoded as follows:
 *	  0 deg -> 0x0000 (down)
 *	  90 deg -> 0x4000 (left)
 *	  180 deg -> 0x8000 (up)
 *	  270 deg -> 0xC000 (right)
 */
void FFBTriggerFrictionEffect(bool upload, double strength)
{
    struct input_event event;
	debug(1, "FFBTriggerFrictionEffect\n");
	//debug(1, "FFBTriggerFrictionEffect upload=%d strength=%f\n", upload, strength);
	if(FF_FRICTION_LOADED==(supportedFeatures & FF_FRICTION_LOADED)) 
	{
		struct ff_effect* frictionEffect=&ffb_effects[friction_effect_idx];
		if(upload)
		{
			unsigned short minForce = (unsigned short)((getConfig()->minFriction / 100.0) * 65535.0); 
			unsigned short maxForce = (unsigned short)((getConfig()->maxFriction / 100.0) * 65535.0);
			unsigned short range = maxForce - minForce;

			unsigned short saturation = (unsigned short)((strength * range) + minForce);

			frictionEffect->u.condition[0].left_coeff =  16384;
			frictionEffect->u.condition[0].right_coeff = 16384;

			frictionEffect->u.condition[0].left_saturation = saturation; 
			frictionEffect->u.condition[0].right_saturation = saturation;
			debug(3, " -> left_saturation: %d, right_saturation: %d\n", frictionEffect->u.condition[0].left_saturation, frictionEffect->u.condition[0].right_saturation);

			frictionEffect->u.condition[1] = frictionEffect->u.condition[0];

			/* update effect */
			if (ioctl(device_handle, EVIOCSFF, frictionEffect) < 0)
				debug(1, "ERROR: uploading effect failed (%s) [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
		}

		
		memset(&event, 0, sizeof(event));
		event.type = EV_FF;
		event.code = frictionEffect->id;

		//STOP PREVIOUS EFFECT
		event.value = 0;
		bool rs=write(device_handle, &event, sizeof(event));
		fsync(device_handle);

		if(strength!=0.0)
		{
			//START EFFECT
			event.value = 1;
			if (write(device_handle, &event, sizeof(event)) != sizeof(event))
				fprintf(stderr, "ERROR: starting FF_FRICTION effect failed (%s) [%s:%d]\n",	strerror(errno), __FILE__, __LINE__);

			fsync(device_handle);
		}
	}
	else
	{
		debug(1, " -> friction effect not supported.\n");
	}
}

void FFBTriggerRumbleEffectDefault(bool upload, double strength)
{
	FFBTriggerRumbleEffect(upload, strength, both_motors);
}

void FFBTriggerRumbleEffect(bool upload, double strength, motor_select motor)
{
    struct input_event event;

	debug(1, "FFBTriggerRumbleEffect\n");
	if(FF_RUMBLE_LOADED==(supportedFeatures & FF_RUMBLE_LOADED)) 
	{
		struct ff_effect* rumbleEffect=&ffb_effects[rumble_effect_idx];
		if(upload)
		{
			if (strength > 1.0)
				strength = 1.0;
			else if (strength < -1.0)
				strength = -1.0;

			unsigned short minForce = (unsigned short)((getConfig()->minConstant / 100.0) * 65535.0); 
			unsigned short maxForce = (unsigned short)((getConfig()->maxConstant / 100.0) * 65535.0);
			unsigned short range = maxForce - minForce;
			
			unsigned short level = (unsigned short)((strength * range) + minForce);

			switch (motor) {
				case weak_motor:
					rumbleEffect->u.rumble.strong_magnitude = 0;
					rumbleEffect->u.rumble.weak_magnitude = level/3;
					break;
				case strong_motor:
					rumbleEffect->u.rumble.strong_magnitude = level;
					rumbleEffect->u.rumble.weak_magnitude = 0;
					break;
				case both_motors:
				default:
					rumbleEffect->u.rumble.strong_magnitude = level;
					rumbleEffect->u.rumble.weak_magnitude = level/3;
					break;
			}

			/* update effect */
			if (ioctl(device_handle, EVIOCSFF, rumbleEffect) < 0)
				debug(1, "ERROR: uploading effect failed (%s) [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
		}

		
		memset(&event, 0, sizeof(event));
		event.type = EV_FF;
		event.code = rumbleEffect->id;

		//STOP PREVIOUS EFFECT
		event.value = 0;
		bool rs=write(device_handle, &event, sizeof(event));

		if (strength!=0.0)
		{
			//START EFFECT
			event.value = 1;
			if (write(device_handle, &event, sizeof(event)) != sizeof(event))
				fprintf(stderr, "ERROR: starting FF_RUMBLE effect failed (%s) [%s:%d]\n",	strerror(errno), __FILE__, __LINE__);

			write(device_handle, &event, sizeof(event)) != sizeof(event);
		}
	}
	else
	{
		debug(1, " -> rumble effect not supported\n");
	}	
}

/* --- set flobal gain for all effects (1-100) --- */
void FFBSetGlobalGain(int level)
{
    struct input_event gain;
	debug(1, "FFBSetGlobalGain\n");

	memset(&gain, 0, sizeof(gain));
	gain.type = EV_FF;
	gain.code = FF_GAIN;
	gain.value = 65535 * CLAMP(level, 0, 100) / 100;
	debug(3, " -> level: %d, gain.value: %d\n", level, gain.value);

	if (write(device_handle, &gain, sizeof(gain)) != sizeof(gain)) 
		debug(1, "Error setting global gain\n");
	else{
		supportedFeatures|=FF_GAIN_LOADED;
	}
}

/* --- AutoCenter Global Setting (1-100) for 1 second only --- */
void FFBSetGlobalAutoCenter(int strength, int duration_ms)
{
    struct input_event event;
	debug(1, "FFBSetGlobalAutoCenter duration: %d ms\n", duration_ms);
	
	memset(&event, 0, sizeof(event));
	event.type = EV_FF;
	event.code = FF_AUTOCENTER;
	event.value = 65535 * CLAMP(strength, 0, 100) / 100;

	/* Enable autocalibration / auto-center */
	if (write(device_handle, &event, sizeof(event)) != sizeof(event))
		debug(1, "ERROR: failed to enable auto centering (%s) [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
	else {
		supportedFeatures |= FF_AUTOCENTER_LOADED;
	}

	/* Keep auto-center for 1 second */
	for (int elapsed = 0; running && elapsed < duration_ms; elapsed += 20)
        usleep((duration_ms - elapsed < 20 ? duration_ms - elapsed : 20) * 1000);

	event.value = 0;
	if (write(device_handle, &event, sizeof(event)) != sizeof(event))
		debug(1, "ERROR: failed to disable auto centering (%s) [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
}

void FFBTriggerTestEffect(unsigned int effect, double strength)
{
    switch(effect)
    {
		ThreadParams *testParams;
        case FF_CONSTANT:
			testParams = malloc(sizeof(ThreadParams));
			*testParams = (ThreadParams){0, strength, 0, 5000};
			startWorkerAsync(WorkerSetPosition, testParams);
            break;
        case FF_SPRING:
            FFBTriggerSpringEffect(true, strength, false);
            break;
        case FF_FRICTION:
            FFBTriggerFrictionEffect(true, strength);
            break;
        case FF_AUTOCENTER:
			testParams = malloc(sizeof(ThreadParams));
			*testParams = (ThreadParams){0, 0.84, 0, 50000};
			FFBStartCalibration(WorkerSetCenter, testParams);
            break;              
        case FF_RUMBLE:
            FFBTriggerRumbleEffectDefault(true, strength);
            break;  
		case FF_SINE:
            FFBTriggerSineEffect(true, 19,0.20);
            break;  						
    }
}

char logitechSysFsDirectory[256];

unsigned short GetSYSFSEntry(const char* entryName)
{
	char logitechSysFsFile[256];
	strcpy(logitechSysFsFile, logitechSysFsDirectory);
  	strcat(logitechSysFsFile, entryName);

	debug(3, "Opening file %s to read value\n", logitechSysFsFile);
	FILE * file = fopen(logitechSysFsFile,"r");

	unsigned short value=0;

	if(file!=0){
		fscanf(file, "%hu", &value);
		fclose (file);
		debug(3, "Value read: %hu\n", value);
	}
	else{
		debug(3, "can not read value, did you start it with 'sudo'?\n");
	}

	return value;
}

void SetSYSFSEntry(const char* entryName, int entryValue)
{
	char logitechSysFsFile[256];
	strcpy(logitechSysFsFile, logitechSysFsDirectory);
  	strcat(logitechSysFsFile, entryName);

	debug(3, "Opening file %s to set value %d\n", logitechSysFsFile, entryValue);
	FILE * file = fopen(logitechSysFsFile,"w");

	if(file!=0){
		fprintf(file,"%d",  entryValue);
		fclose (file);
		debug(3, "Value updated\n");
	}
	else{
		debug(3, "can not set value, did you start it with 'sudo'?\n");
	}
}

bool SetLogitechSysFsDirectory(int idxDevice)
{
	printf("SetLogitechSysFsDirectory ... \n");
	bool rs=false;

    struct udev *udev;
    struct udev_enumerate *enumerate;
    struct udev_list_entry *udevices, *dev_list_entry;
    struct udev_device *dev;

    udev = udev_new();

    enumerate = udev_enumerate_new(udev);
    udev_enumerate_add_match_subsystem(enumerate, "input");
    udev_enumerate_scan_devices(enumerate);
    udevices = udev_enumerate_get_list_entry(enumerate);

	bool exitEnum;

    udev_list_entry_foreach(dev_list_entry, udevices) {
        const char *path;

        path = udev_list_entry_get_name(dev_list_entry);
        dev = udev_device_new_from_syspath(udev, path);

		if(udev_device_get_devnode(dev) && strcmp(udev_device_get_devnode(dev),devices[idxDevice].path)==0)
		{
			debug(2, "Logitech device found, storing SYSFS path...\n");

			strcpy(logitechSysFsDirectory, path);
  			strcat(logitechSysFsDirectory, "/device/device/");

			rs=true;

			break;
		}

        udev_device_unref(dev);
    }

    udev_enumerate_unref(enumerate);
    udev_unref(udev);

	return rs;
}
