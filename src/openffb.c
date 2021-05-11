#include <stdio.h>
#include <signal.h>
#include <string.h>
#include "cli.h"
#include "openffb.h"
#include "ffb.h"
#include "config.h"
#include "debug.h"

#include "sdlhelper.h"

void handleSignal(int signal);

int running = 1;

int main(int argc, char **argv)
{
  signal(SIGINT, handleSignal);

  /* Read the initial config */
  if (parseConfig(DEFAULT_CONFIG_PATH) != FFB_CONFIG_STATUS_SUCCESS)
    printf("Warning: No valid openffb config file found, a default is being used\n");

  /* reading game profile settings */
  if (parseConfig(DEFAULT_GAME_SETTING_PATH) != FFB_CONFIG_STATUS_SUCCESS)
    printf("Warning: No valid game config file found, default FFB settings will be used\n");

  FFBConfig *localConfig = getConfig();


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
  debug(0, "\nDebug messages will appear below, you are in debug mode %d.\n\n", localConfig->debugLevel);

  strcpy(localConfig->hapticName, arguments.haptic_name);

  /* dispatch to requested function */
  if(!initHaptic(localConfig->hapticName))
    return EXIT_FAILURE;

  if(containArgument(GET_SUPPORTED_EFFECTS))
    dumpSupportedFeatures();

  if(containArgument(TRIGGER_SDL_EFFECT)){
    double SDLStrength = 0.5;
    if(containArgument(SET_FORCE))
      SDLStrength=((double)atoi(getArgumentValue(TRIGGER_SDL_EFFECT)))/100;
    
    TriggerEffect(hapticEffectFromString(getArgumentValue(TRIGGER_SDL_EFFECT)), SDLStrength);
  }

  if(containArgument(TRIGGER_SEGA_FFB_RAW_REQUEST))
    processPacket(getArgumentValue(TRIGGER_SEGA_FFB_RAW_REQUEST));

return 0;



  /* Process packets forever */
  FFBStatus processingStatus;
  while (running)
  {
    processingStatus = readPacket();
    switch (processingStatus)
    {
      case FFB_STATUS_ERROR_CHECKSUM:
        debug(0, "Error: A checksum error occoured\n");
        break;
      case FFB_STATUS_ERROR_TIMEOUT:
        break;
      default:
        break;
      }
  }

  /* Close the file pointer */
  if (!disconnectFFB())
  {
    debug(0, "Error: Could not disconnect from serial\n");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

void handleSignal(int signal)
{
  if (signal == 2)
  {
    debug(0, "\nClosing down OpenFFB...\n");
    running = 0;
  }
}
