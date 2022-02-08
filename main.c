#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>


/*Function Prototypes*/
void printPrompt();
void executeCommand();
void changeDirectory();
void tokenize();
int countArgs();
void addBackground();
void add();
void listNodes();
void checkChildTermination();
void runCommands();


int main() {
	while(1) {
		char holdPrompt[1024];
		printPrompt(holdPrompt);
		char* tokens[128];

		checkChildTermination();


		tokenize(tokens, readline(holdPrompt));
		runCommands(tokens);
	}
	return 0;
}


typedef struct Node {
	pid_t pid;
	char command[1024];
	struct Node* next;
} node;

node* head = NULL;


void runCommands(char** tokens) {
	if (strcmp(tokens[0], "cd") == 0) {
		changeDirectory(tokens);
	}else if (strcmp(tokens[0], "bg") == 0) {
		add(tokens);
	}else if (strcmp(tokens[0], "bglist") == 0) {
		listNodes();
	}else if (strcmp(tokens[0], "exit") == 0) {
		exit(1);
	}else{
		executeCommand(tokens);
	}
}

/*---------- Helper Functions----------*/


void freeList();

int determineListLength() {
	node* front = head;
	int count = 0;
	while (front != NULL) {
		front = front -> next;
		count++;
	}	
	return count;
}

int countArgs(char** tokens) {
	int i = 0;;
	while (tokens[i] != NULL) {
		tokens[i++];
	}
	return i;
}

void tokenize(char** tokens, char* userInput) {
	char* args = strtok(userInput, " ");
	int i = 0;
	while (args) {
		tokens[i++] = args;
		args = strtok(NULL, " ");
	}
	tokens[i] = NULL;
}


/*----------Part 1----------*/


/* 
 * Purpose:    	display the prompt.
 * Parameters: 	N/A (for right now).
 * Returns:	N/A	
 */
void printPrompt(char* holdPrompt) {
	holdPrompt[0] = '\0';

	char hostname[256];
	gethostname(hostname, sizeof(hostname));

	
	char curr_dir[256];
	getcwd(curr_dir, sizeof(curr_dir));

	strcat(holdPrompt, getlogin());
	strcat(holdPrompt, "@");
	strcat(holdPrompt, hostname);
	strcat(holdPrompt, ": ");
	strcat(holdPrompt, curr_dir);
	strcat(holdPrompt, " > ");
}


/*----------Part 2----------*/

void changeDirectory(char** path) {
	if (path[1] == NULL || strcmp(path[1], "~") == 0) {
		chdir(getenv("HOME"));
	} else if (!strcmp(path[1], "..")) {
		chdir("../");
	} else if (countArgs(path) > 2) {
		perror("chdir");
	}
       	else {
		chdir(path[1]);
	}
}

/*
 * Purpose:	Allow the user to execute arbitrary commands using the 
 * 		the shell program.
 */
void executeCommand(char** commands) {
	pid_t pid = fork();
	pid == 0 ? execvp(commands[0], commands) : waitpid(pid, NULL, 0);
}


/*----------Part 3----------*/


/*Purpose: Check if the child terminates
 *Parameters:
 *Returns:
 *Notes: This function is follows the pseudo implementation learned in tutorial3
 */

void checkChildTermination() {
	if (determineListLength() > 0) {
		pid_t pid = waitpid(0, NULL, WNOHANG);

		while (pid > 0) {
			node* temp = head;
			if (head -> pid == pid) {
				printf("%d: %s has terminated.\n", temp -> pid, temp -> command);
				head = head -> next;
			} else {
				while (temp -> next -> pid != pid) {
					temp = temp -> next;
				}
				printf("%d: %s has terminated.\n", temp -> next -> pid, 
						temp -> next -> command);
				temp -> next = temp -> next -> next;
				free(temp -> next);
			}
			pid = waitpid(0, NULL, WNOHANG);
		}
	}
}

void listNodes() {
	node* temp = head;

	while (temp != NULL) {
		printf("%d: %s\n", temp -> pid, temp -> command);
		temp = temp -> next;
	}
	printf("Total Background Jobs: %d\n", determineListLength());
}	

void add(char** command) {
	pid_t pid = fork();
	if (!pid) {
		execvp(command[1], command + 1);
	} else {
		node* newNode = (node*)malloc(sizeof(node));
		newNode -> pid = pid;
		newNode -> command[0] = '\0';
		for (int i = 1; command[i] != NULL; i++) {
			strcat(newNode -> command, command[i]);
			strcat(newNode -> command, " ");
		}
		newNode -> next = head;
		head = newNode;
	}
}
