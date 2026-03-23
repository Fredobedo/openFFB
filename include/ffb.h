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
#include <stdint.h>


//#define SYNC 0x80

/* Status for the entire packet */
#define STATUS_SUCCESS          0x01
#define STATUS_UNSUPPORTED      0x02 // an unsupported command was sent
#define STATUS_CHECKSUM_FAILURE 0x03 // the checksum on the command packet did not match a computed checksum
#define STATUS_OVERFLOW         0x04 // an overflow occurred while processing the command

// 2 type of Commands can be sent to OPENFFB (first byte sent), 8 bytes request
#define OPENFFB_SET_BULK_EFFECTS_CMD  0x80  // => the one implemented by Aganyte
#define OPENFFB_GENERIC_CMD           0x99  // => extensions specific for openffb integration

// OPENFFB REQUESTS
#define OPENFFB_GET_WHEEL_POSITION_SUB_CMD     0x01
#define OPENFFB_GET_POWER_LINE_SUB_CMD         0x02 // not used for Sega but Namco
#define OPENFFB_SET_CENTER_SUB_CMD             0x10 // used during initialization
#define OPENFFB_SET_MAX_RIGHT_SUB_CMD          0x11 // used during start of calibration, at init
#define OPENFFB_SET_MAX_LEFT_SUB_CMD           0x12 // Not used by now
#define OPENFFB_SET_FRICTION_SUB_CMD           0x13 // No reply for this request
#define OPENFFB_NOT_READY_SUB_CMD              0x6E // No reply for this request
#define OPENFFB_RESET_DEVICE_SUB_CMD           0x6F // No reply for this request

// OPENFFB REPLIES
#define OPENFFB_WHEEL_POSITION_REPLY_CMD       0x90  
#define OPENFFB_POWER_LINE_REPLY_CMD           0x91
#define OPENFFB_CENTER_REPLY_CMD               0x92
#define OPENFFB_MAX_RIGHT_REPLY_CMD            0x93 
#define OPENFFB_MAX_LEFT_REPLY_CMD             0x94

// #define GET_WHEEL_POSITION 0x01
// #define GET_POWER_LINE     0x02

// #define SET_CENTER    0xA0
// #define SET_MAX_RIGHT 0xA1  
// #define SET_MAX_LEFT  0xA2
// #define NOT_READY     0xFE
// #define RESET_DEVICE  0xFF

#define SEGA_FFB_CONTROLLER_PACKET_SIZE       8
#define SEGA_FFB_CONTROLLER_REPLY_PACKET_SIZE 4

typedef struct
{
    uint8_t startByte;            // Start byte
    double spring;                // from 0x00 to 0x7F -> 128 levels converted to double 0.0 -> 1.0
    double friction;              // from 0x00 to 0x7F -> 128 levels converted to double 0.0 -> 1.0 
    int torqueDirection;          // 0x00 = Left, 0x01  = Right
    double torquePower;           // from 0x00 to 0x7F -> 128 levels converted to double 0.0 -> 1.0
    //double torque;
    double sineFrequency;         // from 0x00 to 0x7F -> 128 levels converted to double 0.0 -> 1.0
    double sineIntensity;         // from 0x00 to 0x7F -> 128 levels converted to double 0.0 -> 1.0
   // uint8_t crc;                  // (D1 ^ D2 ^ D3 ^ D4) & 0x7F
} FFBPacket;

unsigned char rawPacket[SEGA_FFB_CONTROLLER_PACKET_SIZE];
unsigned char replyPacket[SEGA_FFB_CONTROLLER_REPLY_PACKET_SIZE];
unsigned char previous_rawpacket[SEGA_FFB_CONTROLLER_PACKET_SIZE];
unsigned char max_rawpacket[SEGA_FFB_CONTROLLER_PACKET_SIZE];

typedef enum
{
    FFB_STATUS_SUCCESS,
    FFB_STATUS_ERROR,
    FFB_STATUS_ERROR_TIMEOUT,
    FFB_STATUS_ERROR_CHECKSUM,
    FFB_STATUS_ERROR_UNSUPPORTED_COMMAND,
    FFB_STATUS_ERROR_SYNCH_REQUIRED,
} FFBStatus;

/* --- init haptic device  --- */
int initFFB(char *devicePath);
int disconnectFFB();

FFBStatus tryResynch();

/* --- for debugging purpose only --- */
FFBStatus readDebugPacket(int amount);

/* --- read usb2 serial communication from Sega FFB Controller --- */
FFBStatus readPacket();

FFBStatus WriteReplyPacket();

/* --- dispatch Sega FFB Controller request --- */
FFBStatus processPacket(unsigned char* packet);
/* --- openFFB execute an initial effect when communication with Sega FFB controller is up and running --- */
void playCOMInitEffect();
/* --- openFFB execute an end effect when communication with Sega FFB controller is finished --- */
void playCOMEndEffect();


#endif //FFB_H_
