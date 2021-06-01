#include <dirent.h>
#include <stdio.h>
#include <string.h>

#include "config.h"
#include "debug.h"
#include "cli.h"

#include "ffbhelper.h"
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
    debug(0, "Usage: openffb [HAPTIC_NAME] [OPTIONS]\n\n");
    debug(0, "Haptic_name:                            Select haptic name or index\n");
    debug(0, "Options:\n");
    debug(0, "  -?,  --help                                Displays this text\n");
    debug(0, "  -v,  --version                             Displays OpenFFB Version\n");
    debug(0, "  -c,  --configuration                       Displays OpenFFB Configuration\n");
    debug(0, "  -a,  --availableHaptics                    Displays list of haptics\n");
    debug(0, "  -h,  --haptic=[NAME/IDX]                   Haptic Name or index\n");
    debug(0, "  -gp, --gameProfile=[NAME]                  Load game specific settings\n");    
    debug(0, "  -s,  --supportedEffects                    Displays supported effects\n");
    debug(0, "  -t,  --triggerEffect=[TYPE]                Activate 1 of these effects\n");
    debug(0, "  -f,  --force=[1-100]                       Strength of the triggered effect\n");
    //debug(0, "                                             - SINE\n");
    //debug(0, "                                             - SQUARE\n");
    //debug(0, "                                         - TRIANGLE\n");            //NOT SUPPORTED YET
    //debug(0, "                                         - SAWUP\n");               //NOT SUPPORTED YET
    //debug(0, "                                         - SAWDOWN\n");             //NOT SUPPORTED YET
    debug(0, "                                             - CONSTANT\n");
    debug(0, "                                             - SPRING\n"); 
    //debug(0, "                                         - DAMPER\n");              //NOT SUPPORTED YET
    //debug(0, "                                         - INERTIA\n");             //NOT SUPPORTED YET
    debug(0, "                                             - FRICTION\n"); 
    //debug(0, "                                         - RAMP\n");                //NOT SUPPORTED YET
    debug(0, "                                             - AUTOCENTER\n"); 
    debug(0, "                                             - RUMBLE\n"); 
    debug(0, "\n");    
    debug(3, "   --- NEXT IS FOR DEBUGGING PURPOSE !!!! --- \n");
    debug(3, "\n"); 
    debug(3, "  -4,  --4BytesSegaFFBRawRequest=[PACKET]  Activate FFB Effects based on a 4 bytes raw request:\n");
    debug(3, "                                       - D0 => Spring     (00->7F)\n");
    debug(3, "                                       - D1 => Friction   (00->7F)\n");
    debug(3, "                                       - D2 => Constant Torque Direction (Left=00, Right=01) \n");
    debug(3, "                                       - D3 => Constant Torque Power (00->FF)\n");
    debug(3, "\n");
    debug(3, "  -6,  --6BytesSegaFFBRawRequest=[PACKET]  Activate FFB Effects based on a 6 bytes raw request:\n");
    debug(3, "                                       - D0 => Start byte (80)\n");
    debug(3, "                                       - D1 => Spring     (00->7F)\n");
    debug(3, "                                       - D2 => Friction   (00->7F)\n");
    debug(3, "                                       - D3 => Constant Torque Direction (Left=00, Right=01) \n");
    debug(3, "                                       - D4 => Constant Torque Power (00->FF)\n");
    debug(3, "                                       - D5 => CRC        (D1^D2^D3^D4)&7F\n");
    debug(3, "\n");
    debug(3, "  -lf,  --loadFile=[NAME]                    Load raw input file\n");
    debug(3, "\n");
    //debug(0, "Examples:\n");
    //debug(0, "openffb -h=0  -> \n");
    //debug(0, "openffb -h=0  -> \n");

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
    debug(0, "Error: Could not find the Haptic Effect specified for string %s\n", effectString);
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

 unsigned char ahex2bin(unsigned char MSB, unsigned char LSB) {  
    if (MSB > '9') MSB -= 7;          // Convert MSB value to a contiguous range (0x30..0x3F)  
    if (LSB > '9') LSB -= 7;          // Convert LSB value to a contiguous range (0x30..0x3F)  
     return (MSB <<4) | (LSB & 0x0F); // Make a result byte  using only low nibbles of MSB and LSB thus neglecting the input register case
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
        FFBDumpAvailableDevices();
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
        char *command = strtok(argv[optind], "=:");
        char* token = NULL;

        if(command!=NULL){
           
            if ((strcmp(command, "--haptic") == 0)                        || (strcmp(command, "-h") == 0)) {
                strcpy(arguments.haptic_name, strtok(NULL, "="));
            }              
            else if ((strcmp(command, "--gameProfile") == 0)         || (strcmp(command, "-gp") == 0)) {
                strcpy(arguments.game_profile, strtok(NULL, "="));
            }
            else if ((strcmp(command, "--supportedEffects") == 0)         || (strcmp(command, "-s") == 0)) {
                arguments.keyvalue[cpKeyValue].mode=GET_SUPPORTED_EFFECTS;
                cpKeyValue++;
            }
            else if ((strcmp(command, "--triggerEffect") == 0)         || (strcmp(command, "-t") == 0)) {
                arguments.keyvalue[cpKeyValue].mode=TRIGGER_EFFECT;
                strcpy(arguments.keyvalue[cpKeyValue].value,strtok(NULL, "="));
                cpKeyValue++;
            }
            else if ((strcmp(command, "--force") == 0)                    || (strcmp(command, "-f") == 0)) {
                arguments.keyvalue[cpKeyValue].mode=SET_FORCE;
                strcpy(arguments.keyvalue[cpKeyValue].value,strtok(NULL, "="));
                cpKeyValue++;                                    
            }
            else if ((strcmp(command, "--loadFile") == 0)                    || (strcmp(command, "-lf") == 0)) {
                arguments.keyvalue[cpKeyValue].mode=LOAD_RAW_EFFECT_FILE;
                strcpy(arguments.keyvalue[cpKeyValue].value,strtok(NULL, "="));
                cpKeyValue++;                                    
            }            
            else if ((strcmp(command, "--4BytesSegaFFBRawRequest") == 0)  || (strcmp(command, "-4") == 0)) {
                arguments.keyvalue[cpKeyValue].mode=TRIGGER_SEGA_FFB_RAW_REQUEST;
                token=strtok(NULL, "=");
                
                unsigned char AsciiHexToBin[4]={ahex2bin(token[0],token[1]),
                                                ahex2bin(token[2],token[3]),
                                                ahex2bin(token[4],token[5]),
                                                ahex2bin(token[6],token[7])};

                sprintf(arguments.keyvalue[cpKeyValue].value, "%c%c%c%c%c%c",
                    0x80,
                    AsciiHexToBin[0], 
                    AsciiHexToBin[1],
                    AsciiHexToBin[2],
                    AsciiHexToBin[3],
                    (AsciiHexToBin[0]^AsciiHexToBin[1]^AsciiHexToBin[2]^AsciiHexToBin[3])&0x7F);
                
                cpKeyValue++;
            }
            else if ((strcmp(command, "--6BytesSegaFFBRawRequest") == 0)  || (strcmp(command, "-6") == 0)) {
                arguments.keyvalue[cpKeyValue].mode=TRIGGER_SEGA_FFB_RAW_REQUEST;
                token=strtok(NULL, "=");
                
                sprintf(arguments.keyvalue[cpKeyValue].value, "%c%c%c%c%c%c",
                                                ahex2bin(token[0],token[1]),
                                                ahex2bin(token[2],token[3]),
                                                ahex2bin(token[4],token[5]),
                                                ahex2bin(token[6],token[7]),
                                                ahex2bin(token[8],token[9]),
                                                ahex2bin(token[10],token[11]));
                                                
                cpKeyValue++;                
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
        else
            return FFB_CLI_STATUS_SUCCESS_CONTINUE;
    }
    else if(strlen(arguments.haptic_name)==0){ 
        return FFB_CLI_STATUS_ERROR;
    }
    else
        return FFB_CLI_STATUS_SUCCESS_CONTINUE;
}

