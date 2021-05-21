#ifndef OPENFFB_H_
#define OPENFFB_H_

#include <stdbool.h>

typedef enum
{
    STATE_INIT_FINISHED,
    STATE_RUNNING,
}openffb_state;

int main(int argc, char **argv);
void handleSignal(int sig);

#endif // OPENFFB_H_
