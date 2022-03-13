#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <ctype.h>

#define HUNMIL 100000000
#define HOUR 36000
#define SIXHUN 600
#define TEN 10
#define USLEEP_INTERVAL 50000
#define MAXTRAINS 10000
#define STARVATION 4


struct train_t {
  int number;
  char direction;
  int loading_t;
  int crossing_t;
  int is_ready;
  int crossed;
} trains[MAXTRAINS];

int trainCount = 0;
int starvationCount[STARVATION];

pthread_cond_t cross[MAXTRAINS];
pthread_mutex_t track[MAXTRAINS];
pthread_t threads[MAXTRAINS];

pthread_mutex_t finishLoadingMutex;

pthread_cond_t finishCrossing;
pthread_mutex_t finishedCrossingMutex;

int timeTrack = 0;


/*Function Prototypes*/
void extractData();
void* train_driver();
int nextTrain();
void dispatcher();
int timer();
void create();
int sameDirections();
void joinThreads();
void destroyTrains();
void createEmpty();

int main(int argc, char *argv[]) {
 	extractData(argv[1]);
  	timeTrack = timer();
	pthread_mutex_init(&finishLoadingMutex, NULL);
	pthread_cond_init(&finishCrossing, NULL);
	pthread_mutex_init(&finishedCrossingMutex, NULL); 
	create();
  	dispatcher();
	joinThreads();
	destroyTrains();
	return 0;
}

/*-----Train Setup-----*/

void extractData(char *f) {
	FILE *fp = fopen(f, "r");

	int cross_t, load_t;
	char dir;

  	for (;fscanf(fp, "%c %d %d\n", &dir, &load_t, &cross_t) != EOF; trainCount++) {
    		struct train_t* train = &trains[trainCount];
    		train->number = trainCount;
    		train->loading_t = load_t;
    		train->crossing_t = cross_t;
    		train->direction = dir;
  	}
  	fclose(fp);
}

/* Purpose: 	Start a timer to keep track of the train timings.
 * Parameters: 	void
 * Returns: 	The time.
 * Source: 	Tutorial 6 slides.
 */
int timer(void) {
  	struct timespec start;

  	if (clock_gettime(CLOCK_REALTIME, &start) == -1){
		perror("clock gettime");
		exit(EXIT_FAILURE);
  	}
  	return (start.tv_sec * TEN) + (start.tv_nsec / HUNMIL);
}

/* Purpose: 	For each train; create a thread, condition variable, and mutex.
 * Parameters: 	void
 * Returns: 	void
 */
void create(void) {
	createEmpty();
	for (int i = 0 ; i < trainCount; i++) {
    		pthread_cond_init(&cross[i], NULL);
  	  	pthread_mutex_init(&track[i], NULL);
    		pthread_create(&threads[i], NULL, train_driver, (void*) &trains[i]);
  	}
}

/*-----Train Functions-----*/

/* Purpose: 	Dispatch each train.
 * Parameters:	void
 * Returns:	void
 */
void dispatcher(void) {
	usleep(USLEEP_INTERVAL); 
	int notCrossed = 0;
	for (int i = 0; i < trainCount; i++) {
		if (!trains[i].crossed) {
			notCrossed++;
		}
	}
	while (notCrossed > 0) {
		if (!nextTrain()) {
			usleep(USLEEP_INTERVAL);
			continue;
		}
   	pthread_cond_wait(&finishCrossing, &finishedCrossingMutex);
    	pthread_mutex_unlock(&finishedCrossingMutex);
    	notCrossed--;
	}
}


/* Purpose: 	Print the status of each train in the following formats:
 *		 - 00:00:00.0 Train <number> is ready to go <East/West>
 *		 - 00:00:00.0 Train <number> is <ON/OFF> the main track going <EAST/WEST>
 * Parameters:	int utime - unformatted time
 * 		int pos - determines if the Train is loaded or or ON/OFF the main track eg:
 * 				- pos = 2: the train has been loaded
 * 				- pos = 1: the train ON/OFF the main track
 * 		struct train_t* t - Struct data type of train information.
 * Returns:	void		
 */
