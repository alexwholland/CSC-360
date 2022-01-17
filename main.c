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

int main() {
	while(1) {	
		char holdPrompt[1024];
		printPrompt(holdPrompt);
		char* userInput = readline(holdPrompt);

		char* args = strtok(userInput, " ");
		char* tokens[128];

		int i = 0;
		while (args) {
			tokens[i++] = args;
			args = strtok(NULL, " ");
		}
		tokens[i] = NULL;
		if (!strcmp(tokens[0], "cd")) {
			changeDirectory(tokens);
		}
		executeCommand(tokens);
	}
return 0;
}


void changeDirectory(char** path) {
	if (path[1] == NULL || !strcmp(path[1], "~")) {
		chdir(getenv("HOME"));
	} else if (!strcmp(path[1], "..")) {
		chdir("../");
	} else {
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
	
	if (pid == 0) {
		execvp(commands[0], commands);
	} else {
		waitpid(pid, NULL, 0);
	}
}
