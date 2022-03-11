#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>

#define USLEEP_INTERVAL 50000
#define ON
#define READY
#define STARVATION 4


struct train_t {
  int ID; //file-order
  int loading_time;
  int crossing_time;
  char direction; // east or west
  int is_ready;
  int crossed;
} trains[1000];

//struct train* highest_priority_train;
struct timespec start, stop;
int total_trains = 0;

pthread_cond_t cross[1000];
pthread_mutex_t crossMutex[1000];
pthread_t threads[1000];

pthread_mutex_t finishLoadingMutex;

pthread_cond_t finishCrossing;
pthread_mutex_t finishedCrossingMutex;

// 1 tick = 0.1 seconds
int beginningTicks = 0;

void* train_driver();
int signal_highest_priority_ready_train();
void dispatcher();
void looping();
void build();
int getTicks();
void create();
void updateTrains(char);
int sameDirections();
void joinThreads();
void destroyTrains();

int main(int argc, char *argv[]) {
 	build(argv[1]);
  	beginningTicks = getTicks();
	pthread_mutex_init(&finishLoadingMutex, NULL);
	pthread_cond_init(&finishCrossing, NULL);
	pthread_mutex_init(&finishedCrossingMutex, NULL); 
	create();
  	dispatcher();
	joinThreads();
	destroyTrains();
	return 0;
}


void destroyTrains() {
	pthread_cond_destroy(&finishCrossing);
	pthread_mutex_destroy(&finishLoadingMutex);
	pthread_mutex_destroy(&finishedCrossingMutex);
	for (int i = 0; i < total_trains; i++) {
		pthread_cond_destroy(&cross[i]);
		pthread_mutex_destroy(&crossMutex[i]);
	}

}

void joinThreads() {
	for (int i = 0; i < total_trains; i++) {
		pthread_join(threads[i], NULL);
	}
}

void create() {
	for (int i=0;i<total_trains; i++) {
    		pthread_cond_init(&cross[i], NULL);
  	  	pthread_mutex_init(&crossMutex[i], NULL);
    		pthread_create(&threads[i], NULL, train_driver, (void*)&trains[i]);
  	}
}


int getTicks() {
  struct timespec start;

  if (clock_gettime(CLOCK_REALTIME, &start) == -1){
	perror("clock gettime");
	exit(EXIT_FAILURE);
  }

  return start.tv_sec * 10 + start.tv_nsec / 100000000;
}

int timeFromBeginning() {
  return getTicks() - beginningTicks;
}


void printStatus(int ticks, int pos, struct train_t* t ) {
	printf("%02d:%02d:%02d.%d Train %2d %s %4s\n", 
			ticks/36000, (ticks%36000)/600, (ticks%600/10), ticks%10, 
			t->ID, pos==2?"is ready to go":pos==0?"is ON the main track going":
			"is OFF the main track going", t->direction == 'e'?"East":
			t->direction == 'E'?"East":"West");
}

void looping(struct train_t* t, int *is_ready, int loading_time, int pos) {
	while (!*is_ready) {
		if (timeFromBeginning() >= loading_time) {
			pos==2?pthread_mutex_lock(&finishLoadingMutex):
				pthread_mutex_lock(&finishedCrossingMutex);
			printStatus(loading_time, pos, t);
			*is_ready = 1;
			pos==2?pthread_mutex_unlock(&finishLoadingMutex):
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

  	pthread_mutex_lock(&crossMutex[t->ID]);

  	looping(t, &t->is_ready, t->loading_time,  2);

  	pthread_cond_wait(&cross[t->ID], &crossMutex[t->ID]);

  	int getting_on_time = timeFromBeginning();
  	printStatus(getting_on_time, 0, t);
  	pthread_mutex_unlock(&crossMutex[t->ID]);

	int test = t->crossing_time + getting_on_time;
	looping(t, &t->crossed, test, 1);	

	return NULL;
}


int sameDirection(struct train_t* node, struct train_t** highest) {
	if (toupper(node->direction) == toupper((*highest)->direction) 
		&& (node->loading_time <= (*highest)->loading_time)) {
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


char last_four_trains[] = {'\0', '\0', '\0', '\0'};
int starvation_case(struct train_t* node, struct train_t** highest) {    
	int flag1;
	int flag2;
  	for (int i = 0; i < STARVATION; i++) {
   		if (last_four_trains[i] == toupper(node->direction) || last_four_trains[i] == '\0'){
			flag1 = 1;
    		}
		if (last_four_trains[i] == toupper((*highest)->direction) || last_four_trains[i] == '\0'){
			flag2 = 1;
  	
		}	
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
	if ((toupper(node->direction) == 'E' && last_four_trains[0] == '\0') 
			|| toupper(node->direction) != last_four_trains[0]) {
		*highest = node;
	}
}

int signal_highest_priority_ready_train() {
	struct train_t* highest_priority_train = NULL;
  	for (int i = total_trains - 1; i >= 0; i--) {
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
    // no train was ready
    return 0;
  }

  updateTrains(highest_priority_train->direction);
  // take highest_priority_train->crossMutex
  pthread_mutex_lock(&crossMutex[highest_priority_train->ID]);

  // signal highest_priority_train->cross
  pthread_cond_signal(&cross[highest_priority_train->ID]);

  pthread_mutex_lock(&finishedCrossingMutex);
  pthread_mutex_unlock(&crossMutex[highest_priority_train->ID]);
  return 1;
}

void updateTrains(char direction) {
	for (int i = STARVATION-1; i >= 0; i--) {
		last_four_trains[i] = last_four_trains[i-1];
	}
	last_four_trains[0] = toupper(direction);
}


void dispatcher() {
  	usleep(USLEEP_INTERVAL);
	int notCrossed = 0;
	for (int i = 0; i < total_trains; i++) {
		if (!trains[i].crossed) {
			notCrossed++;
		}
	}
	while (notCrossed > 0) {
		if (!signal_highest_priority_ready_train()) {
			usleep(USLEEP_INTERVAL);
			continue;
		}
   	pthread_cond_wait(&finishCrossing, &finishedCrossingMutex);
    	pthread_mutex_unlock(&finishedCrossingMutex);
    	notCrossed--;
	}
}

void build(char *f) {
	FILE *fp = fopen(f, "r");

	int crossing_time, loading_time;
	char direction;

  	while(fscanf(fp, "%c %d %d\n", &direction, &loading_time, &crossing_time) != EOF) {
    		struct train_t* node = &trains[total_trains];
    		node->ID = total_trains;
    		node->loading_time = loading_time;
    		node->crossing_time = crossing_time;
    		node->direction = direction;
    		node->is_ready;
    		node->crossed;
		total_trains++;
  }
  fclose(fp);
}

