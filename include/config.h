#ifndef CONFIG_H_
#define CONFIG_H_

//#include "input.h"

#define DEFAULT_CONFIG_PATH "/etc/openffb/config"
#define DEFAULT_GAME_SETTING_PATH "/etc/openffb/games/"

#define MAX_PATH_LENGTH 1024
#define MAX_LINE_LENGTH 1024

typedef struct
{
    char hapticName[MAX_PATH_LENGTH];
    char gameSettingPath[MAX_PATH_LENGTH];
    char defaultGameSetting[MAX_PATH_LENGTH];
    char segaFFBControllerPath[MAX_PATH_LENGTH];
    int  debugLevel;

    // All forces /designated within a game/ will be scaled to exist between MinForce and MaxForce
    // 0-100: you may want to set a min force if your wheel doesn't deliver sufficient forces when values are low
    int minForce;

    // 0-100: you may want to set a max force below 100 if your wheel's forces are stronger than desired @ 100%
    int maxForce;

    // Set to 1 if you want to enable rumble, else 0.
    int enableRumble;
    int reverseRumble;

    // Set to 1 if you want to enable rumble in triggers, else 0.
    int enableRumbleTriggers;

    // Length of a feedback command. While a long period works fine, some games may still require shorter ones.
    int feedbackLength;

    // If a game does not specify its own Centering or Friction forces (e.g. only specifies roll left/right),
    // then a default centering and friction force can be applied here. If a game has any of its own such forces,
    // these values will be overwritten immediately in-game.
    // 0-100; Centering is the force that brings the wheel back to the center. Use -1 to disable behavior.
    int defaultCentering;

    // 0-100; Friction determines how difficult a wheel is to turn. Use -1 to disable behavior.
    int defaultFriction;

    // Set to 1 if you want to enable dynamic strength adjustments, else 0.
    int enableFFBStrengthDynamicAdjustment;

    // Set to 1 if you want FFB strength adjustments to persist between loads, else 0.
    int enableFFBStrengthPersistence;

    // Use this function to set the global gain of the specified haptic device.
    // The user may specify the maximum gain by setting the environment variable SDL_HAPTIC_GAIN_MAX which should be between 0 and 100.
    // All calls to SDL_HapticSetGain() will scale linearly using SDL_HAPTIC_GAIN_MAX as the maximum.
    int globalGain;
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
void DumpConfig();
//FFBConfigStatus parseGameSetting(char *path, InputMappings *inputMappings);

#endif // CONFIG_H_
