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


int main() {
	while(1) {
		char holdPrompt[1024];
		printPrompt(holdPrompt);
		char* tokens[128];
		tokenize(tokens, readline(holdPrompt));
		if (!strcmp(tokens[0], "cd")) {
			changeDirectory(tokens);
		} else if (!strcmp(tokens[0], "bg")) {
			addBackground(tokens);
		} else if (!strcmp (tokens[0], "exit")) {
			exit(1);
		}
		executeCommand(tokens);
	}
	return 0;
}

int listLength = 0;

typedef struct Node {
	char command[1024];
	pid_t pid;
	struct Node* next;
} node;

node* head = NULL;

void addBackground(char** commands) {
	pid_t pid = fork();

	if (pid == 0) {
		execvp(commands[1], commands + 1);
	} else if (pid > 0) {
		add(commands, pid);
	}
}


void add(char** command, pid_t pid) {
	node* newNode = (node*)malloc(sizeof(node));
	newNode -> pid = pid;
	newNode -> command[0] = '\0';
	int i = 1;

	while (command[i] != NULL) {
		strcat(newNode -> command, command[i]);
		strcat(newNode -> command, " ");
		i++;
	}
	newNode -> next = head;
	head = newNode;
	listLength++;
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

/*
 * Purpose:	Allow the user to execute arbitrary commands using the 
 * 		the shell program.
 */
void executeCommand(char** commands) {
	pid_t pid = fork();
	pid == 0 ? execvp(commands[0], commands) : waitpid(pid, NULL, 0);
}
