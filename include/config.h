#ifndef CONFIG_H_
#define CONFIG_H_

//#include "input.h"

#define DEFAULT_CONFIG_PATH "/etc/openjvs/config"
#define DEFAULT_GAME_SETTING_PATH "/etc/openjvs/games/"

#define MAX_PATH_LENGTH 1024
#define MAX_LINE_LENGTH 1024

typedef struct
{
    char gameSettingPath[MAX_PATH_LENGTH];
    char defaultGameSetting[MAX_PATH_LENGTH];
    char segaFFBControllerPath[MAX_PATH_LENGTH];
    int  debugLevel;
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
//FFBConfigStatus parseGameSetting(char *path, InputMappings *inputMappings);

#endif // CONFIG_H_
