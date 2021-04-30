#ifndef FFB_H_
#define FFB_H_


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <time.h>
#include <stdarg.h>
#include <sys/ioctl.h>


#define SYNC 0x80

/* Status for the entire packet */
#define STATUS_SUCCESS 0x01
#define STATUS_UNSUPPORTED 0x02      // an unsupported command was sent
#define STATUS_CHECKSUM_FAILURE 0x03 // the checksum on the command packet did not match a computed checksum
#define STATUS_OVERFLOW 0x04         // an overflow occurred while processing the command

typedef struct
{
    unsigned char spring;               // from 0x00 to 0x7F -> 128 levels
    unsigned char friction;             // from 0x00 to 0x7F -> 128 levels
    unsigned char torqueDirection;      // 0x00 = Left, 0x01  = Right
    unsigned char torquePower;          // from 0x00 to 0xFF -> 128 levels
} FFBPacket;

typedef enum
{
    FFB_STATUS_SUCCESS,
    FFB_STATUS_ERROR,
    FFB_STATUS_ERROR_TIMEOUT,
    FFB_STATUS_ERROR_CHECKSUM,
    FFB_STATUS_ERROR_UNSUPPORTED_COMMAND,
} FFBStatus;

int initFFB(char *devicePath);
int disconnectFFB();

FFBStatus processPacket();

FFBStatus readPacket(FFBPacket *packet);

#endif //FFB_H_
