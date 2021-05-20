    
#include "debug.h"
#include "ffbhelper.h"
#include "config.h"
#include <time.h>
    
#include<stdio.h>
#include<string.h>

#include <linux/input.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <ctype.h>

/*
some links: 
	https://www.kernel.org/doc/html/latest/input/ff.html
	https://github.com/Eliasvan/Linux-Force-Feedback
	https://www.youtube.com/watch?v=pCq01LHaIVg
	https://www.linuxjournal.com/article/6429
	https://github.com/Wiladams/LJIT2RPi/blob/master/tests/test_input.c
	https://github.com/flosse/linuxconsole/blob/master/utils/fftest.c
	https://github.com/OpenJVS/OpenJVS/blob/bobby/ffb/src/ffb.c
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

bool FFBGetDeviceVendorProductVersion(int handle, char *deviceVendorProductVersion)
{
	struct input_id device_info;

	if (ioctl(handle, EVIOCGID, &device_info)) 
	{
		debug(1," Error in evdev ioctl for FFBGetDeviceVendorProductVersion (%s) [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
    	return false;
	}
	else{
		snprintf(deviceVendorProductVersion, 42,"%04hx/%04hx/%04hx", device_info.vendor, device_info.product, device_info.version);
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
	if (!testBit(FF_RUMBLE, ff_bits))
		return false;
	else
		return true;
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
        if ((fd = open(fname, O_RDONLY)) > -1)
        {
			if(FFBCheckIfFFBDevice(fd)){
				strcpy(devices[NbrOfDevices].path, fname);
				FFBGetDeviceName(fd, devices[NbrOfDevices].realName);
				strcpy(devices[NbrOfDevices].simplifiedName, FFBGetHapticSimplifiedName(devices[NbrOfDevices].realName));
				FFBGetDeviceVendorProductVersion(fd, devices[NbrOfDevices].vendorProductVersion);
				FFBGetDeviceDriverVersion(fd, devices[NbrOfDevices].driverVersion);
				
				NbrOfDevices++;
			}
			
			close(fd);
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
				debug(0, "Device[%d] Vendor/Product/Version : %s\n", i, devices[i].vendorProductVersion);
				debug(0, "Device[%d] Driver Version         : %s\n", i, devices[i].driverVersion);
				debug(0, "Device[%d] path                   : %s\n", i, devices[i].path);
			}
		}
	}
}

char* FFBGetDevicePath(char* device_name)
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
		return devices[idxDevice].path;
}

bool FFBInitHaptic(char* device_name)
{
	if(FFBGetAllDevices()) {
		char* devicePath=FFBGetDevicePath(device_name);
		printf("devicePath=%s\n", devicePath);
		if(devicePath){
			unsigned char ffb_supported[1 + FF_MAX/8/sizeof(unsigned char)];

			device_handle = open(devicePath, O_RDWR|O_NONBLOCK);

			if (device_handle > -1) {
				printf("Using device %s.\n\n", device_name);
				
				FFBCreateHapticEffects();
				FFBSetGlobalGain(getConfig()->globalGain);
				FFBSetGlobalAutoCenter(getConfig()->autoCenter);
				return true;
			}
			else {
				debug(0, "ERROR: can not open %s (%s)\n", device_name, strerror(errno));
				return false;
			}
		}
	}
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
	effect->replay.length = HAPTIC_INFINITY; 
	effect->replay.delay = 0;
	effect->direction = 0xC000;
	//effect->replay =1; 
	//effect->u.constant.level = 0; 			// this is an sint16 => -32768 to 32767
	effect->u.constant.envelope.attack_length = 0;
	effect->u.constant.envelope.attack_level = 0;		// -> this one to update
	effect->u.constant.envelope.fade_length = 0;
	effect->u.constant.envelope.fade_level = 0;			// -> this one to update

	if(ioctl(device_handle, EVIOCSFF, effect))
		debug(1," Error creating FF_CONSTANT effect (%s) [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
	else{
		supportedFeatures|=FF_CONSTANT_LOADED;
		debug(1, "FF_CONSTANT Effect id=%d\n", effect->id);

		/* Start effect */
		
		memset(&event, 0, sizeof(event));
		event.type = EV_FF;
		event.code = effect->id;
		event.value = 1;
		if (write(device_handle, &event, sizeof(event)) != sizeof(event))
			fprintf(stderr, "ERROR: starting effect failed (%s) [%s:%d]\n",	strerror(errno), __FILE__, __LINE__);
			
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
	effect->replay.length = HAPTIC_INFINITY;
	effect->replay.delay = 1000;

	if(ioctl(device_handle, EVIOCSFF, effect))
		debug(1," Error creating FF_PERIODIC FF_SINE effect (%s) [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
	else{
		supportedFeatures|=FF_SINE_LOADED;
		debug(1, "FF_SINE Effect     id=%d\n", effect->id);
		
		/* Start effect */
		memset(&event, 0, sizeof(event));
		event.type = EV_FF;
		event.code = effect->id;
		event.value = 1;
		if (write(device_handle, &event, sizeof(event)) != sizeof(event))
			fprintf(stderr, "ERROR: starting effect failed (%s) [%s:%d]\n",	strerror(errno), __FILE__, __LINE__);	
	}

	
    /* --- FF_FRICTION --- */
	effect=&ffb_effects[friction_effect_idx];
	memset(effect,0,sizeof(ffb_effects[0]));
	
	effect->id = -1;
	effect->type = FF_FRICTION;

	effect->trigger.button = 0;
	effect->trigger.interval = 0;
	effect->replay.length = HAPTIC_INFINITY; 
	effect->replay.delay = 0;
	effect->direction = 0xC000;
	effect->u.condition->left_saturation = 0xFFFF;
	effect->u.condition->right_saturation = 0xFFFF;

	if(ioctl(device_handle, EVIOCSFF, effect))
		debug(1," Error creating FF_FRICTION  effect (%s) [%s:%d]\n", strerror(errno), __FILE__, __LINE__);	
	else{
		supportedFeatures|=FF_FRICTION_LOADED;
		debug(1, "FF_FRICTION Effect id=%d\n", effect->id);	
		
		/* Start effect */
		memset(&event, 0, sizeof(event));
		event.type = EV_FF;
		event.code = effect->id;
		event.value = 1;
		if (write(device_handle, &event, sizeof(event)) != sizeof(event))
			fprintf(stderr, "ERROR: starting effect failed (%s) [%s:%d]\n",	strerror(errno), __FILE__, __LINE__);		
	}

    /* --- FF_SPRING --- */
	effect=&ffb_effects[spring_effect_idx];
	memset(effect,0,sizeof(ffb_effects[0]));
	
	effect->id = -1;
	effect->type = FF_SPRING;

	effect->u.condition[0].right_saturation = 0x7fff;
	effect->u.condition[0].left_saturation = 0x7fff;
	effect->u.condition[0].right_coeff = 0x2000;
	effect->u.condition[0].left_coeff = 0x2000;
	effect->u.condition[0].deadband = 0x0;
	effect->u.condition[0].center = 0x0;
	effect->trigger.button = 0;
	effect->trigger.interval = 0;
	effect->replay.length = HAPTIC_INFINITY;  /* 20 seconds */
	effect->replay.delay = 0;

	if(ioctl(device_handle, EVIOCSFF, effect))
		debug(1," Error creating FF_SPRING  effect (%s) [%s:%d]\n", strerror(errno), __FILE__, __LINE__);		
	else{
		supportedFeatures|=FF_SPRING_LOADED;
		debug(1, "FF_SPRING Effect   id=%d\n", effect->id);	

		/* Start effect */
		memset(&event, 0, sizeof(event));
		event.type = EV_FF;
		event.code = effect->id;
		event.value = 1;
		if (write(device_handle, &event, sizeof(event)) != sizeof(event))
			fprintf(stderr, "ERROR: starting effect failed (%s) [%s:%d]\n",	strerror(errno), __FILE__, __LINE__);		
	}

    /* --- FF_RUMBLE --- */
	effect=&ffb_effects[rumble_effect_idx];
	memset(effect,0,sizeof(ffb_effects[0]));
	
	effect->id = -1;
	effect->type = FF_RUMBLE;

	effect->u.rumble.strong_magnitude = 0; 
	effect->u.rumble.weak_magnitude = 0;   
	effect->replay.length = HAPTIC_INFINITY; 
	effect->replay.delay = 1000;

	if(ioctl(device_handle, EVIOCSFF, effect))
		debug(1," Error creating FF_SPRING  effect (%s) [%s:%d]\n", strerror(errno), __FILE__, __LINE__);		
	else{
		supportedFeatures|=FF_RUMBLE_LOADED;
		debug(1, "FF_RUMBLE Effect   id=%d\n", effect->id);	

		/* Start effect */
		memset(&event, 0, sizeof(event));
		event.type = EV_FF;
		event.code = effect->id;
		event.value = 1;
		if (write(device_handle, &event, sizeof(event)) != sizeof(event))
			fprintf(stderr, "ERROR: starting effect failed (%s) [%s:%d]\n",	strerror(errno), __FILE__, __LINE__);
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
    debug(0, "     ffb_supported periodic effects:\n");
    debug(0, "      - sine:         %s", FFBCheckEffect(FF_SINE_LOADED));
    debug(0, "      - square:       %s", FFBCheckEffect(FF_SQUARE_LOADED));
    debug(0, "      - triangle:     %s", FFBCheckEffect(FF_TRIANGLE_LOADED));
    debug(0, "      - sawtoothup:   %s", FFBCheckEffect(FF_SAW_UP_LOADED));
    debug(0, "      - sawtoothdown: %s", FFBCheckEffect(FF_SAW_DOWN_LOADED));
    debug(0, "\n");
    debug(0, "     ffb_supported constant effect:\n");
    debug(0, "      - constant:     %s", FFBCheckEffect(FF_CONSTANT_LOADED));
    debug(0, "\n");
    debug(0, "     ffb_supported condition effects:\n");
    debug(0, "      - spring:       %s", FFBCheckEffect(FF_SPRING_LOADED));
    debug(0, "      - damper:       %s", FFBCheckEffect(FF_DAMPER_LOADED));
    debug(0, "      - inertia:      %s", FFBCheckEffect(FF_INERTIA_LOADED));
    debug(0, "      - friction:     %s", FFBCheckEffect(FF_FRICTION_LOADED));
    debug(0, "\n");
    debug(0, "     ffb_supported ramp effect:\n");
    debug(0, "      - ramp:         %s", FFBCheckEffect(FF_RAMP_LOADED));
    debug(0, "\n");
    debug(0, "     ffb_supported custom effect:\n");
    debug(0, "      - custom:       %s", FFBCheckEffect(FF_CUSTOM_LOADED));
    debug(0, "\n");
    debug(0, "     ffb_supported global features:\n");
    debug(0, "      - gain:        %s", FFBCheckEffect(FF_GAIN_LOADED));
    debug(0, "      - autocenter:  %s", FFBCheckEffect(FF_AUTOCENTER_LOADED));
    debug(0, "\n");
	debug(0, "     Rumble ffb_supported:\n");
    debug(0, "      - rumble:      %s", FFBCheckEffect(FF_RUMBLE_LOADED));
}

void FFBStopEffect(int effect_id)
{
    if (ioctl(device_handle, EVIOCRMFF, effect_id) < 0)
        fprintf(stderr, "ERROR: removing effect failed (%s) [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
}

void FFBStopAllEffects()
{
	if(device_handle)
	{
		for(int cp=0; cp < sizeof(effects_idx)/sizeof(int); cp++)
			FFBStopEffect(ffb_effects[cp].id);
	}
}

//Dummy call for now (TBC)
void FFBTriggerSineEffect(double strength)
{
	debug(1, "FFBTriggerSineEffect\n");
	if(FF_SINE_LOADED==(supportedFeatures & FF_SINE_LOADED)) 
	{
		struct ff_effect* effect=&ffb_effects[sine_effect_idx];

		short minForce = (short)(strength > 0.001 ? (getConfig()->minForce / 100.0 * 32767.0) : 0); // strength is a double so we do an epsilon check of 0.001 instead of > 0.
		short maxForce = (short)(getConfig()->maxForce / 100.0 * 32767.0);
		short range = maxForce - minForce;
		short coeff = (short)(strength * range + minForce);
		if (coeff < 0)
			coeff = 32767;

		effect->u.periodic.envelope.attack_level = 10;//(short)(coeff); //0x7fff; 		-> this one to update?
		effect->u.periodic.envelope.fade_level = 10;//(short)(coeff); //0x7fff; 		-> this one to update?

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

void FFBTriggerConstantEffect(double strength)
{
	debug(1, "TriggerConstantEffect\n");
	if(FF_CONSTANT_LOADED==(supportedFeatures & FF_CONSTANT_LOADED)) 
	{
		struct ff_effect* effect=&ffb_effects[constant_effect_idx];

		if (strength > 1.0)
			strength = 1.0;
		else if (strength < -1.0)
			strength = -1.0;

		int confMinForce = getConfig()->minForce;
		int confMaxForce = getConfig()->maxForce;

		short MinForce = (short)(strength > 0.001 ? (getConfig()->minForce / 100.0 * 32767.0) : 0);
		short MaxForce = (short)(getConfig()->maxForce / 100.0 * 32767.0);
		short range = MaxForce - MinForce;
		short level = (short)(strength * range + MinForce);

		effect->u.constant.level =  level;	
		effect->u.constant.envelope.attack_level =  (short)(strength * 32767.0); /* this one counts! */
		effect->u.constant.envelope.fade_level =  (short)(strength * 32767.0);	 /* only to be safe  */

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

void FFBTriggerFrictionEffect(double strength)
{
	FFBTriggerFrictionEffectWithDefaultOption(strength, false);
}

void FFBTriggerFrictionEffectWithDefaultOption(double strength, bool isDefault)
{
	debug(1, "FFBTriggerFrictionEffect\n");
	if(FF_CONSTANT_LOADED==(supportedFeatures & FF_CONSTANT_LOADED)) 
	{
		struct ff_effect* effect=&ffb_effects[constant_effect_idx];

		short minForce = (short)(strength > 0.001 ? (getConfig()->minForce / 100.0 * 32767.0) : 0); // strength is a double so we do an epsilon check of 0.001 instead of > 0.
		short maxForce = (short)(getConfig()->maxForce / 100.0 * 32767.0);
		short range = maxForce - minForce;
		short coeff = (short)(strength * range + minForce);
		if (coeff < 0)
			coeff = 32767;

		effect->u.condition->left_coeff =  (short)(coeff);
		effect->u.condition->right_coeff =  (short)(coeff);
		
		/* update effect */
    	if (ioctl(device_handle, EVIOCSFF, effect) < 0)
            debug(1, "ERROR: uploading effect failed (%s) [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
	}
	else
	{
		debug(1, "-> not ffb_supported, will try default rumble\n");
		FFBTriggerRumbleEffectDefault(strength);
	}


/*

	debug(1, "TriggerFrictionEffectWithDefaultOption\n");
	if (ffb_supported & SDL_HAPTIC_FRICTION) {
		SDL_HapticEffect tempEffect;
		SDL_memset(&tempEffect, 0, sizeof(SDL_HapticEffect));
		tempEffect.type = SDL_HAPTIC_FRICTION;
		tempEffect.condition.type = SDL_HAPTIC_FRICTION;
		tempEffect.condition.direction.type = SDL_HAPTIC_CARTESIAN;
		tempEffect.condition.delay = 0;
		tempEffect.condition.length = isDefault ? SDL_HAPTIC_INFINITY : getConfig()->feedbackLength;
		tempEffect.condition.left_sat[0] = 0xFFFF;
		tempEffect.condition.right_sat[0] = 0xFFFF;

		short minForce = (short)(strength > 0.001 ? (getConfig()->minForce / 100.0 * 32767.0) : 0); // strength is a double so we do an epsilon check of 0.001 instead of > 0.
		short maxForce = (short)(getConfig()->maxForce / 100.0 * 32767.0);
		short range = maxForce - minForce;
		short coeff = (short)(strength * range + minForce);
		if (coeff < 0)
			coeff = 32767;

		tempEffect.condition.left_coeff[0] = (short)(coeff);
		tempEffect.condition.right_coeff[0] = (short)(coeff);

		if(SDL_HapticUpdateEffect(haptic, effects.effect_friction_id, &tempEffect)!=0)
			debug(1, "Error updating effect: %s\n", SDL_GetError());
		else if(SDL_HapticRunEffect(haptic, effects.effect_friction_id, SDL_HAPTIC_INFINITY)!=0)
			debug(1, "Error executing effect: %s\n", SDL_GetError());
		else
			debug(1,"->success\n");				
	}
	else {
		debug(1, "-> not ffb_supported, will try default rumble\n");
		TriggerRumbleEffectDefault(strength);
	}
	*/
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

		int confMinForce = getConfig()->minForce;
		int confMaxForce = getConfig()->maxForce;

		short MinForce = (short)(strength > 0.001 ? (getConfig()->minForce / 100.0 * 32767.0) : 0);
		short MaxForce = (short)(getConfig()->maxForce / 100.0 * 32767.0);
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

void FFBTriggerSpringEffect(double strength)
{
	FFBTriggerSpringEffectWithDefaultOption(strength, false);
}

void FFBTriggerSpringEffectWithDefaultOption(double strength, bool isDefault)
{
	debug(1, "FFBTriggerSpringEffect\n");
	if (FF_SPRING_LOADED==(supportedFeatures & FF_SPRING_LOADED))  
	{
		struct ff_effect* effect=&ffb_effects[spring_effect_idx];
		short minForce = (short)(strength > 0.001 ? (getConfig()->minForce / 100.0 * 32767.0) : 0); // strength is a double so we do an epsilon check of 0.001 instead of > 0.
		short maxForce = (short)(getConfig()->maxForce / 100.0 * 32767.0);
		short range = maxForce - minForce;
		short coeff = (short)(strength * range + minForce);
		if (coeff < 0)
			coeff = 32767;
		
		effect->u.condition[0].right_saturation = (short)(coeff * 2.0); 
		effect->u.condition[0].left_saturation = (short)(coeff * 2.0); 
		effect->u.condition[0].right_coeff = (short)(coeff);
		effect->u.condition[0].left_coeff = (short)(coeff);

		/* update effect */
    	if (ioctl(device_handle, EVIOCSFF, effect) < 0)
            debug(1, "ERROR: uploading effect failed (%s) [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
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
            FFBTriggerConstantEffect(strength);
            break;
        case FF_SPRING:
            FFBTriggerSpringEffect(strength);
            break;
        case FF_FRICTION:
            FFBTriggerFrictionEffect(strength);
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

