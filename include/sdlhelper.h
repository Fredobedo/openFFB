#ifndef SDLHELPER_H_
#define SDLHELPER_H_

#include <stdint.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_haptic.h>


#define SDL_USED_SUBSYSTEMS SDL_INIT_TIMER|SDL_INIT_JOYSTICK|SDL_INIT_HAPTIC
SDL_Haptic* haptic;
unsigned int supported;
bool SDL_Haptic_RumbleSupported;

/* --- structure holding effect addresses --- */
struct effects_t {
	unsigned int effect_constant_id;
	unsigned int effect_leftramp_id;
	unsigned int effect_rightramp_id;
	unsigned int effect_friction_id;
	unsigned int effect_leftright_id;
	unsigned int effect_sine_id;
	unsigned int effect_spring_id;
	unsigned int effect_vibration_id;
	unsigned int effect_inertia_id;
	unsigned int effect_ramp_id;
	unsigned int effect_damper_id;
	unsigned int effect_sawtoothup_id;
	unsigned int effect_sawtoothdown_id;
	unsigned int effect_triangle_id;
} effects;


/* --- close haptic and sql quit --- */
void abort_execution(void);

//void HapticPrintSupported(SDL_Haptic* haptic);
int initHaptic(char* name);

void CreateHapticEffects();
void DumpAvailableHaptics();
char* GetHapticSimplifiedName(const char* name);

/* --- SDL Effects (from Boomslangnz)          --- */
/* we most probably will not use all of them       */
/* But they certainly can be used for test purpose */
void TriggerSineEffect(uint16_t period, uint16_t fadePeriod, double strength, bool replay);
void TriggerLeftRightEffect(double smallstrength, double largestrength, bool replay);
void TriggerTriangleEffect(double strength, bool replay);
void TriggerSawToothUpEffect(double strength, bool replay);
void TriggerSawToothDownEffect(double strength, bool replay);
void TriggerConstantEffect(int direction, double strength);
void TriggerSpringEffect(double strength);
void TriggerSpringEffectWithDefaultOption(double strength, bool isDefault);
void TriggerDamperEffect(double strength, bool replay);
void TriggerInertiaEffect(double strength, bool replay);
void TriggerFrictionEffect(double strength);
void TriggerFrictionEffectWithDefaultOption(double strength, bool isDefault);
void TriggerRampEffect(double start, double end, bool replay);
void TriggerAutoCenterEffect(double strength);
void TriggerRumbleEffectDefault(double strength);
void TriggerRumbleEffect(double strength, int length);


/* --- generic function for testing --- */
void TriggerEffect(unsigned int sdsl_effect,double strength);
/* --- This will stop the effect if it's running. Effects are automatically destroyed when the device is closed. --- */
void stopEffect(int effect);
/* --- This will stop all effects if they are running. Effects are automatically destroyed when the device is closed. --- */
void stopAllEffects(void);

/* --- additional functions --- */
//void CenterWheel();
void dumpSupportedFeatures();

#endif