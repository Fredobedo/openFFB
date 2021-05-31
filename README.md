# openFFB
## Introduction
The goal of this project is to use consumer FFB Racing wheels on Arcade racing cabs/systems.  
The user can take an USB Racing Wheel which support Force Feedback effects (i.e. Logitech GT), connect it to a Single Board Computer (i.e. a raspberry PI) and enjoy FFB effects with arcade games.  
  
This can be achieved thanks to an interface between the arcade System and the SBC, called "Sega FFB Controller" that you can get from Aganythe. (c/o https://www.gamoover.net/Forums/index.php?topic=42310.0)  
  
Please note that this Sega FFB Controller may achieve far more possibilities depending on the firmware we inject but this will not be covered here (i.e. act as an interface between a PC and a racing cab).    
  
![available](https://github.com/Fredobedo/openFFB/blob/master/docs/images/openffb_1.jpg?raw=true)  

But wait, arcade Systems like Naomi, Chihiro, Triforce, etc. require a connection to a JVS IO Board (arcade controller interface). 
This is required here in our Use Case, by the System to retrieve Wheel's position, pedals, etc.  
  
For this pupose, we use "OpenJVS", a Software JVS IO Board, that also can run on a Raspberry PI.  
  
![available](https://github.com/Fredobedo/openFFB/blob/master/docs/images/openffb_2.jpg?raw=true)  

But wait, how can we use it?  
"Wipi", a Web Application Interface has also been created for the best End User experience, helping users in configuring and using "OpenJVS", "OpenFFB" and has NetDimm support too.  
  
"WiPi" can be found as a full image, ready to be used, with all pre-configured, please read below paragraph for more information.  
  
![available](https://github.com/Fredobedo/openFFB/blob/master/docs/images/openffb_3.jpg?raw=true)  

## Dependencies
- You will need a Sega FFB Controller.  
- You will need a SBC with at least 2 USB 2.0 ports (one for the racing wheel and the second for the connection with the Sega FFB Controller).  
- You will need a 32GB MicroSD if you plan to use the "Ready to Go WiPi" image for Raspberry PI.
     
## Installation and Running
THE FOLLOWING INFORMATION IS ONLY USEFULL FOR PERSONS WILLING TO INTEGRATE OPENFFB MANUALLY IN THEIR SOLUTION/BUILD.  
PEOPLE MAY USE THE "READY TO GO WIPI" INSTEAD, WHICH OFFERS ALL NECESSARY WEB INTERFACE TO USE AND MANAGE IT.  
  
  
Installation may be done like this:
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

## External references
- Sega FFB Controller: https://www.gamoover.net/Forums/index.php?topic=42310.0  
- OpenJVS: https://github.com/OpenJVS/OpenJVS  
- WiPi: TBD  
- new-lg4ff:  https://github.com/berarma/new-lg4ff

## Disclaimer
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.