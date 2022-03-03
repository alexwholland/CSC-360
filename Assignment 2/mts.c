#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#define HIGH 1
#define LOW 0

typedef struct Train {
	pthread_cond_t *train_convar;
	char direction;
	int crossing_time;
	int loading_time;
	int priority;
	int number;
} train;

int trainCount = 0;
int builtTrains = 0;

int countTrains();
void buildTrains();

int main(int argc, char *argv[]) {
	if (argc < 2) {
		printf("Could not read file input\n");
   		return 1;
	}

   	trainCount = countTrains(argv[1]);
//	printf("%d\n", trainCount);

	pthread_cond_t train_conditions[trainCount];

	train *trains = malloc(trainCount * sizeof(*trains));
	buildTrains(trains, argv[1], train_conditions);

	printf("%d\n", trains[0].loading_time);
  	return 0;
}

int countTrains(char *f) {
	FILE *fp = fopen(f, "r");
	int count = 0;
	for (char c = fgetc(fp); c != EOF; c = fgetc(fp)) {
		if (c == '\n') {
			count++;
		}
	}
	fclose(fp);
	return count;
}

int priority(char direction) {
	if (direction == 'W' || direction == 'E') {
		return HIGH;
	}
		return LOW;
}

void buildTrains(train *trains, char *f, pthread_cond_t *train_conditions) {
	for (int i = 0; i < trainCount; i++) {
		pthread_cond_init(&train_conditions[i], NULL);
	}
	char direction[3], loading_time [3], crossing_time[3];
	int train_number = 0;
	FILE *fp = fopen(f, "r");
	while (fscanf(fp, "%s %s %s", direction, loading_time, crossing_time) != EOF) {
		trains[train_number].number = train_number;
		trains[train_number].direction = direction[0];
		trains[train_number].priority = priority(direction[0]);
		trains[train_number].loading_time = atoi(loading_time);
		trains[train_number].crossing_time = atoi(crossing_time);
		trains[train_number].train_convar = &train_conditions[train_number];
		train_number++;
		builtTrains++;
	}
	fclose(fp);
}
