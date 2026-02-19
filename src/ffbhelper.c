    
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

/*
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
*/
#define LONG_BITS (sizeof(long) * 8)
struct ff_effect effect;

bool LogitechWheelDetected=false;

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

int GetWheelPositionIOCTL()
{
	int wheelPosition=-1;
	struct input_absinfo absinfo;

	if (ioctl(device_handle, EVIOCGABS(ABS_X), &absinfo) >= 0) {
		wheelPosition = absinfo.value;
	}

	return wheelPosition;
}
/*
ABS_X: max right = 16382
      mmax_left  = 0 
*/
int GetWheelPosition()
{
	struct input_event event;
    fd_set file_descriptor;
    struct timeval tv;

	FD_ZERO(&file_descriptor);
    FD_SET(device_handle, &file_descriptor);

    tv.tv_sec = 0;
    tv.tv_usec = 2 * 1000;

    if (select(device_handle + 1, &file_descriptor, NULL, NULL, &tv) < 1)
        return -1;

	int bytesRead = read(device_handle, &event, sizeof(struct input_event));

	if (bytesRead == sizeof(struct input_event)) {
		if (event.type == EV_ABS && event.code == ABS_X) {
			return event.value;
		}
	}
	return -1; // Indicate an error or no position available
}

bool FFBInitHaptic(char* device_name)
{
	if(FFBGetAllDevices()) {
		int idxDevice=FFBGetDeviceIdx(device_name);

		if(idxDevice> -1 && devices[idxDevice].path){
			unsigned char ffb_supported[1 + FF_MAX/8/sizeof(unsigned char)];

			device_handle = open(devices[idxDevice].path, O_RDWR|O_NONBLOCK);

			if (device_handle > -1) {
				debug(2, "Using device %s.\n\n", device_name);

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
				FFBSetGlobalGain(getConfig()->globalGain);

				FFBSetGlobalAutoCenter(40,2000); 

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

void FFBCreateHapticConstantEffect()
{
	/* --- FF_CONSTANT --- */
	struct ff_effect* effect=&ffb_effects[constant_effect_idx];
	memset(effect,0,sizeof(ffb_effects[0]));
	
	effect->id = -1;
	effect->type = FF_CONSTANT;

	effect->trigger.button = 0;
	effect->trigger.interval = 0;
	effect->replay.length = 1000; 
	effect->replay.delay = 0;
	effect->direction = 0xC000;
	
	effect->u.constant.level  = 0;
	effect->u.constant.envelope.attack_length = 0;
	effect->u.constant.envelope.attack_level = 0;		
	effect->u.constant.envelope.fade_length = 0;
	effect->u.constant.envelope.fade_level = 0;			

	if(ioctl(device_handle, EVIOCSFF, effect))
		debug(2," Error creating FF_CONSTANT effect (%s) [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
	else{
		supportedFeatures|=FF_CONSTANT_LOADED;
		debug(2, "FF_CONSTANT Effect id=%d\n", effect->id);
	}
}

void FFBCreateHapticSineEffect()
{
	/* --- FF_SINE --- */
	struct ff_effect* effect=&ffb_effects[sine_effect_idx];
	memset(effect,0,sizeof(ffb_effects[0]));
	
	effect->id = -1;
	effect->type = FF_PERIODIC;
	

	effect->trigger.button = 0;
	effect->trigger.interval = 0;
	effect->replay.length = 0;
	effect->replay.delay = 0;
	effect->direction = 16384;	

	effect->u.periodic.waveform = FF_SINE;
	effect->u.periodic.period = 100;		// 0.1 second 
	effect->u.periodic.magnitude = 0x6000;	// 
	effect->u.periodic.offset = 0;
	effect->u.periodic.phase = 0;

	effect->u.periodic.envelope.attack_length = 0;
	effect->u.periodic.envelope.attack_level = 0;
	effect->u.periodic.envelope.fade_length = 0;
	effect->u.periodic.envelope.fade_level = 0;

	if(ioctl(device_handle, EVIOCSFF, effect))
		debug(2," Error creating FF_PERIODIC FF_SINE effect (%s) [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
	else{
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
	effect->type = FF_FRICTION;

	effect->trigger.button = 0;
	effect->trigger.interval = 0;
	effect->replay.length = 1000;  
	effect->replay.delay = 0;

	effect->u.condition[0].right_saturation = 0x0;
	effect->u.condition[0].left_saturation = 0x0;
	effect->u.condition[0].right_coeff = 0x0;
	effect->u.condition[0].left_coeff = 0x0;
	effect->u.condition[0].deadband = 0x0;
	effect->u.condition[0].center = 0x0;
	effect->u.condition[1] = effect->u.condition[0];

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
	effect->type = FF_DAMPER;

	effect->trigger.button = 0;
	effect->trigger.interval = 0;
	effect->replay.length = 1000; 
	effect->replay.delay = 0;
	effect->direction = 0x0000; // 0x4000; // 0x8000 -> left, 0xC000-> right

	effect->u.condition->left_saturation = 0;
	effect->u.condition->right_saturation = 0;

	if(ioctl(device_handle, EVIOCSFF, effect))
		debug(2," Error creating FF_DAMPER  effect (%s) [%s:%d]\n", strerror(errno), __FILE__, __LINE__);	
	else{
		supportedFeatures|=FF_DAMPER_LOADED;
		debug(2, "FF_DAMPER Effect   id=%d\n", effect->id);	
	}
}

void FFBCreateHapticSpringEffect()
{
	/* --- FF_SPRING --- */
	struct ff_effect* effect=&ffb_effects[spring_effect_idx];
	memset(effect,0,sizeof(ffb_effects[0]));
	
	effect->id = -1;
	effect->type = FF_SPRING;

	effect->trigger.button = 0;
	effect->trigger.interval = 0;
	effect->replay.length = HAPTIC_INFINITY;  
	effect->replay.delay = 0;

	/* Initialize all condition parameters to zero so the spring loads with no force */
	effect->u.condition[0].deadband = 0x0;
	effect->u.condition[0].center = 0x0;
	effect->u.condition[0].left_saturation = 0x0;
	effect->u.condition[0].right_saturation = 0x0;
	effect->u.condition[0].left_coeff = 0x0;
	effect->u.condition[0].right_coeff = 0x0;
	effect->u.condition[1] = effect->u.condition[0];

	if(ioctl(device_handle, EVIOCSFF, effect))
		debug(2," Error creating FF_SPRING  effect (%s) [%s:%d]\n", strerror(errno), __FILE__, __LINE__);		
	else{
		supportedFeatures|=FF_SPRING_LOADED;
		debug(2, "FF_SPRING Effect   id=%d\n", effect->id);	
	}
}

void  FFBCreateHapticRumbleEffect()
{
	/* --- FF_RUMBLE --- */
	struct ff_effect* effect=&ffb_effects[rumble_effect_idx];
	memset(effect,0,sizeof(ffb_effects[0]));
	
	effect->id = -1;
	effect->type = FF_RUMBLE;

	effect->trigger.button = 0;
	effect->trigger.interval = 0;
	effect->replay.length = 1000; 
	effect->replay.delay = 0;
	effect->direction = 0x4000; // 0x4000 ; // 0x8000 -> left, 0xC000-> right


	effect->u.rumble.strong_magnitude = 0x6000; 
	effect->u.rumble.weak_magnitude = 0x2000;   

	if(ioctl(device_handle, EVIOCSFF, effect))
		debug(2," Error creating FF_RUMBLE  effect (%s) [%s:%d]\n", strerror(errno), __FILE__, __LINE__);		
	else{
		supportedFeatures|=FF_RUMBLE_LOADED;
		debug(2, "FF_RUMBLE Effect   id=%d\n", effect->id);	
	}
}

void FFBCreateAllHapticEffects()
{
	FFBCreateHapticConstantEffect();
	FFBCreateHapticSineEffect();
	FFBCreateHapticFrictionEffect();
	FFBCreateHapticDamperEffect();
	FFBCreateHapticSpringEffect();
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
    debug(1, "\nAborting program execution.\n");
	if(device_handle){
		FFBStopAllEffects();
		//FFBRemoveAllEffects();
		close(device_handle);
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
		memset(&event, 0, sizeof(event));
		event.type = EV_FF;
		event.code = effect_id;
		event.value = 0;

		bool rs=write(device_handle, &event, sizeof(event));
}

void FFBStopAllEffects()
{
	if(device_handle > 0)
	{
		int num_effects = sizeof(ffb_effects)/sizeof(struct ff_effect);
		for(int cp=0; cp < num_effects; cp++)
			FFBStopEffect(ffb_effects[cp].id);

	}
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
	debug(0, "FFBTriggerSineEffect");
	if(FF_SINE_LOADED==(supportedFeatures & FF_SINE_LOADED)) 
	{
debug(0, " -> arg_frequency: %.2f, arg_intensity: %.2f\n", frequency, intensity);

		struct ff_effect* sineEffect=&ffb_effects[sine_effect_idx];
		if(upload)
		{
			// to convert from 0.5-1.0 to 50-100Hz
			//frequency*=(360.0f * getConfig()->periodAdjustmentFactor); 

			//According https://github.com/flyinghead/flycast/blob/master/core/hw/naomi/midiffb.cpp
			// we see that value of 2 = 1Hz and based on max value of 0x7f, we only have a range 127/2Hz => 0.5(0x01) to 64Hz(0x7F)
    		//if (frequency < 0.5f) frequency = 0.5f;
    		//if (frequency > 120.0f) frequency = 120.0f;
			//sineEffect->u.periodic.period= (unsigned short)frequency;
			sineEffect->u.periodic.period= (unsigned short)(1000.0f / (frequency/4.0f)); // period in milliseconds



			// Frequency Math: Calculate period in microseconds => Period = 1000 / Frequency(Hz)
			// For 50Hz: 1000 / 50 = 20ms
			// Clamp value between 5-1000
			// int period_us = (int)(1000.0f / frequency); // period in milliseconds
			// if (period_us < 5) period_us = 5;       // it looks like it's the Minimum period of my G27
			// if (period_us > 1000) period_us = 1000; // Maximum period
			// sineEffect->u.periodic.period = period_us; 

debug(0, " -> frequency converted to period: %ums\n", sineEffect->u.periodic.period);

			// Clamp intensity to -1.0 to 1.0 (maps to -32767 to 32767 evdev range)
			// rg_intensity: 0.16 -> 0.18*32767=5887 // is the minimum on G27 to feel something which corresponds to 0x16 from Sega FFB
			if (intensity < -1.0f) intensity = -1.0f;
			if (intensity > 1.0f) intensity = 1.0f;

			int confMinIntensity = getConfig()->minIntensity;
			int confMaxIntensity = getConfig()->maxIntensity;

			//let's try to calculate amplitude level based on min/max intensity set in game config
			//32767 is max for evdev, it's a signed value, so in theory it could be negative too but it makes no sense for sine wave magnitude
			//short MinIntensity = (short)(intensity > 0.001 ? (confMinIntensity / 100.0 * 32767.0) : 0); // if 20 in config=> 6553.4
			short MinIntensity = (short)(confMinIntensity / 100.0 * 32767.0);
			short MaxIntensity = (short)(confMaxIntensity / 100.0 * 32767.0);
debug(0, " -> minIntensity: %d, maxIntensity: %d\n", MinIntensity, MaxIntensity);
			
			short range = MaxIntensity - MinIntensity; // => 26214
			sineEffect->u.periodic.magnitude = (short)(((intensity/1.0) * range) + MinIntensity);
debug(0, " -> intensity converted to magnitude: %d\n", sineEffect->u.periodic.magnitude);             

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

		//START EFFECT
		event.value = 1;
		if (write(device_handle, &event, sizeof(event)) != sizeof(event))
			fprintf(stderr, "ERROR: starting FF_SINE effect failed (%s) [%s:%d]\n",	strerror(errno), __FILE__, __LINE__);

		write(device_handle, &event, sizeof(event)) != sizeof(event);
	}
	else
	{
		debug(1, " -> sine effect not supported.\n");
	}
}

// FFBTriggerSpringEffect applies or updates a force feedback spring effect on a device, 
// configuring its strength and uploading the effect if supported; if not supported, it falls back to a default rumble effect. 
// The function manages effect parameters, uploads them to the device, and handles starting or stopping the effect as needed.
void FFBTriggerSpringEffect(bool upload, double strength)
{
	debug(1, "FFBTriggerSpringEffect\n");
	if(FF_SPRING_LOADED==(supportedFeatures & FF_SPRING_LOADED)) 
	{
		struct ff_effect* springEffect=&ffb_effects[spring_effect_idx];
		if(upload)
		{
			short minForce = (short)(strength > 0.001 ? (getConfig()->minSpring / 100.0 * 16384.0) : 0); // strength is a double so we do an epsilon check of 0.001 instead of > 0.
			short maxForce = (short)(getConfig()->maxSpring / 100.0 * 16384.0);
			short range = maxForce - minForce;
			short coeff = (short)(strength * range + minForce);
			if (coeff > 16384)
				coeff = 16384;

debug(0, " -> strength: %.2f, coeff: %d\n", strength, coeff);

			springEffect->u.condition[0].right_coeff = (short)(coeff);
			springEffect->u.condition[0].left_coeff = (short)(coeff);
			springEffect->u.condition[0].right_saturation = (unsigned short)(coeff) * 2; 
			springEffect->u.condition[0].left_saturation = (unsigned short)(coeff) * 2; 

debug(0, " -> right_coeff: %d, left_coeff: %d\n", springEffect->u.condition[0].right_coeff, springEffect->u.condition[0].left_coeff);
debug(0, " -> right_saturation: %d, left_saturation: %d\n", springEffect->u.condition[0].right_saturation, springEffect->u.condition[0].left_saturation);

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

		//START EFFECT
		event.value = 1;
		if (write(device_handle, &event, sizeof(event)) != sizeof(event))
			fprintf(stderr, "ERROR: starting FF_CONSTANT effect failed (%s) [%s:%d]\n",	strerror(errno), __FILE__, __LINE__);

		write(device_handle, &event, sizeof(event)) != sizeof(event);
	}
	else
	{
		debug(1, " -> spring effect not supported.\n");
	}
}




void FFBCreateHapticInertiaEffect()
{
	/* --- FF_INERTIA --- */
	struct ff_effect* effect = &ffb_effects[inertia_effect_idx];
	memset(effect, 0, sizeof(ffb_effects[0]));

	effect->id = -1;
	effect->type = FF_INERTIA;

	effect->trigger.button = 0;
	effect->trigger.interval = 0;
	effect->replay.length = 1000;
	effect->replay.delay = 0;
	effect->direction = 0x4000; /* along X axis */

	/* Initialize condition params to zero so inertia loads with no force */
	effect->u.condition[0].deadband = 0x0;
	effect->u.condition[0].center = 0x0;
	effect->u.condition[0].left_saturation = 0xffff;
	effect->u.condition[0].right_saturation = 0xffff;
	effect->u.condition[0].left_coeff = 0x4000;
	effect->u.condition[0].right_coeff = 0x4000;
	effect->u.condition[1] = effect->u.condition[0];

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

//FRED??
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

/**
 * Generic force-feedback effect envelope (struct ff_envelope):
 *   @attack_length: duration of the attack (ms)
 *   @attack_level:  level at the beginning of the attack
 *   @fade_length:   duration of fade (ms)
 *   @fade_level:    level at the end of fade
 *  Constant:
 *   @level:         strength of the effect; may be negative
 */
void FFBTriggerConstantEffect(bool upload, double strength)
{
	debug(1, "FFBTriggerConstantEffect\n");
	if(FF_CONSTANT_LOADED==(supportedFeatures & FF_CONSTANT_LOADED)) 
	{
		struct ff_effect* constantEffect=&ffb_effects[constant_effect_idx];
		if(upload)
		{
			if (strength > 1.0)
				strength = 1.0;
			else if (strength < -1.0)
				strength = -1.0;

			int confMinForce = getConfig()->minTorque;
			int confMaxForce = getConfig()->maxTorque;

			short MinForce = (short)(strength > 0.001 ? (confMinForce / 100.0 * 32767.0) : 0);
			short MaxForce = (short)(getConfig()->maxTorque / 100.0 * 32767.0);
			short range = MaxForce - MinForce;
			short level = (short)(strength * range + MinForce);

			//from -32767 to 32767 (max value of a signed short)
			//the fact is that for my Logitech, effect starts at 10000 until 32767
			//so, it starts from 28->7F
			//a possibility is to set minForce to 25-30 in game profile
			constantEffect->u.constant.level = level;	

			/* Here we set the two values to the max as the arcade system 'manages" fades                */
			constantEffect->u.constant.envelope.attack_level =  (unsigned short)(strength * 65535.0); /* this one counts! */
			constantEffect->u.constant.envelope.fade_level =    (unsigned short)(strength * 65535.0); /* only to be safe  */

			/* update effect */
			if (ioctl(device_handle, EVIOCSFF, constantEffect) < 0)
				debug(1, "ERROR: uploading effect failed (%s) [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
		}

		
		memset(&event, 0, sizeof(event));
		event.type = EV_FF;
		event.code = constantEffect->id;

		//STOP PREVIOUS EFFECT
		event.value = 0;
		bool rs=write(device_handle, &event, sizeof(event));

		//START EFFECT
		event.value = 1;
		if (write(device_handle, &event, sizeof(event)) != sizeof(event))
			fprintf(stderr, "ERROR: starting FF_CONSTANT effect failed (%s) [%s:%d]\n",	strerror(errno), __FILE__, __LINE__);

		write(device_handle, &event, sizeof(event)) != sizeof(event);
	}
	else
	{
		debug(1, " -> constant effect not supported.\n");
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
	debug(1, "FFBTriggerFrictionEffect\n");
	//debug(1, "FFBTriggerFrictionEffect upload=%d strength=%f\n", upload, strength);
	if(FF_FRICTION_LOADED==(supportedFeatures & FF_FRICTION_LOADED)) 
	{
		struct ff_effect* frictionEffect=&ffb_effects[friction_effect_idx];
		if(upload)
		{
			short minForce = (short)(strength > 0.001 ? (getConfig()->minFriction / 100.0 * 32767.0) : 0); // strength is a double so we do an epsilon check of 0.001 instead of > 0.
			short maxForce = (short)(getConfig()->maxFriction / 100.0 * 32767.0);
			short range = maxForce - minForce;
			short coeff = (short)(strength * range + minForce);
			if (coeff < 0)
				coeff = 32767;

			frictionEffect->u.condition[0].left_coeff = (short)(coeff);
			frictionEffect->u.condition[0].left_saturation = (short)(coeff * 2.0); 
			frictionEffect->u.condition[0].right_saturation = (short)(coeff * 2.0); 
			frictionEffect->u.condition[0].right_coeff = (short)(coeff);
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

		//START EFFECT
		event.value = 1;
		if (write(device_handle, &event, sizeof(event)) != sizeof(event))
			fprintf(stderr, "ERROR: starting FF_FRICTION effect failed (%s) [%s:%d]\n",	strerror(errno), __FILE__, __LINE__);

		write(device_handle, &event, sizeof(event)) != sizeof(event);
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

			short MinForce = (short)(strength > 0.001 ? (getConfig()->minTorque / 100.0 * 32767.0) : 0);
			short MaxForce = (short)(getConfig()->maxTorque / 100.0 * 32767.0);
			short range = MaxForce - MinForce;
			short level = (short)(strength * range + MinForce);

			switch (motor) {
				case weak_motor:
					rumbleEffect->u.rumble.strong_magnitude = 0;
					rumbleEffect->u.rumble.weak_magnitude = level;
					break;
				case strong_motor:
					rumbleEffect->u.rumble.strong_magnitude = level;
					rumbleEffect->u.rumble.weak_magnitude = 0;
					break;
				case both_motors:
				default:
					rumbleEffect->u.rumble.strong_magnitude = level;
					rumbleEffect->u.rumble.weak_magnitude = level;
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

		//START EFFECT
		event.value = 1;
		if (write(device_handle, &event, sizeof(event)) != sizeof(event))
			fprintf(stderr, "ERROR: starting FF_RUMBLE effect failed (%s) [%s:%d]\n",	strerror(errno), __FILE__, __LINE__);

		write(device_handle, &event, sizeof(event)) != sizeof(event);
	}
	else
	{
		debug(1, " -> rumble effect not supported\n");
	}	
}

/* --- set flobal gain for all effects (1-100) --- */
void FFBSetGlobalGain(int level)
{
	debug(1, "FFBSetGlobalGain\n");

	memset(&gain, 0, sizeof(gain));
	gain.type = EV_FF;
	gain.code = FF_GAIN;
	gain.value = 0xFFFFUL * level / 100;

	if (write(device_handle, &gain, sizeof(gain)) != sizeof(gain)) 
		debug(1, "Error setting global gain\n");
	else{
		supportedFeatures|=FF_GAIN_LOADED;
	}
}

/* --- AutoCenter Global Setting (1-100) for 1 second only --- */
void FFBSetGlobalAutoCenter(int level, int duration_ms)
{
	debug(1, "FFBSetGlobalAutoCenter (during 1 sec)\n");
	
	memset(&event, 0, sizeof(event));
	event.type = EV_FF;
	event.code = FF_AUTOCENTER;
	event.value = 0xFFFFUL * level / 100;	

	/* Enable autocalibration / auto-center */
	if (write(device_handle, &event, sizeof(event)) != sizeof(event))
		debug(1, "ERROR: failed to enable auto centering (%s) [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
	else {
		supportedFeatures |= FF_AUTOCENTER_LOADED;
	}

	/* Keep auto-center for 1 second */
	usleep(duration_ms*1000);

	/* Disable auto-center */
	memset(&event, 0, sizeof(event));
	event.type = EV_FF;
	event.code = FF_AUTOCENTER;
	event.value = 0;
	if (write(device_handle, &event, sizeof(event)) != sizeof(event))
		debug(1, "ERROR: failed to disable auto centering (%s) [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
}

void FFBTriggerTestEffect(unsigned int effect, double strength)
{
	//debug(0,"FFBTriggerEffect effect=%u, strength=%f\n",effect, strength);
    switch(effect)
    {
        case FF_CONSTANT:
            FFBTriggerConstantEffect(true, strength);
            break;
        case FF_SPRING:
            FFBTriggerSpringEffect(true, strength);
            break;
        case FF_FRICTION:
            FFBTriggerFrictionEffect(true, strength);
            break;
        case FF_AUTOCENTER:
            FFBSetGlobalAutoCenter(40, 1500);
            break;              
        case FF_RUMBLE:
            FFBTriggerRumbleEffectDefault(true, strength);
            break;  
		case FF_SINE:
            FFBTriggerSineEffect(true, 19,0.20);
            break;  						
    }
}

// alternate_modes
// combine_pedals  
// damper_level  
// ffb_leds        
// gain
// real_id            
// spring_level
// autocenter       
// country
// friction_level
// peak_ffb_level  
// range  
// report_descriptor
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