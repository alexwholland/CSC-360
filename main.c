#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>


/*Function Prototypes*/

void printPrompt();
void executeCommand();

int main() {
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
	executeCommand(tokens);
	return 0;
}

/* 
 * Purpose:    	display the prompt.
 * Parameters: 	N/A (for right now).
 * Returns:	N/A	
 */
void printPrompt(char* holdPrompt) {
	holdPrompt[0] = '\0';

	char hostname[1024];
	gethostname(hostname, sizeof(hostname));

	
	char curr_dir[1024];
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
