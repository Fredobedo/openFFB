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
    .gameProfilePath = DRIVING_PROFILE_PATH,
    .gameProfile = "generic-driving",
    //.segaFFBControllerPath = "/home/fred/Documents/openFFB/virtualSerialOUT",
    .segaFFBControllerPath = "/dev/serial/by-id/usb-1a86_USB_Serial-if00-port0",
    .debugLevel = 0,
    .globalGain = 100,
    .initializationGain = 100,
    .autoCenter =0,
    .minSpring = 0,
    .maxSpring = 100,
    .minFriction = 0,
    .maxFriction = 100,
    .minTorque = 0,
    .maxTorque = 100,
    .minIntensity = 0,
    .maxIntensity = 100,
    .periodAdjustmentFactor = 1.0,
    .logitechSteeringRange = 900,
    .SendWheelPositionToMidi = 0,
    .InvertedWheelPosition = 1,
};

void DumpConfig()
{
    printf("------------------------------------------------------------------------\n");
    printf("-- Dump configuration:\n");
    printf("------------------------------------------------------------------------\n");    
    printf("gameProfilePath         = %s\n", config.gameProfilePath);
    printf("gameProfile             = %s\n", config.gameProfile);
    printf("segaFFBControllerPath   = %s\n", config.segaFFBControllerPath);
    printf("debugLevel              = %d\n", config.debugLevel);
    printf("globalGain              = %d\n", config.globalGain);
    printf("initializationGain      = %d\n", config.initializationGain);
    printf("autoCenter              = %d\n", config.autoCenter);
    printf("minSpring               = %d\n", config.minSpring);
    printf("maxSpring               = %d\n", config.maxSpring);
    printf("minFriction             = %d\n", config.minFriction);
    printf("maxFriction             = %d\n", config.maxFriction);
    printf("minTorque               = %d\n", config.minTorque);
    printf("maxTorque               = %d\n", config.maxTorque);  
    printf("minIntensity            = %d\n", config.minIntensity);
    printf("maxIntensity            = %d\n", config.maxIntensity);  
    printf("periodAdjustmentFactor  = %f\n", config.periodAdjustmentFactor);
    printf("staticSpring            = %d\n", config.minTorque);
    printf("staticFriction          = %d\n", config.maxTorque); 
    printf("logitechSteeringRange   = %d\n", config.logitechSteeringRange);
    printf("SendWheelPositionToMidi = %d\n", config.SendWheelPositionToMidi);
    printf("InvertedWheelPosition   = %d\n", config.InvertedWheelPosition);
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

        else if (strcmp(command, "DEFAULT_DRIVING_PROFILE") == 0)
            strcpy(config.gameProfile, getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "DEBUG_MODE") == 0)
            config.debugLevel = atoi(getNextToken(NULL, " ", &saveptr));
        
        else
            printf("Error: Unknown configuration command %s\n", command);
    }

    fclose(file);

    return FFB_CONFIG_STATUS_SUCCESS;
}

FFBConfigStatus parseDrivingProfile(char *path)
{
    printf("parseDrivingProfile %s\n", path);
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

        if (strcmp(command, "GLOBAL_GAIN") == 0)
            config.globalGain = atoi(getNextToken(NULL, " ", &saveptr));
        if (strcmp(command, "INITIALIZATION_GAIN") == 0)
            config.initializationGain = atoi(getNextToken(NULL, " ", &saveptr));
        else if (strcmp(command, "AUTO_CENTER") == 0)
            config.autoCenter = atoi(getNextToken(NULL, " ", &saveptr));   
        else if (strcmp(command, "MIN_SPRING") == 0)
            config.minSpring = atoi(getNextToken(NULL, " ", &saveptr));   
        else if (strcmp(command, "MAX_SPRING") == 0)
            config.maxSpring = atoi(getNextToken(NULL, " ", &saveptr));   
        else if (strcmp(command, "MIN_FRICTION") == 0)
            config.minFriction = atoi(getNextToken(NULL, " ", &saveptr));   
        else if (strcmp(command, "MAX_FRICTION") == 0)
            config.maxFriction = atoi(getNextToken(NULL, " ", &saveptr));   
        else if (strcmp(command, "MIN_TORQUE") == 0)
            config.minTorque = atoi(getNextToken(NULL, " ", &saveptr));                                       
        else if (strcmp(command, "MAX_TORQUE") == 0)
            config.maxTorque = atoi(getNextToken(NULL, " ", &saveptr));   
        else if (strcmp(command, "MIN_INTENSITY") == 0)
            config.minIntensity = atoi(getNextToken(NULL, " ", &saveptr));   
        else if (strcmp(command, "MAX_INTENSITY") == 0)
            config.maxIntensity = atoi(getNextToken(NULL, " ", &saveptr));
        else if (strcmp(command, "PERIOD_ADJUSTMENT_FACTOR") == 0)
            config.periodAdjustmentFactor = atof(getNextToken(NULL, " ", &saveptr));
        else if (strcmp(command, "STATIC_SPRING") == 0)
            config.staticSpring = atoi(getNextToken(NULL, " ", &saveptr));                                       
        else if (strcmp(command, "STATIC_FRICTION") == 0)
            config.staticFriction = atoi(getNextToken(NULL, " ", &saveptr));    
        else if (strcmp(command, "LOGITECH_STEERING_RANGE") == 0)
            config.logitechSteeringRange = atoi(getNextToken(NULL, " ", &saveptr)); 
        else if (strcmp(command, "SEND_WHEEL_POSITION_TO_MIDI") == 0)
            config.SendWheelPositionToMidi = atoi(getNextToken(NULL, " ", &saveptr)); 
        else if (strcmp(command, "INVERTED_WHEEL_POSITION") == 0)
            config.InvertedWheelPosition = atoi(getNextToken(NULL, " ", &saveptr)); 
        else
            printf("Error: Unknown configuration command %s\n", command);
    }

    fclose(file);

    return FFB_CONFIG_STATUS_SUCCESS;
}