#ifndef CLI_H_
#define CLI_H_

typedef enum
{
    FFB_CLI_STATUS_ERROR = 0,
    FFB_CLI_STATUS_SUCCESS_CLOSE,
    FFB_CLI_STATUS_SUCCESS_CONTINUE,
} FFBCLIStatus;

FFBCLIStatus parseArguments(int argc, char **argv, char *game);
FFBCLIStatus printVersion();

#endif // CLI_H_
