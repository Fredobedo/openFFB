# openFFB
## Installation and Running

sudo apt install git cmake
```
git clone https://github.com/Fredobedo/openFFB
cd OpenFFB
make
```
To run locally (from inside the root directory):
sudo ./build/openffb [PARAMETERS]


To install globally and run (from inside the root directory):
make install
sudo openffb [PARAMETERS]

To make OpenJVS run at startup you can use:
sudo systemctl enable openjvs
sudo systemctl start openjvs