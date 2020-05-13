# Limitedness USing INfinite Games
This program is not a robust tool; it is only used to test the approach of solving the limitedness problem using infinite games (see the [paper](https://arxiv.org/abs/1708.03603) by Mikolaj Bojanczyk).
This program uses [source code](https://github.com/nathanael-fijalkow/stamina/) of the tool [STAMINA](https://stamina.labri.fr/), which can also solve limitedness problems.
It turns out that the implemented approach using games(at least in its current form) can not compete with STAMINA.

### Installation
Create a subdirectory of the directory this file is in and then type:

cmake ..  
make  
./games  

Further installation of [SPOT](https://spot.lrde.epita.fr/) and [OINK](https://github.com/trolando/oink) is required. The program simply calls the commands *autfilt* and *oink* of SPOT/OINK, to check correct installation open a terminal and type:  

autfilt --version  
oink --help  

This program was only tested on Linux.

### Usage
Even for small cost automata(for example 2 or 3 states, 2 letters, 2 counters) the limitedness computation may take a long time or fill the memory completely. Killing this program will **not** automatically kill 'autfilt' if it is currently running. You should keep track of the memory used by 'games'/'autfilt' and **save important data before usage of this program**. This especially applies to the computation of star heights.  

The program needs to write some auxiliary files and may write optional files. The directory(it must already exist) can be set using '-a=*path*'. Always specify paths using the file separator '/'.  

Some basic console commands:  
h			- prints more detailed usage instructions  
q			- quit the program  

do *path*		- executes the instructions in the file at the given path  
lim *path*		- decides whether the cost automaton in the given file is limited  
fp *path*		- decides whether the NFA in the given file has the finite power property  
sh *path*		- computes the star height of the DFA in the given file  

-v			- print verbose output to console  
-c			- compare results to STAMINA  
-s			- print some statistics after commands terminate  
-g=*mode*		- change between different implementations of the limitedness game  

The reading of input automata and their format is due to STAMINA.  
Some files containing commands can be found in './examples/scripts/'.  

### Source Code
games.h/cpp		- contains reductions from limitedness to infinite games  
condition.h/cpp		- contains some auxiliary data structures for the reductions  
parity.h/cpp		- contains data structures for parity automata/games  
wrapper.h/cpp		- interface to other tools  
tests.h/cpp		- some functions for testing  
All other files in './games/' are used for IO/debugging.  
Used STAMINA source code is contained in './stamina/'. See the file './changed\_to\_stamina'.  


