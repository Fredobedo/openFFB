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

//void handleSignal(int signal);

int running = 1;

int main(int argc, char **argv)
{
  signal(SIGINT, handleSignal);
  FFBConfig *localConfig = getConfig(); 

  void initCOM()
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

  /* Read the initial config */
  if (parseConfig(CONFIG_PATH) != FFB_CONFIG_STATUS_SUCCESS)
    printf("Warning: No valid openffb config file found, a default is being used\n");

  /* reading game profile settings */
  char racingProfilePathAndName[256];
  strcpy(racingProfilePathAndName, DRIVING_PROFILE_PATH);
  strcat(racingProfilePathAndName, getConfig()->drivingProfile);

  if (parseDrivingProfile(racingProfilePathAndName) != FFB_CONFIG_STATUS_SUCCESS)
    printf("Warning: No valid game config file found, default FFB settings will be used\n");

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
  debug(0, "  Sega FFB Controller:\t\t%s\n", localConfig->segaFFBControllerPath);
  debug(1, "\nDebug messages will appear below, you are in debug mode %d.\n\n", localConfig->debugLevel);

  strcpy(localConfig->hapticName, arguments.haptic_name);

  while(!FFBInitHaptic(localConfig->hapticName) && running){
    debug(0, "Error, can not open device!\n"); 
    sleep(2);
  }

  if(containArgument(GET_SUPPORTED_EFFECTS)){
    FFBDumpSupportedFeatures();
        sleep(5);
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

  initCOM();


  debug(2, "Will start the main loop...\n");

  /* Process packets forever */
  int current_state=0;
  FFBStatus processingStatus;
  int FFBStatusMinus      = FFB_STATUS_ERROR_TIMEOUT;
  int FFBStatusMinusMinus = FFB_STATUS_ERROR_TIMEOUT;
  
  int MAX_TIMOUT_COUNT = 4;
  int nbrOfTimeOut=0;

  int MAX_SUCCESS = 3;
  int nbrOfSuccess=0;

  while (running)
  {
    processingStatus = readPacket();
    switch (processingStatus)
    {
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
            initCOM();
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
