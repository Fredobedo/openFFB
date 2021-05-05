#ifndef SDLHELPER_H_
#define SDLHELPER_H_

#include <stdint.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_haptic.h>


// All forces /designated within a game/ will be scaled to exist between MinForce and MaxForce
// 0-100: you may want to set a min force if your wheel doesn't deliver sufficient forces when values are low
#define CONFIG_MIN_FORCE      0

// 0-100: you may want to set a max force below 100 if your wheel's forces are stronger than desired @ 100%
#define CONFIG_MAX_FORCE      100

// Set to 1 if you want to enable rumble, else 0.
#define CONFIG_ENABLE_RUMBLE  1
#define CONFIG_REVERSE_RUMBLE 0

// Set to 1 if you want to enable rumble in triggers, else 0.
#define CONFIG_ENABLE_RUMBLE_TRIGGERS 1

// Length of a feedback command. While a long period works fine, some games may still require shorter ones.
#define CONFIG_FEEDBACK_LENGTH 500

// If a game does not specify its own Centering or Friction forces (e.g. only specifies roll left/right),
// then a default centering and friction force can be applied here. If a game has any of its own such forces,
// these values will be overwritten immediately in-game.
// 0-100; Centering is the force that brings the wheel back to the center. Use -1 to disable behavior.
#define CONFIG_DEFAULT_CENTERING 0

// 0-100; Friction determines how difficult a wheel is to turn. Use -1 to disable behavior.
#define CONFIG_DEFAULT_FRICTION 0

// Set to 1 if you want to enable dynamic strength adjustments, else 0.
#define CONFIG_ENABLE_FFB_STRENGTH_DYNAMIC_ADJUSTMENT 0

// Set to 1 if you want FFB strength adjustments to persist between loads, else 0.
#define CONFIG_ENABLE_FFBSTRENGTH_PERSISTENCE 0

// Use this function to set the global gain of the specified haptic device.
// The user may specify the maximum gain by setting the environment variable SDL_HAPTIC_GAIN_MAX which should be between 0 and 100.
// All calls to SDL_HapticSetGain() will scale linearly using SDL_HAPTIC_GAIN_MAX as the maximum.
#define CONFIG_GLOBAL_GAIN 0

#define SDL_USED_SUBSYSTEMS SDL_INIT_TIMER|SDL_INIT_JOYSTICK|SDL_INIT_HAPTIC
SDL_Haptic* haptic;
unsigned int supported;

/* --- structure holding effect addresses --- */
struct effects_t {
	int effect_constant_id;
	int effect_leftramp_id;
	int effect_rightramp_id;
	int effect_friction_id;
	int effect_leftright_id;
	int effect_sine_id;
	int effect_spring_id;
	int effect_vibration_id;
	int effect_inertia_id;
	int effect_ramp_id;
	int effect_damper_id;
	int effect_sawtoothup_id;
	int effect_sawtoothdown_id;
	int effect_triangle_id;
} effects;


/* --- close haptic and sql quit --- */
void abort_execution(void);

//void HapticPrintSupported(SDL_Haptic* haptic);
int initHaptic(char* name);
void CreateHapticEffects();
void DumpAvailableHaptics();

/* --- SDL Effects (from Boomslangnz)          --- */
/* we most probably will not use all of them       */
/* But they certainly can be used for test purpose */
void TriggerSineEffect(uint16_t period, uint16_t fadePeriod, double strength);
void TriggerLeftRightEffect(double smallstrength, double largestrength);
void TriggerTriangleEffect(double strength);
void TriggerSawToothUpEffect(double strength);
void TriggerSawToothDownEffect(double strength);
void TriggerConstantEffect(int direction, double strength);
void TriggerSpringEffect(double strength);
void TriggerSpringEffectWithDefaultOption(double strength, bool isDefault);
void TriggerDamperEffect(double strength);
void TriggerInertiaEffect(double strength);
void TriggerFrictionEffect(double strength);
void TriggerFrictionEffectWithDefaultOption(double strength, bool isDefault);
void TriggerRampEffect(double start, double end);
void TriggerAutoCenterEffect(double strength);

/* --- generic function for testing --- */
void TriggerEffect(unsigned int effect,double strength);
/* --- This will stop the effect if it's running. Effects are automatically destroyed when the device is closed. --- */
void clearEffect(int effect);

/* --- additional functions --- */
void CenterWheel();
void dumpSupportedFeatures();

#endif