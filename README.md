# openFFB
## Introduction

## Dependencies

## Installation and Running
Installation may be doen like this:
```
git clone https://github.com/Fredobedo/openFFB
cd openFFB
mkdir build
cd build 
cmake ..
make
cmake --install .
```
file structure after installation:

| Location                                           | Description                                  |
| -------------------------------------------------- | -------------------------------------------- |
| /usr/local/bin/openffb                             | Executable                                   |
| /etc/openffb/config                                | Main configuration file                      |
| /etc/openffb/games/generic-driving                 | Default FFB effect settings                  |
| /etc/openffb/games/generic-driving-no-ffb-support  | A game profile for games not supporting FFB  |
| /etc/openffb/games/initial-d                       | A specific game profile for a game           |

Please have a look at the help menu to have a first insight on OpenFFB:
![help](https://github.com/Fredobedo/openFFB/blob/master/docs/images/openffb_help.jpg?raw=true)

## Test your racing wheel
1. First verify that your wheel is detected:
![available](https://github.com/Fredobedo/openFFB/blob/master/docs/images/openffb_available.jpg?raw=true)

2. Then, test if your linux distribution is supporting FFB effects for your racing wheel: 
![supported](https://github.com/Fredobedo/openFFB/blob/master/docs/images/openffb_supported.jpg?raw=true)

3. Now, you can start OpenFFB:
![start](https://github.com/Fredobedo/openFFB/blob/master/docs/images/openffb_start.jpg?raw=true)

## Configuration
You can dump the actual OpenFFB like this:
![start](https://github.com/Fredobedo/openFFB/blob/master/docs/images/openffb_config.jpg?raw=true)

## Installation drivers for Logitech Racing wheels
Most of the Logitech Racing wheels are not supported in Linux (with the exception of the Logitech G29).
Fortunatelly, 'new-lg4ff' is a custom driver that you can install and which will support most of the FFB effects too.
More information at https://github.com/berarma/new-lg4ff.

## Integration with OpenJVS

## WiPi: Example of a successfull full integration

