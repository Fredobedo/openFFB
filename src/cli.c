#include <dirent.h>
#include <stdio.h>
#include <string.h>

#include "config.h"
#include "debug.h"
#include "cli.h"
#include <ctype.h> 

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
    debug(0, "  -t,  --triggerEffect=[TYPE]                Activate 1 effect: {CONSTANT,SPRING,FRICTION,AUTOCENTER,SINE,RUMBLE}\n"); 
    debug(0, "  -f,  --force=[1-100]                       Strength of the triggered effect\n");

    debug(0, "\n");    
    debug(3, "   --- NEXT IS FOR DEBUGGING PURPOSE !!!! --- \n");
    debug(3, "\n"); 
    debug(3, "  -4,  --4BytesSegaFFBRawRequest=[PACKET]  Activate FFB Effects based on a 4 bytes raw request:\n");
    debug(3, "                                       - D0 => Spring     (00->7F)\n");
    debug(3, "                                       - D1 => Friction   (00->7F)\n");
    debug(3, "                                       - D2 => Constant Torque Direction (Left=01, Right=00) \n");
    debug(3, "                                       - D3 => Constant Torque Power (00->FF)\n");
    debug(3, "\n");
    debug(3, "  -7,  --7BytesSegaFFBRawRequest=[PACKET]  Activate FFB Effects based on a 7 bytes raw request:\n");
    debug(3, "                                       - D0 => Start byte (80)\n");
    debug(3, "                                       - D1 => Spring     (00->7F)\n");
    debug(3, "                                       - D2 => Friction   (00->7F)\n");
    debug(3, "                                       - D3 => Constant Torque Direction (Left=01, Right=00) \n");
    debug(3, "                                       - D4 => Constant Torque Power (00->7F)\n");
    debug(3, "                                       - D5 => Sine Frequency (00->7F)\n");
    debug(3, "                                       - D6 => Sine Intensity (00->7F)\n");
    debug(3, "\n");
    debug(3, "  -8,  --8BytesSegaFFBRawRequest=[PACKET]  Activate FFB Effects based on a 8 bytes raw request:\n");
    debug(3, "                                       - D0 => Start byte (80)\n");
    debug(3, "                                       - D1 => Spring     (00->7F)\n");
    debug(3, "                                       - D2 => Friction   (00->7F)\n");
    debug(3, "                                       - D3 => Constant Torque Direction (Left=01, Right=00) \n");
    debug(3, "                                       - D4 => Constant Torque Power (00->7F)\n");
    debug(3, "                                       - D5 => Sine Frequency (00->7F)\n");
    debug(3, "                                       - D6 => Sine Intensity (00->7F)\n");
    debug(3, "                                       - D7 => CRC        (D1^D2^D3^D4^D5^D6)&7F\n");
    debug(3, "\n");
    debug(3, "  -lf,  --loadFile=[NAME]                    Load raw input file\n");
    debug(3, "\n");
    debug(3, "  -dr,  --dumpRaw=[PACKET_LENGHT]            Dump SEGA FFB Controller packets only\n");
    debug(3, "\n");

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
    debug(0, "2.0.1\n");
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

 int parse_hex_string(const char *input, unsigned char *output) {
    
    if (strlen(input) != 8) return -1;

    for (int i = 0; i < 8; i++) {
        if (!isxdigit((unsigned char)input[i])) return -1;
    }

    output[0] = ahex2bin(input[0], input[1]);
    output[1] = ahex2bin(input[2], input[3]);
    output[2] = ahex2bin(input[4], input[5]);
    output[3] = ahex2bin(input[6], input[7]);

    return 0;
}

//Try to parse user input for sleep request. Ex: Sleep(15)
void SleepFromInput(char* input) {
    int sleep_duration = 0;       // Default duration if no value provided
    char* arg_start = input + 5;  // Pointer to character after "sleep"
    char *endptr;
    bool valid_syntax = false;

    // Skip optional space between 'sleep' and '('
    while (*arg_start == ' ') arg_start++;

    if (*arg_start != '\0' && *arg_start == '(') {
        char* open_paren = strchr(arg_start, '(');
        
        if (open_paren) {
            char* close_paren = strchr(open_paren, ')');
            
            if (close_paren && close_paren > open_paren) {
                char temp_buf[64];
                size_t num_len = close_paren - open_paren - 1;  // Length of text inside parens
                
                if (num_len < sizeof(temp_buf)) {
                    strncpy(temp_buf, open_paren + 1, num_len);
                    temp_buf[num_len] = '\0';
                    
                    // Attempt parse
                    long val = strtol(temp_buf, &endptr, 10);
                    
                    if (val >= 1 && val <= 60000) {
                        sleep_duration = (int)val;
                        printf("FFB> Sleep: %d\n", sleep_duration);
                        usleep(sleep_duration * 1000); // Convert ms to us
                    } else {
                        fprintf(stderr, "Error: Sleep duration must be 1-60000 milliseconds.\n");
                    }
                } else {
                    fprintf(stderr, "Error: Number too long in sleep().\n");
                }
            } else {
                fprintf(stderr, "Error: Missing closing ')' in sleep command.\n");
            }
        }
    } 
}

