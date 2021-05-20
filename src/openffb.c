#include <stdio.h>
#include <signal.h>
#include <string.h>
#include "cli.h"
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

  /* Read the initial config */
  if (parseConfig(CONFIG_PATH) != FFB_CONFIG_STATUS_SUCCESS)
    printf("Warning: No valid openffb config file found, a default is being used\n");

  FFBConfig *localConfig = getConfig();

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

  if(!FFBInitHaptic(localConfig->hapticName))
    return EXIT_FAILURE;

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

  if (!initFFB(localConfig->segaFFBControllerPath))
  {
    debug(0, "Error: Could not initialize communication with Sega FFB Controller (Serial over USB)\n");
    return EXIT_FAILURE;
  }

  debug(2, "Will start the main loop...\n");

  /* Process packets forever */
  FFBStatus processingStatus;
  while (running)
  {
    processingStatus = readPacket();
    switch (processingStatus)
    {
      case FFB_STATUS_ERROR_CHECKSUM:
        debug(2, "Error: A checksum error occoured\n");
        break;
      case FFB_STATUS_ERROR_TIMEOUT:
              debug(2,".");
              fflush(stdout);
        break;
      default:
        break;
      }
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
    running = 0;
  }
}
