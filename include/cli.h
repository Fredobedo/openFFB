#ifndef CLI_H_
#define CLI_H_

#include <stdbool.h>
#include <linux/input.h>

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
    {"SINE", FF_SINE},
    {"SQUARE", FF_SQUARE},
    {"TRIANGLE", FF_TRIANGLE},
    {"SAWUP",   FF_SAW_UP},
    {"SAWDOWN", FF_SAW_DOWN},
    {"CONSTANT", FF_CONSTANT},
    {"SPRING", FF_SPRING},
    {"DAMPER", FF_DAMPER},
    {"INERTIA", FF_INERTIA},
    {"FRICTION", FF_FRICTION},
    {"RAMP", FF_RAMP},
    {"AUTOCENTER", FF_AUTOCENTER},
    {"RUMBLE", FF_RUMBLE},
};

struct arguments_t {
    char haptic_name[128];
    char game_profile[128];
    struct keyvalue_t{
        enum mode_t{ NOTE_SET=0, GET_AVAILABLE_HAPTICS, LOAD_RAW_EFFECT_FILE, GET_SUPPORTED_EFFECTS, TRIGGER_EFFECT, SET_FORCE, TRIGGER_SEGA_FFB_RAW_REQUEST} mode;
        char value[64];
    }keyvalue[10];
}arguments;

bool containArgument(int mode);
char* getArgumentValue(int mode);

FFBCLIStatus parseArguments(int argc, char **argv);

FFBCLIStatus printUsage();
FFBCLIStatus printVersion();

unsigned int hapticEffectFromString(char *effectString);

#endif // CLI_H_
