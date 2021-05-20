#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#include "io.h"

char *getNextToken(char *buffer, char *seperator, char **saveptr)
{
    char *token = strtok_r(buffer, seperator, saveptr);
    if (token == NULL)
        return NULL;

    for (int i = 0; i < (int)strlen(token); i++)
    {
        if ((token[i] == '\n') || (token[i] == '\r'))
        {
            token[i] = 0;
        }
    }
    return token;
}

FFBConfig config = {
    .hapticName="",
    .gameSettingPath = "./",
    .defaultGameSetting = "generic",
    .segaFFBControllerPath = "/home/fred/Documents/openFFB/virtualSerialOUT",
//    .segaFFBControllerPath = "/dev/serial/by-id/usb-1a86_USB_Serial-if00-port0",
    .debugLevel = 0,
    .minForce = 0,
    .maxForce = 100,
    .globalGain = 0,
    .autoCenter =0
};

void DumpConfig()
{
    printf("------------------------------------------------------------------\n");
    printf("-- Dump configuration:\n");
    printf("------------------------------------------------------------------\n");    
    printf("gameSettingPath                    = %s\n", config.gameSettingPath);
    printf("defaultGameSetting                 = %s\n", config.defaultGameSetting);
    printf("segaFFBControllerPath              = %s\n", config.segaFFBControllerPath);
    printf("debugLevel                         = %d\n", config.debugLevel);
    printf("minForce                           = %d\n", config.minForce);
    printf("maxForce                           = %d\n", config.maxForce);
    printf("globalGain                         = %d\n", config.globalGain);
    printf("autoCenter                         = %d\n", config.autoCenter);
}

FFBConfig *getConfig()
{
    return &config;
}

FFBConfigStatus parseConfig(char *path)
{
    FILE *file;
    char buffer[MAX_LINE_LENGTH];
    char *saveptr = NULL;

    if ((file = fopen(path, "r")) == NULL)
        return FFB_CONFIG_STATUS_FILE_NOT_FOUND;

    while (fgets(buffer, MAX_LINE_LENGTH, file))
    {

        /* Check for comments */
        if (buffer[0] == '#' || buffer[0] == 0 || buffer[0] == ' ' || buffer[0] == '\r' || buffer[0] == '\n')
            continue;

        char *command = getNextToken(buffer, " ", &saveptr);


       if (strcmp(command, "SEGA_FFB_CONTROLLER_PATH") == 0)
            strcpy(config.segaFFBControllerPath, getNextToken(NULL, " ", &saveptr));

       else if (strcmp(command, "HAPTIC_NAME") == 0)
            strcpy(config.hapticName, getNextToken(NULL, " ", &saveptr));

       else if (strcmp(command, "GAME_SETTING_PATH") == 0)
            strcpy(config.gameSettingPath, getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "DEFAULT_GAME_SETTING") == 0)
            strcpy(config.defaultGameSetting, getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "DEBUG_MODE") == 0)
            config.debugLevel = atoi(getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "MIN_FORCE") == 0)
            config.minForce = atoi(getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "MAX_FORCE") == 0)
            config.maxForce = atoi(getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "GLOBAL_GAIN") == 0)
            config.globalGain = atoi(getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "AUTO_CENTER") == 0)
            config.autoCenter = atoi(getNextToken(NULL, " ", &saveptr));            
        else
            printf("Error: Unknown configuration command %s\n", command);
    }

    fclose(file);

    return FFB_CONFIG_STATUS_SUCCESS;
}