void runInteractiveMode() {
    char input[65536];
    unsigned char packet[4];
    bool previousLineWasEmpty=false;

    initCOMSegaFFBController();

    printf("=== Interactive Mode ===\n");
    printf("Enter 8 hex digits (e.g., 80112233), or 'quit' to exit.\n");

    while (running) {
        printf("\nFFB> ");
        fflush(stdout);

        while (fgets(input, sizeof(input), stdin) != NULL) {
             if (errno == EINTR)
                 break; // Spurious interrupt; retry

            if (feof(stdin))
                break;

            // Trim newline
            size_t len = strlen(input);
            while (len > 0 && (input[len - 1] == '\n' || input[len - 1] == '\r')) {
                input[--len] = '\0';
            }

            // Skip empty lines
            if (len == 0) 
                break;

            // Check for exit commands
            if (strcasecmp(input, "quit") == 0 || strcasecmp(input, "exit") == 0) {
                printf("Exiting interactive mode.\n");
                break;
            }

            // Check for sleep command
            if (strncasecmp(input, "sleep", 5) == 0) {
                SleepFromInput(input);
                continue;
            }

            // Parse and validate hex input
            if (parse_hex_string(input, packet) != 0) {
                fprintf(stderr, "Error: Input must be exactly 8 valid hex characters (e.g., 80112233).\n");
                break;
            }

            // Load and send packet
            replyPacket[0] = packet[0];
            replyPacket[1] = packet[1];
            replyPacket[2] = packet[2];
            replyPacket[3] = packet[3];

            if(WriteReplyPacket() != FFB_STATUS_SUCCESS) {
                fprintf(stderr, "Error: Failed to send packet to Sega FFB Controller.\n");
                break;
            } 
            else {    
                printf("FFB> Sent: %02X %02X %02X %02X\n", packet[0], packet[1], packet[2], packet[3]);
            }

        }
    }
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
    char* token = NULL;

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
    else if ((strcmp(argv[1], "--interactiveSegaMIDICommand") == 0)    || (strcmp(argv[1], "-is") == 0)) {
        runInteractiveMode();
        return FFB_CLI_STATUS_SUCCESS_CLOSE;
    }
    else if ((strcmp(argv[1], "--4BytesSegaMIDICommand") == 0)  || (strcmp(argv[1], "-4s") == 0)) {

        token=strtok(NULL, "=");
        
        //(Spring, Friction, ConstantTorqueDirection, ConstantTorquePower)
        unsigned char AsciiHexToBin[4]={ahex2bin(token[0],token[1]),  // D0 => MIDI_CMD
                                        ahex2bin(token[2],token[3]),  // D1 => Value1
                                        ahex2bin(token[4],token[5]),  // D2 => Value2
                                        ahex2bin(token[6],token[7])}; // D3 => CRC

                    
        replyPacket[0] = AsciiHexToBin[0];
        replyPacket[1] = AsciiHexToBin[1];
        replyPacket[2] = AsciiHexToBin[2];
        replyPacket[3] = AsciiHexToBin[3];
        
        initCOMSegaFFBController();
        WriteReplyPacket();

        return FFB_CLI_STATUS_SUCCESS_CLOSE;
    }


    // Store all other requests for a specific hapic here:
    int cpKeyValue=0;
    for (int optind = 1; optind < argc ; optind++) {
        /*  --- Parameters with token --- */
        char *command = strtok(argv[optind], "=:");

        if(command!=NULL){
           
            if ((strcmp(command, "--haptic") == 0)                        || (strcmp(command, "-h") == 0)) {
                strcpy(arguments.haptic_name, strtok(NULL, "="));
            }              
            else if ((strcmp(command, "--gameProfile") == 0)              || (strcmp(command, "-gp") == 0)) {
                strcpy(arguments.game_profile, strtok(NULL, "="));
            }
            else if ((strcmp(command, "--supportedEffects") == 0)         || (strcmp(command, "-s") == 0)) {
                arguments.keyvalue[cpKeyValue].mode=GET_SUPPORTED_EFFECTS;
                cpKeyValue++;
            }
            else if ((strcmp(command, "--triggerEffect") == 0)            || (strcmp(command, "-t") == 0)) {
                arguments.keyvalue[cpKeyValue].mode=TRIGGER_EFFECT;
                strcpy(arguments.keyvalue[cpKeyValue].value,strtok(NULL, "="));
                cpKeyValue++;
            }
            else if ((strcmp(command, "--force") == 0)                    || (strcmp(command, "-f") == 0)) {
                arguments.keyvalue[cpKeyValue].mode=SET_FORCE;
                strcpy(arguments.keyvalue[cpKeyValue].value,strtok(NULL, "="));
                cpKeyValue++;                                    
            }
            else if ((strcmp(command, "--loadFile") == 0)                 || (strcmp(command, "-lf") == 0)) {
                arguments.keyvalue[cpKeyValue].mode=LOAD_RAW_EFFECT_FILE;
                strcpy(arguments.keyvalue[cpKeyValue].value,strtok(NULL, "="));
                cpKeyValue++;                                    
            }            
            else if ((strcmp(command, "--dumpRaw") == 0)                  || (strcmp(command, "-dr") == 0)) {
                arguments.keyvalue[cpKeyValue].mode=DUMP_RAW_SEGA_FFB_CONTROLLER;
                strcpy(arguments.keyvalue[cpKeyValue].value,strtok(NULL, "="));
                cpKeyValue++;                                    
            }
            // Start byte and CRC are not passed in parameter here, it is added in code
            else if ((strcmp(command, "--4BytesSegaFFBRawRequest") == 0)  || (strcmp(command, "-4") == 0)) {
                arguments.keyvalue[cpKeyValue].mode=TRIGGER_SEGA_FFB_RAW_REQUEST;
                token=strtok(NULL, "=");
                
                //(Spring, Friction, ConstantTorqueDirection, ConstantTorquePower)
                unsigned char AsciiHexToBin[4]={ahex2bin(token[0],token[1]),  // D0 => Spring
                                                ahex2bin(token[2],token[3]),  // D1 => Friction
                                                ahex2bin(token[4],token[5]),  // D2 => Constant Torque Direction
                                                ahex2bin(token[6],token[7])}; // D3 => Constant Torque Power

                sprintf(arguments.keyvalue[cpKeyValue].value, "%c%c%c%c%c%c",
                    OPENFFB_SET_BULK_EFFECTS_CMD,
                    AsciiHexToBin[0], 
                    AsciiHexToBin[1],
                    AsciiHexToBin[2],
                    AsciiHexToBin[3],
                    (AsciiHexToBin[0]^AsciiHexToBin[1]^AsciiHexToBin[2]^AsciiHexToBin[3])&0x7F);
                
                cpKeyValue++;
            }
            //Start byte is passed but not the CRC here, it is added in code
            else if ((strcmp(command, "--7BytesSegaFFBRawRequest") == 0)  || (strcmp(command, "-7") == 0)) {
                arguments.keyvalue[cpKeyValue].mode=TRIGGER_SEGA_FFB_RAW_REQUEST;
                token=strtok(NULL, "=");

                unsigned char AsciiHexToBin[7]={ahex2bin(token[0],token[1]),   // D0 => Start byte
                                                ahex2bin(token[2],token[3]),   // D1 => Spring
                                                ahex2bin(token[4],token[5]),   // D2 => Friction
                                                ahex2bin(token[6],token[7]),   // D3 => Constant Torque Direction
                                                ahex2bin(token[8],token[9]),   // D4 => Constant Torque Power
                                                ahex2bin(token[10],token[11]), // D5 => Sine Frequency
                                                ahex2bin(token[12],token[13])};// D6 => Sine Intensity
 
                sprintf(arguments.keyvalue[cpKeyValue].value, "%c%c%c%c%c%c%c%c",
                    AsciiHexToBin[0], 
                    AsciiHexToBin[1],
                    AsciiHexToBin[2],
                    AsciiHexToBin[3],
                    AsciiHexToBin[4],
                    AsciiHexToBin[5],
                    AsciiHexToBin[6],
                    (AsciiHexToBin[1]^AsciiHexToBin[2]^AsciiHexToBin[3]^AsciiHexToBin[4]^AsciiHexToBin[5]^AsciiHexToBin[6])&0x7F);

                cpKeyValue++;                
            }
            else if ((strcmp(command, "--8BytesSegaFFBRawRequest") == 0)  || (strcmp(command, "-8") == 0)) {
                arguments.keyvalue[cpKeyValue].mode=TRIGGER_SEGA_FFB_RAW_REQUEST;
                token=strtok(NULL, "=");
                
                sprintf(arguments.keyvalue[cpKeyValue].value, "%c%c%c%c%c%c%c%c",
                                                ahex2bin(token[0],token[1]),    // D0 => Start byte
                                                ahex2bin(token[2],token[3]),    // D1 => Spring
                                                ahex2bin(token[4],token[5]),    // D2 => Friction
                                                ahex2bin(token[6],token[7]),    // D3 => Constant Torque Direction
                                                ahex2bin(token[8],token[9]),    // D4 => Constant Torque Power
                                                ahex2bin(token[10],token[11]),  // D5 => Sine Frequency
                                                ahex2bin(token[12],token[13]),  // D6 => Sine Intensity
                                                ahex2bin(token[14],token[15])); // D7 => CRC
                                                
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

