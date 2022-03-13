# CSc 360: Programming Assignment 2

`mts.c` is a program that simulates an automated control system for a railway track. 
This program emualtes the scheduling of multiple threads sharing a common resource in an Operating System

## Simulation Rules
In order to ensure that each train crosses the track at an approriate time, the following rules are enforce in order:
1. Ensure the train has not already crossed.
2. Ensure a train exists.
3. If four trains in the same direction have travelled back-to-back on the track, let a train waiting in the opposite direction cross.
4. Check if a train has high priority (e.g. `E` or `W`). **NOTE**: This rule has been implemented but is no longer required for this assignment as of 12/03/2022.
5. If two compared trains are travelling in the same direction, the train which finished loading first crosses. If their loading times are the same the train that appears first in the input file crosses the track.
