#ifndef CONFIG_H_
#define CONFIG_H_

//#include "input.h"

#define CONFIG_PATH "/etc/openffb/config"
#define DRIVING_PROFILE_PATH "/etc/openffb/games/"

#define MAX_PATH_LENGTH 1024
#define MAX_LINE_LENGTH 1024

typedef struct
{
    char hapticName[MAX_PATH_LENGTH];
    char gameProfilePath[MAX_PATH_LENGTH];
    char gameProfile[MAX_PATH_LENGTH];
    char segaFFBControllerPath[MAX_PATH_LENGTH];
    int  debugLevel;

    // All forces /designated within a game/ will be scaled to exist between MinForce and MaxForce
    // 0-100: you may want to set a min force if your wheel doesn't deliver sufficient forces when values are low
    int minForce;

    // 0-100: you may want to set a max force below 100 if your wheel's forces are stronger than desired @ 100%
    int maxForce;

    // Use this function to set the global gain of the specified haptic device.
    // The user may specify the maximum gain by setting the environment variable SDL_HAPTIC_GAIN_MAX which should be between 0 and 100.
    // All calls to SDL_HapticSetGain() will scale linearly using SDL_HAPTIC_GAIN_MAX as the maximum.
    int globalGain;

    //0-100: auto center force
    int autoCenter;
} FFBConfig;

typedef enum
{
    FFB_CONFIG_STATUS_ERROR = 0,
    FFB_CONFIG_STATUS_SUCCESS = 1,
    FFB_CONFIG_STATUS_FILE_NOT_FOUND,
    FFB_CONFIG_STATUS_PARSE_ERROR,
} FFBConfigStatus;

FFBConfig *getConfig();
FFBConfigStatus parseConfig(char *path);
FFBConfigStatus parseDrivingProfile(char *path);
void DumpConfig();
//FFBConfigStatus parseGameSetting(char *path, InputMappings *inputMappings);

#endif // CONFIG_H_