void printStatus(int utime, int pos, struct train_t* t ) {
	printf("%02d:%02d:%02d.%d Train %2d %s %4s\n", 
			utime/HOUR, (utime%HOUR)/SIXHUN, (utime%SIXHUN/TEN), utime%TEN, 
			t->number, pos==2?"is ready to go":pos==0?"is ON the main track going":
			"is OFF the main track going", t->direction == 'e'?"East":
			t->direction == 'E'?"East":"West");
}


/* Purpose:	Determine if the train has finished loading and crossing.	
 * Parameters:	stuct train_t* t - Struct data type of train information
 *		int *is_ready - determines if the train is ready to cross
 *		int loading_time - loading time of a train.
 *		int pos - a flag to determine if the train has finished loading
 *			  or crossing:
 *			  	- 2: loaded
 *			  	- 1: crossed
 * Returns:	void
 */
void trainStatus(struct train_t* t, int *is_ready, int loading_time, int pos) {
	while (!*is_ready) {
		if ((timer() - timeTrack) >= loading_time) {
			pos == 2 ? pthread_mutex_lock(&finishLoadingMutex):
				pthread_mutex_lock(&finishedCrossingMutex);
			printStatus(loading_time, pos, t);
			*is_ready = 1;
			pos == 2 ? pthread_mutex_unlock(&finishLoadingMutex):
				pthread_cond_signal(&finishCrossing),
				pthread_mutex_unlock(&finishedCrossingMutex);			
		} else {

			usleep(USLEEP_INTERVAL);
		}
	}
}

/* Purpose: 	Start the train routine for each thread.
 * Parameters:	void* args - a train
 * Returns:	void* 
 */
void* train_driver(void* args) {
  	struct train_t* t = args;
  	pthread_mutex_lock(&track[t->number]);
  	trainStatus(t, &t->is_ready, t->loading_t, 2);
  	pthread_cond_wait(&cross[t->number], &track[t->number]);
  	int getting_on_time = timer() - timeTrack;
  	printStatus(getting_on_time, 0, t);
  	pthread_mutex_unlock(&track[t->number]);
	trainStatus(t, &t->crossed, (t->crossing_t + getting_on_time), 1);
	pthread_exit(0);
}

/* Purpose:	Check if two trains are traveling in the same direction.
 * Parameters:	struct train_t* train - Struct data type of trian information.
 *		struct train_t** highest - in formation of the currently highest
 *					   priority train.
 * Returns:	1 if trains are in the same direction or the current train has a smaller
 * 		loading time the the highest priority train. Else 0.
 */
int sameDirection(struct train_t* train, struct train_t** highest) {
	if (toupper(train->direction) == toupper((*highest)->direction) 
		&& (train->loading_t <= (*highest)->loading_t)) {
		*highest = train;
		return 1;
      	}
	return 0;
}

/* Purpose: 	Determines if a train is going Eeast or West:
 * 			- High priority: East (E) or West (W)
 * 			- Low priority: east (e) or west (w)
 * Parameters:	char direction - the direction of the train.
 * Returns:	1 if high priority. Else 0.
 */
int priority(char direction) {
 	if (direction == 'E' || direction == 'W') {
		return 1;
 	}
	return 0;
}


/* Purpose: 	Determines the direction priority of a train.
 * Parameters:	struct train_t* train - Struct data type of train information.
 * 		struct train_t** highest - Current highest priority train.
 * Returns:	1 if one train has higher direction priority over the other.
 * 		Else 0.
 */
int highestPriority(struct train_t* train, struct train_t** highest) {
	if (priority(train->direction) && !priority((*highest)->direction)) {
	    	*highest = train;
      		return 1;
  	} else if (!priority(train->direction) && priority((*highest)->direction)) {
    		return 1;
	}
	return 0; 
}


/* Purpose: 	Create empty int array.
 * Parameters:	void
 * Returns:	void
 */
void createEmpty(void) {
	for (int i = 0; i < STARVATION; i++) {
		starvationCount[i] = '\0';		
	}
}


