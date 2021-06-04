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

    // Use this function to set the global gain of the specified haptic device. (0-100)
    int globalGain;

    //auto center force (0-100)
    int autoCenter;

    // Effects will be scaled here (0-100) 
    int minSpring;
    int maxSpring;

    int minFriction;
    int maxFriction;

    int minTorque;
    int maxTorque;

    int staticSpring;
    int staticFriction;
    int logitechSteeringRange;

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
