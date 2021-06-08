#include <stdio.h>
#include <signal.h>
#include <string.h>
#include "cli.h"
#include "device.h"
#include "openffb.h"
#include "ffb.h"
#include "config.h"
#include "debug.h"

#include "ffbhelper.h"

int running = 1;

int main(int argc, char **argv)
{
  signal(SIGINT, handleSignal);
  FFBConfig *localConfig = getConfig(); 

  void initCOMSegaFFBController()
  {
    debug(1, "Connecting to Sega FFB Controller");
    while(!initFFB(localConfig->segaFFBControllerPath) && running)
    {
      debug(1, ".");
      fflush(stdout);
      sleep(1);
    }
    debug(1, "\n");
  }

 unsigned char ahex2bin(unsigned char MSB, unsigned char LSB) {  
    if (MSB > '9') MSB -= 7;          // Convert MSB value to a contiguous range (0x30..0x3F)  
    if (LSB > '9') LSB -= 7;          // Convert LSB value to a contiguous range (0x30..0x3F)  
     return (MSB <<4) | (LSB & 0x0F); // Make a result byte  using only low nibbles of MSB and LSB thus neglecting the input register case
 }  
  /* Read the initial config */
  if (parseConfig(CONFIG_PATH) != FFB_CONFIG_STATUS_SUCCESS)
    printf("Warning: No valid openffb config file found, a default is being used\n");

  /* Initialise the debug output */
  if (!initDebug(localConfig->debugLevel))
  {
    printf("Failed to initialise debug output\n");
  }

  /* Parsing arguments */
  FFBCLIStatus argumentsStatus = parseArguments(argc, argv);

  switch (argumentsStatus)
  {
  case FFB_CLI_STATUS_ERROR:
    return EXIT_FAILURE;
    break;
  case FFB_CLI_STATUS_SUCCESS_CLOSE:
    return EXIT_SUCCESS;
    break;
  }

  debug(0, "OpenFFB Version "); printVersion();
  debug(0, "Sega FFB Controller: %s\n", localConfig->segaFFBControllerPath);
  debug(1, "\nDebug messages will appear below, you are in debug mode %d.\n\n", localConfig->debugLevel);

  /* update config from arguments */
  strcpy(localConfig->hapticName, arguments.haptic_name);

  /* reading game profile settings */
  if(arguments.game_profile[0]!=0)
    strcpy(localConfig->gameProfile, arguments.game_profile);
  
  char gameProfilePathAndName[256];
  strcpy(gameProfilePathAndName, DRIVING_PROFILE_PATH);
  strcat(gameProfilePathAndName, getConfig()->gameProfile);

  if (parseDrivingProfile(gameProfilePathAndName) != FFB_CONFIG_STATUS_SUCCESS)
    printf("Warning: No valid game config file found, default FFB settings will be used\n");

  /* time to try to connect to racing wheel */
  while(!FFBInitHaptic(localConfig->hapticName) && running){
    debug(0, "Error, can not open device!\n"); 
    sleep(2);
  }

  if(containArgument(GET_SUPPORTED_EFFECTS)){
    FFBDumpSupportedFeatures();
    return EXIT_SUCCESS;
  }

  if(containArgument(TRIGGER_EFFECT)){
    double Strength = 0.85;
    if(containArgument(SET_FORCE))
      Strength=((double)atoi(getArgumentValue(SET_FORCE)))/100;

    FFBTriggerEffect(hapticEffectFromString(getArgumentValue(TRIGGER_EFFECT)), Strength);
    sleep(2);
    return EXIT_SUCCESS;
  }

  if(containArgument(TRIGGER_SEGA_FFB_RAW_REQUEST)){

    processPacket(getArgumentValue(TRIGGER_SEGA_FFB_RAW_REQUEST));
    sleep(20);
    return EXIT_SUCCESS;
  }

  if(containArgument(LOAD_RAW_EFFECT_FILE)){

    FILE *file = fopen(getArgumentValue(LOAD_RAW_EFFECT_FILE), "rb");
    if (file == NULL) {
        printf("Error: %s", strerror(errno));
        return EXIT_FAILURE;
    }
    else{
      int test_prb=0;
        unsigned char line[1024];
        while (fgets(line, sizeof(line), file)) {

          sprintf(line, "%c%c%c%c%c%c",
                  ahex2bin(line[0],line[1]),
                  ahex2bin(line[2],line[3]),
                  ahex2bin(line[4],line[5]),
                  ahex2bin(line[6],line[7]),
                  ahex2bin(line[8],line[9]),
                  ahex2bin(line[10],line[11]));
          
          processPacket(line);
          usleep(5000);
      }
      debug(0, "finished reading file!\n");  
      sleep(20);
      return EXIT_SUCCESS;
    }
  }

  /* Start COM itialization with Sega FFB controller */
  initCOMSegaFFBController();

  /* Process packets forever */
  debug(2, "Will start the main loop...\n");
  int current_state=0;
  FFBStatus processingStatus;
  int FFBStatusMinus      = FFB_STATUS_ERROR_TIMEOUT;
  int FFBStatusMinusMinus = FFB_STATUS_ERROR_TIMEOUT;
  
  int MAX_TIMOUT_COUNT = 10;
  int nbrOfTimeOut=0;

  int MAX_SUCCESS = 3;
  int nbrOfSuccess=0;

  while (running)
  {
    if(containArgument(DUMP_RAW_SEGA_FFB_CONTROLLER))
      processingStatus = readDebugPacket(atoi(getArgumentValue(DUMP_RAW_SEGA_FFB_CONTROLLER))); 
    else
      processingStatus = readPacket();

    switch (processingStatus)
    {
      case FFB_STATUS_ERROR_SYNCH_REQUIRED:
        tryResynch();
        break;
      case FFB_STATUS_ERROR_CHECKSUM:
        debug(1, "Error: checksum error occoured\n");
        break;
      case FFB_STATUS_ERROR:
      case FFB_STATUS_ERROR_TIMEOUT:
        debug(1, "Error: timout occoured, nbrOfTimeOut=%d\n", nbrOfTimeOut);      
        nbrOfSuccess=0;
        if (nbrOfTimeOut==MAX_TIMOUT_COUNT){
            playCOMEndEffect();
            closeDevice();
            initCOMSegaFFBController();
            nbrOfTimeOut=0;
            current_state=STATE_INIT_FINISHED;
        }
        else
            nbrOfTimeOut++;

        break;
      case FFB_STATUS_SUCCESS:
        nbrOfTimeOut=0;
        if(current_state==STATE_INIT_FINISHED) {
            if(nbrOfSuccess==MAX_SUCCESS){
              playCOMInitEffect();
              nbrOfSuccess=0;
              current_state=STATE_RUNNING;
            }
            else{
              nbrOfSuccess++;
            }
        }
        break;
      default:
        //debug(1, "default processingStatus=%d\n", processingStatus);      
        break;
    }
    FFBStatusMinusMinus = FFBStatusMinus;
    FFBStatusMinus      = processingStatus;
  }

  /* Close the file pointer */
  if (!disconnectFFB())
  {
    debug(2, "Error: Could not disconnect from serial\n");
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

void handleSignal(int signal)
{
  if (signal == 2)
  {
    debug(2, "\nClosing down OpenFFB...\n");
    disconnectFFB();
    running = 0;
  }
}
