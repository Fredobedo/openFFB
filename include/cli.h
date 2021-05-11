#ifndef CLI_H_
#define CLI_H_

#include <stdbool.h>
#include <SDL2/SDL_haptic.h>

typedef enum
{
    FFB_CLI_STATUS_ERROR = 0,
    FFB_CLI_STATUS_SUCCESS_CLOSE,
    FFB_CLI_STATUS_SUCCESS_CONTINUE,
} FFBCLIStatus;

static const struct
{
    const char *string;
    unsigned int FFB_EFFECT;
} hapticEffectConversion[] = {
    {"SDL_HAPTIC_SINE", SDL_HAPTIC_SINE},
    {"SDL_HAPTIC_LEFTRIGHT", SDL_HAPTIC_LEFTRIGHT},
    {"SDL_HAPTIC_TRIANGLE", SDL_HAPTIC_TRIANGLE},
    {"SDL_HAPTIC_SAWTOOTHUP", SDL_HAPTIC_SAWTOOTHUP},
    {"SDL_HAPTIC_SAWTOOTHDOWN", SDL_HAPTIC_SAWTOOTHDOWN},
    {"SDL_HAPTIC_CONSTANT", SDL_HAPTIC_CONSTANT},
    {"SDL_HAPTIC_SPRING", SDL_HAPTIC_SPRING},
    {"SDL_HAPTIC_DAMPER", SDL_HAPTIC_DAMPER},
    {"SDL_HAPTIC_INERTIA", SDL_HAPTIC_INERTIA},
    {"SDL_HAPTIC_FRICTION", SDL_HAPTIC_FRICTION},
    {"SDL_HAPTIC_RAMP", SDL_HAPTIC_RAMP},
    {"SDL_HAPTIC_AUTOCENTER", SDL_HAPTIC_AUTOCENTER},
};

FFBCLIStatus parseArguments(int argc, char **argv, char *haptic, int* dumpSupportedEffects, unsigned int* triggerSDLeffect, double* SDLStrengh, char* triggerSegaRawRequest);
FFBCLIStatus printVersion();

unsigned int hapticEffectFromString(char *effectString);

#endif // CLI_H_
