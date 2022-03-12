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
pthread_mutex_t crossMutex[MAXTRAINS];
pthread_t threads[MAXTRAINS];

pthread_mutex_t finishLoadingMutex;

pthread_cond_t finishCrossing;
pthread_mutex_t finishedCrossingMutex;

int beginningTicks = 0;


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
  	beginningTicks = timer();
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
    		struct train_t* node = &trains[trainCount];
    		node->number = trainCount;
    		node->loading_t = load_t;
    		node->crossing_t = cross_t;
    		node->direction = dir;
  	}
  	fclose(fp);
}


int timer() {
  	struct timespec start;

  	if (clock_gettime(CLOCK_REALTIME, &start) == -1){
		perror("clock gettime");
		exit(EXIT_FAILURE);
  	}
  	return (start.tv_sec * TEN) + (start.tv_nsec / HUNMIL);
}


void create() {
	createEmpty();
	for (int i = 0 ; i < trainCount; i++) {
    		pthread_cond_init(&cross[i], NULL);
  	  	pthread_mutex_init(&crossMutex[i], NULL);
    		pthread_create(&threads[i], NULL, train_driver, (void*) &trains[i]);
  	}
}

/*-----Train Functions-----*/

void dispatcher() {
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


void printStatus(int ticks, int pos, struct train_t* t ) {
	printf("%02d:%02d:%02d.%d Train %2d %s %4s\n", 
			ticks/HOUR, (ticks%HOUR)/SIXHUN, (ticks%SIXHUN/TEN), ticks%TEN, 
			t->number, pos==2?"is ready to go":pos==0?"is ON the main track going":
			"is OFF the main track going", t->direction == 'e'?"East":
			t->direction == 'E'?"East":"West");
}

void trainStatus(struct train_t* t, int *is_ready, int loading_time, int pos) {
	while (!*is_ready) {
		if ((timer() - beginningTicks) >= loading_time) {
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

// definition of a train driver thread
void* train_driver(void* arg) {
  	struct train_t* t = arg;
  	pthread_mutex_lock(&crossMutex[t->number]);
  	trainStatus(t, &t->is_ready, t->loading_t, 2);
  	pthread_cond_wait(&cross[t->number], &crossMutex[t->number]);
  	int getting_on_time = timer() - beginningTicks;
  	printStatus(getting_on_time, 0, t);
  	pthread_mutex_unlock(&crossMutex[t->number]);
	trainStatus(t, &t->crossed, (t->crossing_t + getting_on_time), 1);
	pthread_exit(0);
}


int sameDirection(struct train_t* node, struct train_t** highest) {
	if (toupper(node->direction) == toupper((*highest)->direction) 
		&& (node->loading_t <= (*highest)->loading_t)) {
		*highest = node;
		return 1;
      	}
	return 0;
}

int highestPriority(struct train_t* node, struct train_t** highest) {
	if (isupper(node->direction) && islower((*highest)->direction)) {
	    	*highest = node;
      		return 1;
  	} else if (islower(node->direction) && isupper((*highest)->direction)) {
    		return 1;
	}
	return 0; 
}

void createEmpty() {
	for (int i = 0; i < STARVATION; i++) {
		starvationCount[i] = '\0';		
	}
}


int checkStarvation(char direction, int i) {
	if (starvationCount[i] == toupper(direction) || starvationCount[i] == '\0') {
		return 1;
	}
	return 0;
}

int starvation_case(struct train_t* node, struct train_t** highest) {    
	int flag1;
	int flag2;
  	for (int i = 0; i < STARVATION; i++) {
   /*		if (starvationCount[i] == toupper(node->direction) || starvationCount[i] == '\0'){
			flag1 = 1;
    		}
		if (starvationCount[i] == toupper((*highest)->direction) || starvationCount[i] == '\0'){
			flag2 = 1;
  	
		}*/
		flag1 = checkStarvation(node->direction, i);
		flag2 = checkStarvation((*highest)->direction, i);	
	}
	if (flag1 == 0 && flag2 == 1) {
		*highest = node;
		return 1;
	} else if (flag1 == 1 && flag2 == 0) {
		return 1;
	}
  	return 0;
}


void oppositeDirection(struct train_t* node, struct train_t** highest) {
	if ((toupper(node->direction) == 'E' && starvationCount[0] == '\0') 
			|| toupper(node->direction) != starvationCount[0]) {
		*highest = node;
	}
}



void updateTrains(char direction) {
	for (int i = STARVATION-1; i >= 0; i--) {
		starvationCount[i] = starvationCount[i-1];
	}
	starvationCount[0] = toupper(direction);
}



int nextTrain() {
	struct train_t* highest_priority_train = NULL;
  	for (int i = trainCount - 1; i >= 0; i--) {
    		struct train_t* node = &trains[i];
    		if (!node->is_ready || node->crossed) { 
			continue;
		} else if (highest_priority_train == NULL) {
      			highest_priority_train = node;
      			continue;
		} else if (starvation_case(node, &highest_priority_train)) {
			continue;
		} else if (highestPriority(node, &highest_priority_train)) { 
			continue;
		} else if (sameDirection(node, &highest_priority_train)) { 
			continue;
		}
		oppositeDirection(node, &highest_priority_train);
	}
  	if (highest_priority_train == NULL) {
    		return 0;
  	}
  	updateTrains(highest_priority_train->direction);
  	pthread_mutex_lock(&crossMutex[highest_priority_train->number]);
  	pthread_cond_signal(&cross[highest_priority_train->number]);
  	pthread_mutex_lock(&finishedCrossingMutex);
  	pthread_mutex_unlock(&crossMutex[highest_priority_train->number]);
  	return 1;
}

/*-----Terminate Trains-----*/

void joinThreads() {
	for (int i = 0; i < trainCount; i++) {
		pthread_join(threads[i], NULL);
	}
}


void destroyTrains() {
	pthread_cond_destroy(&finishCrossing);
	pthread_mutex_destroy(&finishLoadingMutex);
	pthread_mutex_destroy(&finishedCrossingMutex);
	for (int i = 0; i < trainCount; i++) {
		pthread_cond_destroy(&cross[i]);
		pthread_mutex_destroy(&crossMutex[i]);
	}

}
