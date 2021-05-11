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
  {
    printf("Warning: No valid config file found, a default is being used\n");
  }
  FFBConfig *localConfig = getConfig();



  /* Initialise the debug output */
  if (!initDebug(localConfig->debugLevel))
  {
    printf("Failed to initialise debug output\n");
  }

  /* Parsing arguments */
  char triggerSegaRawRequest[6];
  unsigned int triggerSDLeffect=0;
  double SDLStength = 0.5;
  int dumpSupportedEffects=0;

  FFBCLIStatus argumentsStatus = parseArguments(argc, argv, 
                                                localConfig->hapticName,
                                                &dumpSupportedEffects, 
                                                &triggerSDLeffect, 
                                                &SDLStength, 
                                                triggerSegaRawRequest);
  switch (argumentsStatus)
  {
  case FFB_CLI_STATUS_ERROR:
    return EXIT_FAILURE;
    break;
  case FFB_CLI_STATUS_SUCCESS_CLOSE:
    return EXIT_SUCCESS;
    break;
  case FFB_CLI_STATUS_SUCCESS_CONTINUE:
    break;
  default:
    break;
  }

  debug(0, "OpenFFB Version "); printVersion();

  /* dispatch to requested function */
  if(initHaptic(localConfig->hapticName))
  {
    if(dumpSupportedEffects){
      debug(0, "D3\n");
      dumpSupportedFeatures();
    }
    else
    {
      debug(0, "D4\n");
    }

    /* SDL Effect requested */
    if(triggerSDLeffect)
      TriggerEffect(triggerSDLeffect, SDLStength);

    /* Effect based on raw request */  
    else if(strlen(triggerSegaRawRequest)==6)
      processPacket(triggerSegaRawRequest);
      
  }
return 0;

/*
  if (initInputs(localConfig->defaultGamePath))
  {
    debug(0, "Error: Could not initialise the inputs - make sure you are root\n");
    debug(0, "Try running `sudo openjvs --list` to see the devices\n");
    return EXIT_FAILURE;
  }

  */
  debug(0, "  Sega FFB Controller:\t\t%s\n", localConfig->segaFFBControllerPath);
  debug(0, "\nDebug messages will appear below, you are in debug mode %d.\n\n", localConfig->debugLevel);

  /* Setup the Aganyte's Sega FFB Controller with Serial over USB connection */
  if (!initFFB(localConfig->segaFFBControllerPath))
  {
    debug(0, "Error: Could not initialise FFB\n");
    return EXIT_FAILURE;
  }




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
