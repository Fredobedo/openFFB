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
    .gameSettingPath = "./",
    .defaultGameSetting = "generic",
    .segaFFBControllerPath = "/dev/ttyUSB0",
    .debugLevel = 0,
};

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

       if (strcmp(command, "GAME_SETTING_PATH") == 0)
            strcpy(config.gameSettingPath, getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "DEFAULT_GAME_SETTING") == 0)
            strcpy(config.defaultGameSetting, getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "SEGA_FFB_CONTROLLER_PATH") == 0)
            strcpy(config.segaFFBControllerPath, getNextToken(NULL, " ", &saveptr));
            
        else if (strcmp(command, "DEBUG_MODE") == 0)
            config.debugLevel = atoi(getNextToken(NULL, " ", &saveptr));

        else
            printf("Error: Unknown configuration command %s\n", command);
    }

    fclose(file);

    return FFB_CONFIG_STATUS_SUCCESS;
}