#ifndef SDLHELPER_H_
#define SDLHELPER_H_

#include <stdint.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_haptic.h>

SDL_Haptic* haptic;

void abort_execution(void);
void HapticPrintSupported(SDL_Haptic* haptic);
int initHaptic(char* name);
void DumpAvailableHaptics();

void TriggerSpringEffect(int strength);
void TriggerFrictionEffect(int strength);
void TriggerLeftRightEffect(int direction, int strength);

void CenterWheel();
void dumpSupportedFeatures();

#endif