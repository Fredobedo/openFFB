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
    .enableRumble = 1,
    .reverseRumble = 0,
    .enableRumbleTriggers = 1,
    .feedbackLength = 500,
    .defaultCentering = 0,
    .defaultFriction = 0,
    .enableFFBStrengthDynamicAdjustment = 0,
    .enableFFBStrengthPersistence = 0,
    .globalGain = 0
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
    printf("enableRumble                       = %d\n", config.enableRumble);
    printf("reverseRumble                      = %d\n", config.reverseRumble);
    printf("enableRumbleTriggers               = %d\n", config.enableRumbleTriggers);
    printf("feedbackLength                     = %d\n", config.feedbackLength);
    printf("defaultCentering                   = %d\n", config.defaultCentering);
    printf("defaultFriction                    = %d\n", config.defaultFriction);
    printf("enableFFBStrengthDynamicAdjustment = %d\n", config.enableFFBStrengthDynamicAdjustment);
    printf("enableFFBStrengthPersistence       = %d\n", config.enableFFBStrengthPersistence);
    printf("globalGain                         = %d\n", config.globalGain);

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

        else if (strcmp(command, "ENABLE_RUMBLE") == 0)
            config.enableRumble = atoi(getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "REVERSE_RUMBLE") == 0)
            config.reverseRumble = atoi(getNextToken(NULL, " ", &saveptr));     

        else if (strcmp(command, "ENABLE_RUMBLE_TRIGGERS") == 0)
            config.enableRumbleTriggers = atoi(getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "FEEDBACK_LENGTH") == 0)
            config.feedbackLength = atoi(getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "DEFAULT_CENTERING") == 0)
            config.defaultCentering = atoi(getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "DEFAULT_FRICTION") == 0)
            config.defaultFriction = atoi(getNextToken(NULL, " ", &saveptr));    

        else if (strcmp(command, "ENABLE_FFB_STRENGTH_DYNAMIC_ADJUSTMENT") == 0)
            config.enableFFBStrengthDynamicAdjustment = atoi(getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "ENABLE_FFB_STRENGTH_PERSISTENCE") == 0)
            config.enableFFBStrengthPersistence = atoi(getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "GLOBAL_GAIN") == 0)
            config.globalGain = atoi(getNextToken(NULL, " ", &saveptr));
        else
            printf("Error: Unknown configuration command %s\n", command);
    }

    fclose(file);

    return FFB_CONFIG_STATUS_SUCCESS;
}