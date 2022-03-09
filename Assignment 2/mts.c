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

struct train_t {
  int ID; //file-order
  int loading_time;
  int crossing_time;
  char direction; // east or west
  int is_ready;
  int crossed;
} trains[100];

struct timespec start, stop;
int total_trains;

pthread_cond_t cross[100];
pthread_mutex_t crossMutex[100];
pthread_t threads[100];

pthread_mutex_t finishLoadingMutex = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t finishCrossing = PTHREAD_COND_INITIALIZER;
pthread_mutex_t finishedCrossingMutex = PTHREAD_MUTEX_INITIALIZER;

// 1 tick = 0.1 seconds
int beginningTicks = 0;

void* train_driver();
int all_trains_crossed();
int signal_highest_priority_ready_train();
void operator();
void looping();
void build();
int getTicks();

int main(int argc, char *argv[]) {
  for (int i=0;i<100;i++) {
    pthread_cond_init(&cross[i], NULL);
    pthread_mutex_init(&crossMutex[i], NULL);
  }

  build(argv[1]);
  beginningTicks = getTicks();

  // spawn all train threads
  for (int i=0;i<total_trains; i++) {
    // spawn a thread
    pthread_create(&threads[i], NULL, train_driver, (void*)&trains[i]);
  }

  // run operator on the main thead
  operator();

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


int all_trains_crossed() {
	for (int i=0;i<total_trains;i++) {
    		if (!trains[i].crossed) {
      		// return false if any train has not crossed yet
      			return 0;
    		}
  	}
  	// return true if all trains have crossed
  	return 1;
}

char last_three_trains[] = {'\0', '\0', '\0'};
int starvation_case(char direction) {
  int i;
  for (i=0; i<4; i++) {
    if (last_three_trains[i] == toupper(direction) || last_three_trains[i] == '\0') {
      return 0;
    }
  }
  // return true if all three trains were in opposite direction
  return 1;
}

int signal_highest_priority_ready_train() {
  struct train_t* highest_priority_train = NULL;

  for (int i=total_trains-1; i>=0; i--) {
    struct train_t* node = &trains[i];

    if (!node->is_ready || node->crossed) {
      // skip trains that are not ready or have already crossed
      continue;
    }

    if (highest_priority_train == NULL) {
      // if no other train is ready yet, pick this one
      highest_priority_train = node;
      continue;
    }


   // avoidStarvation(highest_priority_train->direction)
    // check for starvation
    if (starvation_case(node->direction) && !starvation_case(highest_priority_train->direction)) {
      // pick node if it's solving starvation and highest_priority_train is not
      highest_priority_train = node;
      continue;
    } else if (!starvation_case(node->direction) && starvation_case(highest_priority_train->direction)) {
      // skip node if it's not solving starvation and highest_priority_train is
      continue;
    }

    // pick the highest pri
    if (isupper(node->direction) && islower(highest_priority_train->direction)) {
      // if node has higher priority then pick it
      highest_priority_train = node;
      continue;
    } else if (islower(node->direction) && isupper(highest_priority_train->direction)) {
      // if node is lower priority then skip it
      continue;
    }

    // other rules if same priority

    // same direction case
    if (toupper(node->direction) == toupper(highest_priority_train->direction)) {
      // if both in same direction pick then one with smaller loading time
      if (node->loading_time <= highest_priority_train->loading_time) {
        highest_priority_train = node;
      }
      continue;
    }

    // opposite direction case
    if (last_three_trains[0] == '\0') {
       if (toupper(node->direction) == 'E') {
         // pick eastbound train if no train has crossed yet
         highest_priority_train = node;
      }
    } else if (toupper(node->direction) != last_three_trains[0]) {
      // pick the train travelling opposite to the last train that crossed track
      highest_priority_train = node;
    }
  }

  if (highest_priority_train == NULL) {
    // no train was ready
    return 0;
  }

  // update the last_three_trains array
  last_three_trains[2] = last_three_trains[1];
  last_three_trains[1] = last_three_trains[0];
  last_three_trains[0] = toupper(highest_priority_train->direction);

  // take highest_priority_train->crossMutex
  pthread_mutex_lock(&crossMutex[highest_priority_train->ID]);

  // signal highest_priority_train->cross
  pthread_cond_signal(&cross[highest_priority_train->ID]);

  pthread_mutex_lock(&finishedCrossingMutex);
  pthread_mutex_unlock(&crossMutex[highest_priority_train->ID]);
  return 1;
}

// definition of operator function
void operator() {
  // sleep for 0.05 seconds
  usleep(USLEEP_INTERVAL);

  while (!all_trains_crossed()) { // start an infinite loop
    if (!signal_highest_priority_ready_train()) {
      // sleep for 0.05 seconds
      usleep(USLEEP_INTERVAL);
      continue;
    }

    // wait for finishCrossing CV
    pthread_cond_wait(&finishCrossing, &finishedCrossingMutex);
    pthread_mutex_unlock(&finishedCrossingMutex);
  }
}

void build(char *f) {
	FILE *fp = fopen(f, "r");

 	char direction;
  	int loading_time;
  	int crossing_time;
  	total_trains = 0;

  	while(fscanf(fp, "%c %d %d\n", &direction, &loading_time, &crossing_time) != EOF){
    		struct train_t* node = &trains[total_trains];
    		node->ID = total_trains++;
    		node->loading_time = loading_time;
    		node->crossing_time = crossing_time;
    		node->direction = direction;
    		node->is_ready = 0;
    		node->crossed = 0;
  }
  fclose(fp);

}

