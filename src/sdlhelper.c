    
#include "debug.h"
#include "sdlhelper.h"
#include <time.h>
    
#include<stdio.h>
#include<string.h>


int initHaptic(char* name)
{
    int idxDevice=-1;

    /* Initialize the force feedbackness */
    SDL_Init(SDL_INIT_TIMER | SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC);
    SDL_Log("%d Haptic devices detected.\n", SDL_NumHaptics());
    if (SDL_NumHaptics() > 0) {
        /* Try to find matching device */
        for (int i = 0; i < SDL_NumHaptics(); i++) {
            if (strcmp(SDL_HapticName(i), name) != 0){
                idxDevice=i;
                break;
            }
        }

        if (idxDevice ==-1) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Unable to find device matching '%s', aborting.\n",
                name);
            return 0;
        }


        haptic = SDL_HapticOpen(idxDevice);
        if (haptic == NULL) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Unable to create the haptic device: %s\n",
                SDL_GetError());
            return 0;
        }
        SDL_Log("Device: %s\n", SDL_HapticName(idxDevice));
        return 1;
    }
    else {
        return 0;
    }
}

void DumpAvailableHaptics()
{
    if (SDL_NumHaptics() > 0) {
        SDL_Log("Available Haptics:\n");
        SDL_Log("------------------\n");
        /* Try to find matching device */
        for (int i = 0; i < SDL_NumHaptics(); i++) {
            SDL_Log("Device[%d]: %s\n", i, SDL_HapticName(i));
        }
    } 
    else
        SDL_Log("Warning, no haptic found!\n");   
}
/*
 * Cleans up a bit.
 */
void abort_execution(void)
{
    SDL_Log("\nAborting program execution.\n");

    SDL_HapticClose(haptic);
    SDL_Quit();
}


/*
 * Displays information about the haptic device.
 */
char* CheckEffect(unsigned int supported, unsigned int check)
{
    if (supported & check)
        return GREEN "-> OK <-" RESET "\n";
    else
        return RED "Not supported!" RESET "\n";
}

void dumpSupportedFeatures()
{
    unsigned int supported;

    supported = SDL_HapticQuery(haptic);
    SDL_Log("------------------------------------------------------------------\n");
    SDL_Log("-- Checking capabilities:\n");
    SDL_Log("------------------------------------------------------------------\n");
    SDL_Log("   Nbr of programmable effects for this device: %d\n", SDL_HapticNumEffects(haptic));
    SDL_Log("   Nbr of effects the device can play at the same time: %d\n", SDL_HapticNumEffectsPlaying(haptic));
    SDL_Log("\n");
    SDL_Log("     Supported periodic effects:\n");
    SDL_Log("      - sine:         %s", CheckEffect(supported, SDL_HAPTIC_SINE));
    SDL_Log("      - left/right:   %s", CheckEffect(supported, SDL_HAPTIC_LEFTRIGHT));
    SDL_Log("      - triangle:     %s", CheckEffect(supported, SDL_HAPTIC_TRIANGLE));
    SDL_Log("      - sawtoothup:   %s", CheckEffect(supported, SDL_HAPTIC_SAWTOOTHUP));
    SDL_Log("      - sawtoothdown: %s", CheckEffect(supported, SDL_HAPTIC_SAWTOOTHDOWN));
    SDL_Log("\n");
    SDL_Log("     Supported constant effect:\n");
    SDL_Log("      - constant:     %s", CheckEffect(supported, SDL_HAPTIC_CONSTANT));
    SDL_Log("\n");
    SDL_Log("     Supported condition effects:\n");
    SDL_Log("      - spring:       %s", CheckEffect(supported, SDL_HAPTIC_SPRING));
    SDL_Log("      - damper:       %s", CheckEffect(supported, SDL_HAPTIC_DAMPER));
    SDL_Log("      - inertia:      %s", CheckEffect(supported, SDL_HAPTIC_INERTIA));
    SDL_Log("      - friction:     %s", CheckEffect(supported, SDL_HAPTIC_FRICTION));
    SDL_Log("\n");
    SDL_Log("     Supported ramp effect:\n");
    SDL_Log("      - ramp:         %s", CheckEffect(supported, SDL_HAPTIC_RAMP));
    SDL_Log("\n");
    SDL_Log("     Supported custom effect:\n");
    SDL_Log("      - custom:       %s", CheckEffect(supported, SDL_HAPTIC_CUSTOM));
    SDL_Log("\n");
    SDL_Log("     Supported global features:\n");
    SDL_Log("      - gain:        %s", CheckEffect(supported, SDL_HAPTIC_GAIN));
    SDL_Log("      - autocenter:  %s", CheckEffect(supported, SDL_HAPTIC_AUTOCENTER));
    SDL_Log("      - status:      %s", CheckEffect(supported, SDL_HAPTIC_STATUS));
    SDL_Log("      - pause:       %s", CheckEffect(supported, SDL_HAPTIC_PAUSE));
}

void TriggerSpringEffect(int strength)
{

}

void TriggerFrictionEffect(int strength)
{

}

void TriggerSpringEffectInfinite(double strength)
{

}

void TriggerLeftRightEffect(int direction, int strength)
{

}

void CenterWheel()
{ 

}