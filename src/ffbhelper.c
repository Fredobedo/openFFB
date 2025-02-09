    
#include "debug.h"
#include "ffbhelper.h"
#include "config.h"
#include <time.h>
    
#include<stdio.h>
#include<unistd.h>
#include<string.h>

#include <linux/input.h>
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

bool FFBInitHaptic(char* device_name)
{
	if(FFBGetAllDevices()) {
		int idxDevice=FFBGetDeviceIdx(device_name);

		if(idxDevice> -1 && devices[idxDevice].path){
			unsigned char ffb_supported[1 + FF_MAX/8/sizeof(unsigned char)];

			device_handle = open(devices[idxDevice].path, O_RDWR|O_NONBLOCK);

			if (device_handle > -1) {
				printf("Using device %s.\n\n", device_name);

				/*-- test if it's a logitech Racing wheel (ID_VENDOR=046d) --*/
				if(strcmp(devices[idxDevice].vendor,"046d")==0)
					SetLogitechSteeringRange(idxDevice, getConfig()->logitechSteeringRange);

				FFBCreateHapticEffects();
				FFBSetGlobalGain(getConfig()->globalGain);
				//FFBSetGlobalAutoCenter(getConfig()->autoCenter);
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

void FFBCreateHapticEffects()
{
    /* --- FF_CONSTANT --- */
	struct ff_effect* effect=&ffb_effects[constant_effect_idx];
	memset(effect,0,sizeof(ffb_effects[0]));
	
	effect->id = -1;
	effect->type = FF_CONSTANT;

	effect->trigger.button = 0;
	effect->trigger.interval = 0;
	effect->replay.length = 5000; 
	effect->replay.delay = 0;
	
	effect->u.constant.level  = 0;
	
	effect->direction = 0xC000;
	
	effect->u.constant.envelope.attack_length = 0;
	effect->u.constant.envelope.attack_level = 0;		
	effect->u.constant.envelope.fade_length = 0;
	effect->u.constant.envelope.fade_level = 0;			

	if(ioctl(device_handle, EVIOCSFF, effect))
		debug(1," Error creating FF_CONSTANT effect (%s) [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
	else{
		supportedFeatures|=FF_CONSTANT_LOADED;
		debug(1, "FF_CONSTANT Effect id=%d\n", effect->id);

		/* Start effect */
		/*

		*/
	}

	/* --- FF_SINE --- */
	effect=&ffb_effects[sine_effect_idx];
	memset(effect,0,sizeof(ffb_effects[0]));
	
	effect->id = -1;
	effect->type = FF_PERIODIC;

	effect->u.periodic.waveform = FF_SINE;
	effect->u.periodic.period = 100;		/* 0.1 second */
	effect->u.periodic.magnitude = 0x7fff;	/* 0.5 * Maximum magnitude */
	effect->u.periodic.offset = 0;
	effect->u.periodic.phase = 0;
	effect->direction = 0x4000;				/* Along X axis */
	effect->u.periodic.envelope.attack_length = 1000;
	effect->u.periodic.envelope.attack_level = 0; //0x7fff; 		-> this one to update
	effect->u.periodic.envelope.fade_length = 1000;
	effect->u.periodic.envelope.fade_level = 0; //0x7fff; 			-> this one to update
	effect->trigger.button = 0;
	effect->trigger.interval = 0;
	effect->replay.length = 5000;
	effect->replay.delay = 0;
	effect->u.periodic.magnitude=0;

	if(ioctl(device_handle, EVIOCSFF, effect))
		debug(1," Error creating FF_PERIODIC FF_SINE effect (%s) [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
	else{
		supportedFeatures|=FF_SINE_LOADED;
		debug(1, "FF_SINE Effect     id=%d\n", effect->id);

	}

	
    /* --- FF_FRICTION --- */
	effect=&ffb_effects[friction_effect_idx];
	memset(effect,0,sizeof(ffb_effects[0]));
	
	effect->id = -1;
	effect->type = FF_FRICTION;
	effect->u.condition[0].right_saturation = 0x0;
	effect->u.condition[0].left_saturation = 0x0;
	effect->u.condition[0].right_coeff = 0x0;
	effect->u.condition[0].left_coeff = 0x0;
	effect->u.condition[0].deadband = 0x0;
	effect->u.condition[0].center = 0x0;
	effect->u.condition[1] = effect->u.condition[0];
	effect->trigger.button = 0;
	effect->trigger.interval = 0;
	effect->replay.length = 5000;  /* 20 seconds */
	effect->replay.delay = 0;

	if(ioctl(device_handle, EVIOCSFF, effect))
		debug(1," Error creating FF_FRICTION  effect (%s) [%s:%d]\n", strerror(errno), __FILE__, __LINE__);	
	else{
		supportedFeatures|=FF_FRICTION_LOADED;
		debug(1, "FF_FRICTION Effect id=%d\n", effect->id);	
		
		/* Start effect */
		/*
		memset(&event, 0, sizeof(event));
		event.type = EV_FF;
		event.code = effect->id;
		event.value = 1;
		if (write(device_handle, &event, sizeof(event)) != sizeof(event))
			fprintf(stderr, "ERROR: starting FF_FRICTION effect failed (%s) [%s:%d]\n",	strerror(errno), __FILE__, __LINE__);	
		else if(getConfig()->staticFriction > 0)
			FFBTriggerFrictionEffect(((double)getConfig()->staticFriction)/100);				
			
		*/
	}

    /* --- FF_DAMPER --- */
	effect=&ffb_effects[damper_effect_idx];
	memset(effect,0,sizeof(ffb_effects[0]));
	
	effect->id = -1;
	effect->type = FF_DAMPER;

	effect->trigger.button = 0;
	effect->trigger.interval = 0;
	effect->replay.length = 5000; 
	effect->replay.delay = 0;
	effect->direction = 0x0000; // 0x4000; // 0x8000 -> left, 0xC000-> right
	effect->u.condition->left_saturation = 0;
	effect->u.condition->right_saturation = 0;

	if(ioctl(device_handle, EVIOCSFF, effect))
		debug(1," Error creating FF_FRICTION  effect (%s) [%s:%d]\n", strerror(errno), __FILE__, __LINE__);	
	else{
		supportedFeatures|=FF_FRICTION_LOADED;
		debug(1, "FF_FRICTION Effect id=%d\n", effect->id);	

	}

    /* --- FF_SPRING --- */
	effect=&ffb_effects[spring_effect_idx];
	memset(effect,0,sizeof(ffb_effects[0]));
	
	effect->id = -1;
	effect->type = FF_SPRING;

	effect->u.condition[0].deadband = 0x0;
	effect->u.condition[0].center = 0x0;
	effect->trigger.button = 0;
	effect->trigger.interval = 0;
	effect->replay.length = HAPTIC_INFINITY;  
	effect->replay.delay = 0;

	if(ioctl(device_handle, EVIOCSFF, effect))
		debug(1," Error creating FF_SPRING  effect (%s) [%s:%d]\n", strerror(errno), __FILE__, __LINE__);		
	else{
		supportedFeatures|=FF_SPRING_LOADED;
		debug(1, "FF_SPRING Effect   id=%d\n", effect->id);	

		/* Start effect */
		/*
		memset(&event, 0, sizeof(event));
		event.type = EV_FF;
		event.code = effect->id;
		event.value = 1;
		if (write(device_handle, &event, sizeof(event)) != sizeof(event))
			fprintf(stderr, "ERROR: starting FF_SPRING effect failed (%s) [%s:%d]\n",	strerror(errno), __FILE__, __LINE__);		
		else if(getConfig()->staticSpring > 0)
			FFBTriggerSpringEffect(((double)getConfig()->staticSpring)/100);			
		*/
	}

    /* --- FF_RUMBLE --- */
	effect=&ffb_effects[rumble_effect_idx];
	memset(effect,0,sizeof(ffb_effects[0]));
	
	effect->id = -1;
	effect->type = FF_RUMBLE;

	effect->u.rumble.strong_magnitude = 0; 
	effect->u.rumble.weak_magnitude = 0;   
	effect->replay.length = 5000; 
	effect->replay.delay = 0;

	if(ioctl(device_handle, EVIOCSFF, effect))
		debug(1," Error creating FF_SPRING  effect (%s) [%s:%d]\n", strerror(errno), __FILE__, __LINE__);		
	else{
		supportedFeatures|=FF_RUMBLE_LOADED;
		debug(1, "FF_RUMBLE Effect   id=%d\n", effect->id);	

		/* Start effect */
		/*
		memset(&event, 0, sizeof(event));
		event.type = EV_FF;
		event.code = effect->id;
		event.value = 1;
		if (write(device_handle, &event, sizeof(event)) != sizeof(event))
			fprintf(stderr, "ERROR: starting effect failed (%s) [%s:%d]\n",	strerror(errno), __FILE__, __LINE__);
			*/
	}
	
}

void FFBAbortExecution(void)
{
    debug(1, "\nAborting program execution.\n");
	if(device_handle){
		FFBStopAllEffects();
		close(device_handle);
	}
}

char* FFBCheckEffect(unsigned int check)
{
	if (check==(supportedFeatures & check)) 
        return GREEN "-> OK <-" RESET "\n";
    else
        return RED "Not ffb_supported" RESET "\n";
}

void FFBDumpSupportedFeatures()
{
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
    debug(0, "     ffb_supported condition effects:\n");
    debug(0, "      - spring:       %s", FFBCheckEffect(FF_SPRING_LOADED));
    debug(0, "      - friction:     %s", FFBCheckEffect(FF_FRICTION_LOADED));
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
	if(device_handle)
	{
		for(int cp=0; cp < sizeof(effects_idx)/sizeof(int); cp++)
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
		for(int cp=0; cp < sizeof(effects_idx)/sizeof(int); cp++)
			FFBRemoveEffect(ffb_effects[cp].id);
	}
}

/**
 * @waveform: kind of the effect (wave)
 * @period: period of the wave (ms)
 * @magnitude: peak value
 * @offset: mean value of the wave (roughly)
 * @phase: 'horizontal' shift
 * @envelope: envelope data
 */
void FFBTriggerSineEffect(double strength)
{
	debug(1, "FFBTriggerSineEffect\n");
	if(FF_SINE_LOADED==(supportedFeatures & FF_SINE_LOADED)) 
	{
		struct ff_effect* effect=&ffb_effects[sine_effect_idx];

		short minForce = (short)(strength > 0.001 ? (getConfig()->minTorque / 100.0 * 32767.0) : 0); // strength is a double so we do an epsilon check of 0.001 instead of > 0.
		short maxForce = (short)(getConfig()->maxTorque / 100.0 * 32767.0);
		short range = maxForce - minForce;
		short coeff = (short)(strength * range + minForce);
		if (coeff < 0)
			coeff = 32767;

		effect->u.periodic.magnitude=(short)(coeff);

		/* update effect */
    	if (ioctl(device_handle, EVIOCSFF, effect) < 0)
            debug(1, "ERROR: uploading effect failed (%s) [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
	}
	else
	{
		debug(1, "-> not ffb_supported, will try default rumble\n");
		FFBTriggerRumbleEffectDefault(strength);
	}
}

void FFBTriggerSpringEffect(bool upload, double strength)
{
	debug(1, "FFBTriggerSpringEffect\n");
	if(FF_SPRING_LOADED==(supportedFeatures & FF_SPRING_LOADED)) 
	{
		struct ff_effect* springEffect=&ffb_effects[spring_effect_idx];
		if(upload)
		{
			short minForce = (short)(strength > 0.001 ? (getConfig()->minSpring / 100.0 * 32767.0) : 0); // strength is a double so we do an epsilon check of 0.001 instead of > 0.
			short maxForce = (short)(getConfig()->maxSpring / 100.0 * 32767.0);
			short range = maxForce - minForce;
			short coeff = (short)(strength * range + minForce);
			if (coeff < 0)
				coeff = 32767;

			springEffect->u.condition[0].left_coeff = (short)(coeff);
			springEffect->u.condition[0].left_saturation = (short)(coeff * 2.0); 
			springEffect->u.condition[0].right_saturation = (short)(coeff * 2.0); 
			springEffect->u.condition[0].right_coeff = (short)(coeff);
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
		debug(1, "-> not ffb_supported, will try default rumble\n");
		FFBTriggerRumbleEffectDefault(strength);
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
 * The @attack_level and @fade_level are absolute values; when applying
 * envelope force-feedback core will convert to positive/negative
 * value based on polarity of the default level of the effect.
 * Valid range for the attack and fade levels is 0x0000 - 0x7fff
 */
void FFBTriggerConstantEffect(bool upload, double strength)
{
	debug(1, "TriggerConstantEffect\n");
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

			/*
			debug(0,"\nfred: update effect confMinForce=%d", confMinForce);
			debug(0,"\nfred: update effect confMaxForce=%d", confMaxForce);
			debug(0,"\nfred: update effect MinForce=%hd", MinForce);
			debug(0,"\nfred: update effect MaxForce=%hd", MaxForce);
			debug(0,"\nfred: update effect range=%hd", range);
			debug(0,"\nfred: update effect level=%hd", level);
			*/
			/* Here we set the two values to the max as the arcade system 'manages" fades                */
			constantEffect->u.constant.envelope.attack_level =  (short)(strength * 32767.0); /* this one counts! */
			constantEffect->u.constant.envelope.fade_level =    (short)(strength * 32767.0); /* only to be safe  */

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
		debug(1, "-> not ffb_supported, will try default rumble\n");
		FFBTriggerRumbleEffectDefault(strength);
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
		debug(1, "-> not ffb_supported, will try default rumble\n");
		FFBTriggerRumbleEffectDefault(strength);
	}
}

void FFBTriggerRumbleEffectDefault(double strength)
{
	FFBTriggerRumbleEffect(strength, both_motors);
}

void FFBTriggerRumbleEffect(double strength, motor_select motor)
{
	if(FF_RUMBLE_LOADED==(supportedFeatures & FF_RUMBLE_LOADED)) 
	{	
		struct ff_effect* effect=&ffb_effects[rumble_effect_idx];

		if (strength > 1.0)
			strength = 1.0;
		else if (strength < -1.0)
			strength = -1.0;

		short MinForce = (short)(strength > 0.001 ? (getConfig()->minTorque / 100.0 * 32767.0) : 0);
		short MaxForce = (short)(getConfig()->maxTorque / 100.0 * 32767.0);
		short range = MaxForce - MinForce;
		short level = (short)(strength * range + MinForce);

		if(motor==weak_motor){
			effect->u.rumble.strong_magnitude =  0; 
			effect->u.rumble.weak_magnitude =  level;   

			effect->u.rumble.strong_magnitude =  level;
			effect->u.rumble.weak_magnitude =  0;   
		}
		else {	
			effect->u.rumble.strong_magnitude =  level; 
			effect->u.rumble.weak_magnitude =  level;  
		}

		/* update effect */
    	if (ioctl(device_handle, EVIOCSFF, effect) < 0)
            debug(1, "ERROR: uploading effect failed (%s) [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
	}
	else
		debug(1,"FFBTriggerRumbleEffect not supported\n");
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

/* --- AutoCenter Global Setting (1-100)--- */
void FFBSetGlobalAutoCenter(int level)
{
	debug(1, "FFBSetGlobalAutoCenter\n");
	
	memset(&event, 0, sizeof(event));
	event.type = EV_FF;
	event.code = FF_AUTOCENTER;
	event.value = 0xFFFFUL * level / 100;
	if (write(device_handle, &event, sizeof(event)) != sizeof(event))
		debug(1, "ERROR: failed to disable auto centering (%s) [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
	else{
		supportedFeatures|=FF_AUTOCENTER_LOADED;
	}		
}

void FFBTriggerEffect(unsigned int effect, double strength)
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
            FFBSetGlobalAutoCenter(40);
            break;              
        case FF_RUMBLE:
            FFBTriggerRumbleEffectDefault(strength);
            break;  
		case FF_SINE:
            FFBTriggerSineEffect(strength);
            break;  						
    }
}

void SetLogitechSteeringRange(int idxDevice, int range)
{
	printf("SetLogitechSteeringRange ... \n");
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
			debug(2, "Logitech device found, setting range to %d...\n", range);

			// PATH + "/device/device/range"
			char logitechSysFsRangeFile[256];
			strcpy(logitechSysFsRangeFile, path);
  			strcat(logitechSysFsRangeFile, "/device/device/range");

/*
			printf("logitechSysFsRangeFile=%s\n",logitechSysFsRangeFile);
			if(access(logitechSysFsRangeFile, F_OK)!=0){
				debug(2, "This Logitech does not have support for changing Steering Range\n");
			}
			else
			{
				*/
			FILE * file = fopen(logitechSysFsRangeFile,"w");
			
			if(file!=0){
				fprintf(file,"%d",  range);
				fclose (file);
				debug(2, "Steering Range updated\n");
			}
			else{
				debug(2, "can not set Steering Range, did you start it with 'sudo'?\n");
			}
			//}
			//printf("path=%s\n", path);
			//printf("udev_device_get_devnode=%s\n", udev_device_get_devnode(dev));
			break;
		}

        udev_device_unref(dev);
    }

    udev_enumerate_unref(enumerate);
    udev_unref(udev);
}