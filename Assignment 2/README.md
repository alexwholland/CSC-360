# CSc 360: Programming Assignment 2

`mts.c` is a program that simulates an automated control system for a railway track. 
This program emualtes the scheduling of multiple threads sharing a common resource in an Operating System

## Design
1. The input file is read line by line. Each line of the file corresponds to a single train.
2. The train information is store into an array of structs.
3. A timer is used to keep track of the timings of the train.
4. A thread is created for each train in the file.
5. 

### Simulation Rules
In order to ensure that each train crosses the track at an approriate time, the following rules are enforce in order:
1. Ensure the train has not already crossed.
2. Ensure a train exists.
3. If four trains in the same direction have travelled back-to-back on the track, let a train waiting in the opposite direction cross.
4. Check if a train has high priority (e.g. `E` or `W`). **NOTE**: This rule has been implemented but is no longer required for this assignment as of 12/03/2022.
5. If two compared trains are travelling in the same direction, the train which finished loading first crosses. If their loading times are the same the train that appears first in the input file crosses the track.

## Code Structure
Create array of Structs

main {
extractData() store the contents of the file for each individual train

create() for each train create a:
- Thread
- Condition variable 
- mutex
joinThreads() join the train threads once finished
destroyTrains() destroy condition and mutex variables
}

Thread {

}

## Running the program
The program should be runned by using the makefile.
