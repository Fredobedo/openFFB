#include <dirent.h>
#include <stdio.h>
#include <string.h>

#include "config.h"
#include "debug.h"
#include "cli.h"

#include "sdlhelper.h"

/**
 * Print usage information
 * 
 * Prints the usage information for the OpenJVS command
 * line interface
 * 
 * @returns The status of the action performed
 **/
FFBCLIStatus printUsage()
{
    debug(0, "Usage: openffb haptic_name\n\n");
    debug(0, "Options:\n");
    debug(0, "  --help                         Displays this text\n");
    debug(0, "  --version                      Displays OpenFFB Version\n");
    debug(0, "  --configuration                Displays OpenFFB Configuration\n");
    debug(0, "  --availableHaptics             Displays list of haptics\n");
    debug(0, "  --supportedEffects             Displays supported effects\n");
    debug(0, "  --triggerSDLEffect:[TYPE]      Activate 1 of these SDL effects:\n");
    debug(0, "                                  - SDL_HAPTIC_SINE\n");
    debug(0, "                                  - SDL_HAPTIC_LEFTRIGHT\n");
    debug(0, "                                  - SDL_HAPTIC_TRIANGLE\n");
    debug(0, "                                  - SDL_HAPTIC_SAWTOOTHUP\n");        
    debug(0, "                                  - SDL_HAPTIC_SAWTOOTHDOWN\n");
    debug(0, "                                  - SDL_HAPTIC_CONSTANT\n");
    debug(0, "                                  - SDL_HAPTIC_SPRING\n"); 
    debug(0, "                                  - SDL_HAPTIC_DAMPER\n");
    debug(0, "                                  - SDL_HAPTIC_INERTIA\n");
    debug(0, "                                  - SDL_HAPTIC_FRICTION\n"); 
    debug(0, "                                  - SDL_HAPTIC_RAMP\n"); 
    debug(0, "                                  - SDL_HAPTIC_AUTOCENTER\n"); 
    debug(0, "\n");    
    debug(0, "   --- NEXT IS FOR DEBUGGING PURPOSE !!!! --- \n");
    debug(0, "\n"); 
    debug(0, "  --SegaFFB4BytesRawRequest:[PACKET]  Activate FFB Effects based on a 4 bytes raw request:\n");
    debug(0, "                                       - D0 => Spring     (0x00->0x7F)\n");
    debug(0, "                                       - D1 => Friction   (0x00->0x7F)\n");
    debug(0, "                                       - D2 => Constant Torque Direction (Left=0x00, Right=0x01) \n");
    debug(0, "                                       - D3 => Constant Torque Power (0x00->0xFF)\n");
    debug(0, "\n");
    debug(0, "  --SegaFFB6BytesRawRequest:[PACKET]  Activate FFB Effects based on a 4 bytes raw request:\n");
    debug(0, "                                       - D0 => Start byte (0x80)\n");
    debug(0, "                                       - D1 => Spring     (0x00->0x7F)\n");
    debug(0, "                                       - D2 => Friction   (0x00->0x7F)\n");
    debug(0, "                                       - D3 => Constant Torque Direction (Left=0x00, Right=0x01) \n");
    debug(0, "                                       - D4 => Constant Torque Power (0x00->0xFF)\n");
    debug(0, "                                       - D5 => CRC        (D1^D2^D3^D4)&0x7F\n");
    debug(0, "\n");
    return FFB_CLI_STATUS_SUCCESS_CLOSE;
}

/**
 * Print version information
 * 
 * Prints the version information for the OpenJVS command
 * line interface
 * 
 * @returns The status of the action performed
 **/
FFBCLIStatus printVersion()
{
    debug(0, "1.0.0\n");
    return FFB_CLI_STATUS_SUCCESS_CLOSE;
}

unsigned int hapticEffectFromString(char *effectString)
{
    for (long unsigned int i = 0; i < sizeof(hapticEffectConversion) / sizeof(hapticEffectConversion[0]); i++)
    {
        if (strcmp(hapticEffectConversion[i].string, effectString) == 0)
            return hapticEffectConversion[i].FFB_EFFECT;
    }
    debug(0, "Error: Could not find the SDL Haptic Effect specified for string %s\n", effectString);
    return 0;    
}
/**
 * Parses the command line arguments
 * 
 * Parses the command line arguments and sets the
 * mapping name if no arguments are set.
 * 
 * @param argc The amount of arguments
 * @param argv Structure holding the arguments
 * @param game Pointer to a char array holding the game setting file name
 * @returns The status of the action performed
 **/
FFBCLIStatus parseArguments(int argc, char **argv, char *haptic, unsigned int effect, char* segaRawRequest)
{
    // If there are no arguments simply continue
    if (argc <= 1)
        return printUsage();


    // Process all of the different arguments people might send
    if (strcmp(argv[1], "--help") == 0)
    {
        return printUsage();
    }
    else if (strcmp(argv[1], "--version") == 0)
    {
        return printVersion();
    }
    else if (strcmp(argv[1], "--availableHaptics") == 0)
    {
        DumpAvailableHaptics();
        return FFB_CLI_STATUS_SUCCESS_CLOSE;
    }    
    else if (strcmp(argv[1], "--configuration") == 0)
    {
        DumpConfig();
        return FFB_CLI_STATUS_SUCCESS_CLOSE;
    }

    /*  --- Parameters with token --- */
    char *command = strtok(argv[1], ":");
    char* token = NULL;

    if(command!=NULL){
        // If the first argument doesn't start with a dash it must be the hapic name
        if (strcmp(command, "--triggerSDLEffect") == 0)
        {
            token=strtok(NULL, ":");
            effect = hapticEffectFromString(token);
            if (effect==-1)
                return FFB_CLI_STATUS_ERROR;
            else
                return FFB_CLI_STATUS_SUCCESS_CONTINUE;
        }

        // If the first argument doesn't start with a dash it must be the hapic name
        if (strcmp(argv[1], "--SegaFFB4BytesRawRequest") == 0)
        {
            token=strtok(NULL, ":");

            // D0 => Start byte (0x80)
            // D5 => CRC        (D1^D2^D3^D4)&0x7F
            sprintf(segaRawRequest, "%d%s%d",0x80,token, token[0]^token[1]^token[2]^token[3]&0x7F);
            printf("segaRawRequest=%s\n", segaRawRequest);

            return FFB_CLI_STATUS_SUCCESS_CONTINUE;
        }   
        // If the first argument doesn't start with a dash it must be the hapic name
        if (strcmp(argv[1], "--SegaFFB6BytesRawRequest") == 0)
        {
            segaRawRequest=strtok(NULL, ":");
            printf("segaRawRequest=%s\n", segaRawRequest);

            return FFB_CLI_STATUS_SUCCESS_CONTINUE;
        }   

    }
    // If none of these where found, the argument is unknown.
    debug(0, "Unknown argument %s\n", argv[1]);
    return FFB_CLI_STATUS_ERROR;
}
