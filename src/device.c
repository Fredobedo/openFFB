#include "device.h"
#include "debug.h"

#define TIMEOUT_SELECT 200

int serialIO = -1;

int setSerialAttributes(int fd, int myBaud);

int initDevice(char *devicePath)
{
  if ((serialIO = open(devicePath, O_RDWR | O_NOCTTY | O_SYNC | O_NDELAY)) < 0)
  {
    return 0;
  }
   
  /* Setup the serial connection */
  setSerialAttributes(serialIO, B115200);

  return 1;
}

int closeDevice()
{
  if(serialIO>0){
    tcflush(serialIO, TCIOFLUSH);

    struct timeval tv;

    tv.tv_sec = 0;
    tv.tv_usec = TIMEOUT_SELECT * 1000;

    select(serialIO + 1, NULL, NULL, NULL, &tv);
    return close(serialIO) == 0;
  }
  else
    return 0;
}

int readBytes(unsigned char *buffer, int amount)
{
  fd_set fd_serial;
  struct timeval tv;
//debug(0,"1\n");
  FD_ZERO(&fd_serial);
//debug(0,"2\n");  
  FD_SET(serialIO, &fd_serial);

  tv.tv_sec = 0;
  tv.tv_usec = TIMEOUT_SELECT * 1000;
//debug(0,"3\n");
  int filesReadyToRead = select(serialIO + 1, &fd_serial, NULL, NULL, &tv);
//debug(0,"4\n");
  if (filesReadyToRead < 1)
    return -1;

  if (!FD_ISSET(serialIO, &fd_serial))
    return -2;

//debug(0,"5\n");
  return read(serialIO, buffer, amount);
}


/* Sets the configuration of the serial port */
int setSerialAttributes(int fd, int myBaud)
{
  struct termios options;
  int status;
  tcgetattr(fd, &options);

  cfmakeraw(&options);
  cfsetispeed(&options, myBaud);
  cfsetospeed(&options, myBaud);

  options.c_cflag |= (CLOCAL | CREAD);
  options.c_cflag &= ~PARENB;
  options.c_cflag &= ~CSTOPB;
  options.c_cflag &= ~CSIZE;
  options.c_cflag |= CS8;
  options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
  options.c_oflag &= ~OPOST;

  options.c_cc[VMIN] = 0;
  options.c_cc[VTIME] = 0; // One seconds (10 deciseconds)

  tcsetattr(fd, TCSANOW, &options);

  ioctl(fd, TIOCMGET, &status);

  status |= TIOCM_DTR;
  status |= TIOCM_RTS;

  ioctl(fd, TIOCMSET, &status);

  usleep(100 * 1000); // 10mS

  struct serial_struct serial_settings;

  ioctl(fd, TIOCGSERIAL, &serial_settings);
  serial_settings.flags |= ASYNC_LOW_LATENCY;
  ioctl(fd, TIOCSSERIAL, &serial_settings);

  tcflush(serialIO, TCIOFLUSH);
  usleep(100 * 1000); // Required to make flush work, for some reason

  return 0;
}



