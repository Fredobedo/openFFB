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
    debug(0, "Usage: openffb [HAPTIC_NAME] [OPTION]\n\n");
    debug(0, "Haptic_name:                            Select haptic name or index\n");
    debug(0, "Options:\n");
    debug(0, "  --help                                Displays this text\n");
    debug(0, "  --version                             Displays OpenFFB Version\n");
    debug(0, "  --configuration                       Displays OpenFFB Configuration\n");
    debug(0, "  --availableHaptics                    Displays list of haptics\n");
    debug(0, "  --haptic=[NAME]                       Haptic Name or index\n");
    debug(0, "  --supportedEffects                    Displays supported effects\n");
    debug(0, "  --triggerSDLEffect=[TYPE]             Activate 1 of these SDL effects\n");
    debug(0, "  --force=[1-100]                       The force (strength) of the effect to activate\n");
    debug(0, "                                         - SDL_HAPTIC_SINE\n");
    debug(0, "                                         - SDL_HAPTIC_LEFTRIGHT\n");
    debug(0, "                                         - SDL_HAPTIC_TRIANGLE\n");
    debug(0, "                                         - SDL_HAPTIC_SAWTOOTHUP\n");        
    debug(0, "                                         - SDL_HAPTIC_SAWTOOTHDOWN\n");
    debug(0, "                                         - SDL_HAPTIC_CONSTANT\n");
    debug(0, "                                         - SDL_HAPTIC_SPRING\n"); 
    debug(0, "                                         - SDL_HAPTIC_DAMPER\n");
    debug(0, "                                         - SDL_HAPTIC_INERTIA\n");
    debug(0, "                                         - SDL_HAPTIC_FRICTION\n"); 
    debug(0, "                                         - SDL_HAPTIC_RAMP\n"); 
    debug(0, "                                         - SDL_HAPTIC_AUTOCENTER\n"); 
    debug(0, "\n");    
    debug(0, "   --- NEXT IS FOR DEBUGGING PURPOSE !!!! --- \n");
    debug(0, "\n"); 
    debug(0, "  --4BytesSegaFFBRawRequest=[PACKET]  Activate FFB Effects based on a 4 bytes raw request:\n");
    debug(0, "                                       - D0 => Spring     (0x00->0x7F)\n");
    debug(0, "                                       - D1 => Friction   (0x00->0x7F)\n");
    debug(0, "                                       - D2 => Constant Torque Direction (Left=0x00, Right=0x01) \n");
    debug(0, "                                       - D3 => Constant Torque Power (0x00->0xFF)\n");
    debug(0, "\n");
    debug(0, "  --6BytesSegaFFBRawRequest=[PACKET]  Activate FFB Effects based on a 6 bytes raw request:\n");
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

bool containArgument(int mode)
{
    for (int i = 0; i < sizeof(arguments.keyvalue)/sizeof(arguments.keyvalue[0]); i++) {
        if (arguments.keyvalue[i].mode==mode) 
            return true;
    }
    return false;
}

char* getArgumentValue(int mode)
{
    for (int i = 0; i < sizeof(arguments.keyvalue)/sizeof(arguments.keyvalue[0]); i++) {
        if (arguments.keyvalue[i].mode==mode) 
            return arguments.keyvalue[i].value;
    }
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
FFBCLIStatus parseArguments(int argc, char **argv)
{
    // If there are no arguments simply continue
    if (argc <= 1)
        return printUsage();

    // Process all requests that are not for a specific hapic here:
    if ((strcmp(argv[1], "--help") == 0)                  || (strcmp(argv[1], "-?") == 0)) {
        return printUsage();
    }
    else if ((strcmp(argv[1], "--version") == 0)          || (strcmp(argv[1], "-v") == 0)) {
        return printVersion();
    }
    else if ((strcmp(argv[1], "--availableHaptics") == 0) || (strcmp(argv[1], "-a") == 0)) {
        DumpAvailableHaptics();
        return FFB_CLI_STATUS_SUCCESS_CLOSE;
    }    
    else if ((strcmp(argv[1], "--configuration") == 0)    || (strcmp(argv[1], "-c") == 0)) {
        DumpConfig();
        return FFB_CLI_STATUS_SUCCESS_CLOSE;
    }

    // Store all other requests for a specific hapic here:
    int cpKeyValue=0;
    for (optind = 1; optind < argc ; optind++) {
        /*  --- Parameters with token --- */
        char *command = strtok(argv[optind], "=");
        char* token = NULL;

        if(command!=NULL){
            if ((strcmp(command, "--haptic") == 0)                        || (strcmp(command, "-h") == 0)) {
                strcpy(arguments.haptic_name, strtok(NULL, "="));
            }              
            else if ((strcmp(command, "--supportedEffects") == 0)         || (strcmp(command, "-s") == 0)) {
                arguments.keyvalue[cpKeyValue++].mode=GET_SUPPORTED_EFFECTS;
            }
            else if ((strcmp(command, "--triggerSDLEffect") == 0)         || (strcmp(command, "-t") == 0)) {
                arguments.keyvalue[cpKeyValue++].mode=TRIGGER_SDL_EFFECT;
                strcpy(arguments.keyvalue[cpKeyValue].value,strtok(NULL, "="));
            }
            else if ((strcmp(command, "--force") == 0)                    || (strcmp(command, "-f") == 0)) {
                arguments.keyvalue[cpKeyValue++].mode=SET_FORCE;
                strcpy(arguments.keyvalue[cpKeyValue].value,strtok(NULL, "="));                    
            }
            else if ((strcmp(command, "--4BytesSegaFFBRawRequest") == 0)  || (strcmp(command, "-4") == 0)) {
                arguments.keyvalue[cpKeyValue++].mode=TRIGGER_SEGA_FFB_RAW_REQUEST;
                token=strtok(NULL, "=");
                sprintf(arguments.keyvalue[cpKeyValue].value, "%d%s%d",0x80,token, token[0]^token[1]^token[2]^token[3]&0x7F);
            }
            else if ((strcmp(command, "--6BytesSegaFFBRawRequest") == 0)  || (strcmp(command, "-6") == 0)) {
                arguments.keyvalue[cpKeyValue++].mode=TRIGGER_SEGA_FFB_RAW_REQUEST;
                strcpy(arguments.keyvalue[cpKeyValue].value,strtok(NULL, "="));
            }
            else{
                debug(0, "Unknown argument '%s'\n", command);
            }
        }
    }
    if(cpKeyValue> 0) {
        if(strlen(arguments.haptic_name)==0) {
            debug(0, "--hapic parameter is required\n");
            return FFB_CLI_STATUS_ERROR;
        }
        return FFB_CLI_STATUS_SUCCESS_CONTINUE;
    }
    else 
        return FFB_CLI_STATUS_ERROR;
}