/* Purpose: 	Checks starvation. Helper function for starvationCase()
 * Parameters:	direction - direction of train.
 * Returns:	1 if starvation occurs. Else 0.	
 */
int checkStarvation(char direction, int i) {
	if (starvationCount[i] == toupper(direction) || starvationCount[i] == '\0') {
		return 1;
	}
	return 0;
}


/* Purpose:	Checks starvation case. If there are four trains that have crossed 
 * 		in the same direction, the train waiting in the opposite direction 
 * 		gets a chance to cross.
 * Parameters:	struct train_t* train - Struct data type of train information.
 * 		struct train_t** highest - Current highest priority train.
 * Returns:	1 if starvation occurs. Else 0.
 */
int starvationCase(struct train_t* train, struct train_t** highest) {    
	int flag1;
	int flag2;
  	for (int i = 0; i < STARVATION; i++) {
		flag1 = checkStarvation(train->direction, i);
		flag2 = checkStarvation((*highest)->direction, i);	
	}
	if (flag1 == 0 && flag2 == 1) {
		*highest = train;
		return 1;
	} else if (flag1 == 1 && flag2 == 0) {
		return 1;
	}
  	return 0;
}


/* Purpose: 	Check if trains are travelling in the opposite direction.
 * Parameters:	struct train_t* train - Struct data type of train information.
 * 		struct train_t** highest - Current highest priority train.
 * Returns: 	void
 */
void oppositeDirection(struct train_t* train, struct train_t** highest) {
	if (toupper(train->direction) != starvationCount[0] || toupper(train->direction) == 'E' 
				&& starvationCount[0] == '\0') {
		*highest = train;
	}
}


/* Purpose: 	Update the last four trains, and let let the highest priority
 * 		cross the track.	
 * Parameters: 	struct train_t* priorityTrain - The train to cross the track.
 * Returns: 	void
 */
void updateTrains(struct train_t* priorityTrain) {
	for (int i = STARVATION-1; i >= 0; i--) {
		starvationCount[i] = starvationCount[i-1];
	}
	starvationCount[0] = toupper(priorityTrain->direction);
	pthread_mutex_lock(&track[priorityTrain->number]);
  	pthread_cond_signal(&cross[priorityTrain->number]);
  	pthread_mutex_lock(&finishedCrossingMutex);
  	pthread_mutex_unlock(&track[priorityTrain->number]);
  	
}


/* Purpose: 	Determine which train should cross the track next. This
 * 		function organizes the simulation rules.
 * Parameters:  void
 * Returns:	1 if highest priority train is found and crossed. 
 * 		0 if the highest priority train is NULL.
 */
int nextTrain(void) {
	struct train_t* priorityTrain = NULL;
  	for (int i = trainCount - 1; i >= 0; i--) {
    		struct train_t* node = &trains[i];
    		if (!node->is_ready || node->crossed) {
		       	continue;
		} else if (priorityTrain == NULL) {
			priorityTrain = node;
			continue;
		} else if (starvationCase(node, &priorityTrain)) {
			continue;
		} else if (highestPriority(node, &priorityTrain)) { 
			continue;
		} else if (sameDirection(node, &priorityTrain)) {
			continue;
		}
		oppositeDirection(node, &priorityTrain);
	}
  	if (priorityTrain == NULL) { 
		return 0;
	}
  	updateTrains(priorityTrain);
  	return 1;
}

/*-----Terminate Trains-----*/

/* Purpose: 	Join the threads.
 * Parameters:	void
 * Returns:	void
 */
void joinThreads(void) {
	for (int i = 0; i < trainCount; i++) {
		pthread_join(threads[i], NULL);
	}
}

/* Purpose: 	Destroy the train mutex's and conditon variables
 * Parameters:  void
 * Returns:	void
 */
void destroyTrains(void) {
	pthread_cond_destroy(&finishCrossing);
	pthread_mutex_destroy(&finishLoadingMutex);
	pthread_mutex_destroy(&finishedCrossingMutex);
	for (int i = 0; i < trainCount; i++) {
		pthread_cond_destroy(&cross[i]);
		pthread_mutex_destroy(&track[i]);
	}

}
