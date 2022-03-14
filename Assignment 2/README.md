# CSc 360: Programming Assignment 2

`mts.c` is a program that simulates an automated control system for a railway track. 
This program emualtes the scheduling of multiple threads sharing a common resource in an Operating System

## Design
1. The input file is read line by line. Each line of the file corresponds to a single train.
2. The train information is store into an array of structs.
3. A timer is used to keep track of the timings of the train.
4. For each train a thread is created.
5. When a train is created it begins loading.
7. Once the train finishes loading it is compared with the train waiting to use the track to see if it's priority is higher or lower. The simulation rules in the next section are used.
8. When the next highest priority train is picked and the track mutex is unlocked; the train can lock the track mutex and cross.
9. At then end of the program, the train threads are joined and the condition and mutex variables are destroyed.

### Simulation Rules
In order to ensure that each train crosses the track at an approriate time, the following rules are enforce in order:
1. Ensure the train has not already crossed.
2. Ensure a train exists.
3. If four trains in the same direction have travelled back-to-back on the track, let a train waiting in the opposite direction cross.
4. Check if a train has high priority (e.g. `E` or `W`). **NOTE**: This rule has been implemented but is no longer required for this assignment as of 12/03/2022.
5. If two compared trains are travelling in the same direction, the train which finished loading first crosses. If their loading times are the same the train that appears first in the input file crosses the track.

### Output
When the train has loaded `00:00:00.0 Train <number> is ready to go <East/West>`

When the train is on the track `00:00:00.0 Train <number> is <ON/OFF> the main track going <EAST/WEST>`

## Code Structure
Create array of Structs

main {<br />
extractData() store the contents of the file for each individual train<br />
create() for each train create a:<br />
- Thread
- Condition variable
- mutex
joinThreads() join the train threads once finished<br />
destroyTrains() destroy condition and mutex variables<br />
}

Thread {

}

## Running the program
The program should be runned by using the makefile. The program takes a `.txt` input file e.g.

**Input**<br />
e 10 655<br />
W 6 756<br />
E 3 10

**Output**<br />
00:00:00.3 Train 2 is ready to go East<br />
00:00:00.3 Train 2 is ON the main track going East<br />
00:00:00.6 Train 1 is ready to go West<br />
00:00:01.0 Train 0 is ready to go East<br />
00:00:01.3 Train 2 is OFF the main track after going East<br />
00:00:01.3 Train 1 is ON the main track going West<br />
00:00:02.0 Train 1 is OFF the main track after going West<br />
00:00:02.0 Train 0 is ON the main track going East<br />
00:00:02.6 Train 0 is OFF the main track after going East<br />
