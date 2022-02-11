#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <readline/readline.h>
#include <readline/history.h>

#define MAXPRMT 256 //Max prompt size
#define MAXCMD 1024 //Max command size

/*Function Prototypes*/
void displayPrompt();
void executeCmd();
void changeDirectory();
void tokenize();
int countArgs();
void addBackground();
void createProcess();
void displayProcesses();
void checkChildTermination();
void runCommands();
void freeList();
void createNode();

int main(void) {
	while(1) {
		char holdPrompt[MAXPRMT];
		displayPrompt(holdPrompt);

		char* user_input = readline(holdPrompt);
		/*Create a new prompt line if the user presses `Enter`*/
		if(countArgs(user_input) < 1) {
			free(user_input);
			continue;
		}
		checkChildTermination();

		char* tokens[MAXPRMT];
		tokenize(tokens, user_input);

		runCommands(tokens);
		free(user_input);
	}
	freeList();
	return 0;
}

/* Purpose:	typdef sturcture used for storing background process info. 
 * Source:	CSC 360 tutorial-2 slides.
 */
typedef struct Node {
	pid_t pid;
	char command[MAXCMD];
	struct Node* next;
} node;

/*Keep track of the head of the linked list*/
node* head = NULL;

/* Purpose: 	Execute the various supported commands that the user can run.
 * Parameters:	char** cmd - the command inputted by the user.
 * Returns:	void
 */
void runCommands(char** cmd) {
	if (cmd[0] != NULL){
		  (strcmp(cmd[0], "cd") == 0) ? changeDirectory(cmd)
		: (strcmp(cmd[0], "bg") == 0) ? createProcess(cmd)
		: (strcmp(cmd[0], "bglist") == 0) ? displayProcesses()
		: (strcmp(cmd[0], "exit") == 0) ? exit(1)  
	        : executeCmd(cmd);	
	}
}

/*---------- Helper Functions----------*/

/* Purpose:	Free the memory in the linked list.
 * Parameters:	void
 * Returns:	void
 */
void freeList(void){
	node* t_head = head;
	while (t_head != NULL) {
		free(t_head);
	}
	head = NULL;
}

/* Purpose: 	Determine the length of the linked list.
 * Parameters:	void
 * Returns:	count - the length of the linked list.
 */
int determineListLength(void) {
	node* t_head = head;
	int count = 0;
	while (t_head != NULL) {
		t_head = t_head -> next;
		count++;
	}	
	return count;
}

/* Purpose:	Count the number of arguments inputted by the user.
 * Parameters:	char** tokens - Pointer to a char pointer. Holds the user input.
 * Returns:	int i - the number of arguments inputted by the user. 
 */
int countArgs(char** tokens) {
	int i = 0;
	while (tokens[i] != NULL) {
		tokens[i++];
	}
	return i;
}

/* Purpose: 	Tokenize user input strings. Seperate each input line by an 
 * 		empty space.
 * Parameters: 	char** argv - location to store the tokenized input.
 * 		char* userInput - the untokenized user input.
 * Returns:	void
 * Source:	CSC 360 tutorial-2 slides. 	
 */
void tokenize(char** argv, char* userInput) {
	argv[0] = strtok(userInput, " ");
	int i = 0;
	for (i; argv[i] != NULL; i++) {
		argv[i+1] = strtok(NULL, " ");
	}
	argv[i] = NULL;
}


/*----------Part 1----------*/



/* Purpose:    	display the prompt to the terminal. E.g:
 * 		alex@alexh: /home/alex/Desktop/CSC360 >
 * Parameters: 	char* holdPrompt - the prompt.
 * Returns:	void	
 */
void displayPrompt(char* holdPrompt) {
	holdPrompt[0] = '\0';

	char hostname[MAXPRMT], curr_dir[MAXPRMT];

	/*Get the hostname of the current machine.*/
	gethostname(hostname, sizeof(hostname));
	/*Gest the pathname of the current working directory.*/
	getcwd(curr_dir, sizeof(curr_dir));

	snprintf(holdPrompt, MAXCMD, "%s@%s: %s > ", getlogin(), hostname, curr_dir);

}


