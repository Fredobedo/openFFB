# openFFB
## Srerequisite
SDL2

## Installation and Running

sudo apt install git cmake
```
git clone https://github.com/Fredobedo/openFFB
cd OpenFFB
make
```
To run locally (from inside the root directory):
```
sudo ./build/openffb [PARAMETERS]
```

To install globally and run (from inside the root directory):
```
make install
sudo openffb [PARAMETERS]
```
To make OpenJVS run at startup you can use:
```
sudo systemctl enable openjvs
sudo systemctl start openjvs
```
## Test your racing wheel
First verify that your wheel is detected
```
openffb --availableHaptics
```
Then, test which FFB effects are supported
```
openffb --haptic=driving-force-gt --supportedEffects
```
Finally, you can test some effects
```
openffb --triggerSDLEffect=SDL_HAPTIC_SPRING
```

## Installation drivers for Logitech Racing wheels
Most of the Logitech Racing wheels are not supported in Linux (with the exception of the Logitech G29).
Fortunatelly, 'new-lg4ff' is a custom driver that you can install and which will support most of the FFB effects too.
More information at https://github.com/berarma/new-lg4ff.

