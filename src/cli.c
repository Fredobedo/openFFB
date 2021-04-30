#include <dirent.h>
#include <stdio.h>
#include <string.h>

#include "config.h"
#include "debug.h"
#include "cli.h"

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
    debug(0, "Usage: openffb game\n\n");
    debug(0, "Options:\n");
    debug(0, "  --help     Displays this text\n");
    debug(0, "  --version  Displays the OpenJVS Version\n");
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
FFBCLIStatus parseArguments(int argc, char **argv, char *game)
{
    // If there are no arguments simply continue
    if (argc != 1)
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
    // If the first argument doesn't start with a dash it must be a map file.
    else
    {
        strcpy(game, argv[1]);
        return FFB_CLI_STATUS_SUCCESS_CONTINUE;
    }


    // If none of these where found, the argument is unknown.
    //debug(0, "Unknown argument %s\n", argv[1]);
    //return JVS_CLI_STATUS_ERROR;
}