/*----------Part 2----------*/


/* Purpose: 	Change the current directory depending on the various
 * 		supported user commands.
 * Parameters 	char** user_arg - the users inputted commands.	
 * Returns:	void
 * Source:	CSC 360 tutorial-2 slides.
 * 		https://www.geeksforgeeks.org/making-linux-shell-c/
 */
void changeDirectory(char** user_arg) {
	/*Go to home directory if user inputs `cd` or `cd ~`*/
	if (user_arg[1] == NULL || strcmp(user_arg[1], "~") == 0) {
		chdir(getenv("HOME"));
	/*Move back on directory if user inputs `cd ..`*/
	} else if (!strcmp(user_arg[1], "..")) {
		chdir("../");
	/*Do nothing if the user inputs more then two commands e.g. `cd A B`*/
	} else if (countArgs(user_arg) > 2) {
		perror("Error");
	/*If no such directory exists, warn that it does not exist*/
	} else if (chdir(user_arg[1]) == -1) {
		printf("%s: %s\n", user_arg[1], strerror(errno));
	/*Change to specified directory*/
	}else {
		chdir(user_arg[1]);
	}
}

/* Purpose:	Allow the user to execute arbitrary commands using the 
 * 		the shell program.
 * Parameters:	char** cmd - the user inputted command.
 * Returns:	void
 */
void executeCmd(char** cmd) {
	pid_t pid = fork();
	pid == 0 ? execvp(cmd[0], cmd) : waitpid(pid, NULL, 0);
}


/*----------Part 3----------*/


/* Purpose: 	Check if the child process terminates.
 * Parameters:	void 	
 * Returns:	void
 * Source: 	This function follows the pseudo implementation learned in tutorial-3.
 * Notes:	As recommended by in tutorial, the WNOHANG flag is used to indicate that 
 * 		parent process shouldn't wait.
 */
void checkChildTermination(void) {
	if (determineListLength() > 0) {
		pid_t ter = waitpid(0, NULL, WNOHANG);
		while (ter > 0) {
			node* t_head = head;
			if (head -> pid == ter) {
				printf("%d: %s has terminated.\n", t_head -> pid, t_head -> command);
				head = head -> next;
			} else {
				while (t_head -> next -> pid != ter) {
					t_head = t_head -> next;
				}
				printf("%d: %s has terminated.\n", t_head -> next -> pid, 
						t_head -> next -> command);
				t_head -> next = t_head -> next -> next;
				free(t_head -> next);
			}
			ter = waitpid(0, NULL, WNOHANG);
		}
	}
}

/* Purpose:	Display the currently running background processes.
 * Parameters:	void
 * Returns:	void
 */
void displayProcesses(void) {
	for (node* t_head = head; t_head != NULL; t_head = t_head -> next){
		printf("%d: %s\n", t_head -> pid, t_head -> command);
	}
	printf("Total Background Jobs: %d\n", determineListLength());
}	

/* Purpose: 	Create a background process.	
 * Parameters: 	char** cmd - the command.
 * Returns:	void
 */
void createProcess(char** cmd) {
	pid_t pid = fork();
	pid == 0 ? execvp(cmd[1], cmd + 1) : createNode(cmd, pid);

}

/* Purpose: 	Add a new node (command) to the front of the linked list.
 * Parameters: 	char** cmd - the command.
 * 		pid_t - the process id.
 * Returns:	void
 * Source:	CSC 360 tutorial-3.
 */
void createNode(char** cmd, pid_t pid) {
	node* new_node = (node*)malloc(sizeof(node));
	new_node -> command[0] = '\0';
	new_node -> pid = pid;
	for (int i = 1; cmd[i] != NULL; i++) {
		strcat(new_node -> command, cmd[i]);
		strcat(new_node -> command, " ");
	}
	new_node -> next = head;
	head = new_node;
}
