#ifndef FFBHELPER_H_
#define FFBHELPER_H_

#include <stdint.h>
#include <stdbool.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <dirent.h>

#include <libudev.h>
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <unistd.h>


#define HAPTIC_INFINITY 0
//#define HAPTIC_INFINITY 0xFFFF

/* Number of bits for 1 unsigned char */
#define nBitsPerUchar (sizeof(unsigned char) * 8)
/* Number of unsigned chars to contain a given number of bits */
#define nUcharsForNBits(nBits) ((((nBits)-1) / nBitsPerUchar) + 1)
/* Index=Offset of given bit in 1 unsigned char */
#define bitOffsetInUchar(bit) ((bit) % nBitsPerUchar)
/* Index=Offset of the unsigned char associated to the bit at the given index=offset */
#define ucharIndexForBit(bit) ((bit) / nBitsPerUchar)
/* Value of an unsigned char with bit set at given index=offset */
#define ucharValueForBit(bit) (((unsigned char)(1)) << bitOffsetInUchar(bit))
/* Test the bit with given index=offset in an unsigned char array */
#define testBit(bit, array) ((array[ucharIndexForBit(bit)] >> bitOffsetInUchar(bit)) & 1)
unsigned char ff_bits[1 + FF_MAX / 8 / sizeof(unsigned char)];

/* All loaded/supported effects are stored here */
unsigned int supportedFeatures;

#define FF_SINE_LOADED       1U << 0
#define FF_SQUARE_LOADED     1U << 1
#define FF_TRIANGLE_LOADED   1U << 2
#define FF_SAW_UP_LOADED     1U << 3
#define FF_SAW_DOWN_LOADED   1U << 4
#define FF_CONSTANT_LOADED   1U << 5
#define FF_SPRING_LOADED     1U << 6
#define FF_DAMPER_LOADED     1U << 7
#define FF_INERTIA_LOADED    1U << 8
#define FF_FRICTION_LOADED   1U << 9
#define FF_RAMP_LOADED       1U << 10
#define FF_CUSTOM_LOADED     1U << 11
#define FF_GAIN_LOADED       1U << 12
#define FF_AUTOCENTER_LOADED 1U << 13
#define FF_RUMBLE_LOADED     1U << 14

#define DEV_INPUT_EVENT "/dev/input"
#define MAX_DEVICE 10
#define MAX_EFFECTS 16

/* The haptic File descriptor Handle */
int device_handle;

struct device{
	char path[256];
	char realName[256];
	char simplifiedName[256];
	char driverVersion[128];
	char vendor[6];	
	char Product[6];	
	char Version[6];	
} devices[MAX_DEVICE];
int NbrOfDevices;

typedef enum{
	constant_effect_idx = 0,
	sine_effect_idx,
	square_effect_idx,
	triangle_effect_idx,		
	sawtoothup_effect_idx,	
	sawtoothdown_effect_idx,
	spring_effect_idx,
	damper_effect_idx,
	inertia_effect_idx,	
	friction_effect_idx,
	ramp_effect_idx,	
	rumble_effect_idx
} effects_idx;

typedef enum { 
	strong_motor,
	weak_motor,
	both_motors
} motor_select;

struct ff_effect ffb_effects[MAX_EFFECTS];
struct input_event play, stop, gain, event;

bool  FFBGetDeviceName(int handle, char *deviceName);
bool  FFBGetDeviceVendorProductVersion(int handle, char *deviceVendor, char *deviceProduct, char *deviceVersion);
bool  FFBCheckIfFFBDevice(int handle);

char* FFBGetHapticSimplifiedName(const char* name);
int   FFBisEventDevice(const struct dirent *dir);
int   FFBGetAllDevices();
int   FFBGetDeviceIdx(char* device_name);

void  FFBDumpAvailableDevices();

/* --- close haptic and sql quit --- */
void  FFBAbortExecution(void);

bool  FFBInitHaptic(char* name);

void  FFBCreateHapticEffects();

void  FFBTriggerSpringEffect(bool upload, double strength);
void  FFBTriggerConstantEffect(bool upload, double strength);
void  FFBTriggerFrictionEffect(bool upload, double strength);

void  FFBTriggerSineEffect(double strength);
void  FFBTriggerRumbleEffectDefault(double strength);
void  FFBTriggerRumbleEffect(double strength,  motor_select motor);

void FFBStopEffect(int effect_id);
void FFBStopAllEffects(void);

/* --- generic function for testing --- */
void FFBTriggerEffect(unsigned int effect,double strength);
/* --- This will remove the effect from the device. Effects are automatically destroyed when the device is closed. --- */
void FFBRemoveEffect(int effect_id);
/* --- This will remove all effects if they are running. Effects are automatically destroyed when the device is closed. --- */
void FFBRemoveAllEffects(void);

/* --- dump supported features in the console --- */
void FFBDumpSupportedFeatures();

/* --- set global gain and auto center effects --- */
void FFBSetGlobalGain(int level);
void FFBSetGlobalAutoCenter(int level);

/* -- the new-lg4ff logitech driver permits to adapt on the fly the steering wheel range -- */
void SetLogitechSteeringRange(int idxDevice, int range);

#endif