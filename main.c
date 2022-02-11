#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <readline/readline.h>
#include <readline/history.h>


/*Function Prototypes*/
void displayPrompt();
void executeCmd();
void changeDirectory();
void tokenize();
int countArgs();
void addBackground();
void add();
void listNodes();
void checkChildTermination();
void runCommands();
void freeList();
void createNode();

int main() {
	while(1) {
		char holdPrompt[256];
		displayPrompt(holdPrompt);
		char* tokens[256];
		char* user_input = readline(holdPrompt);

		if(countArgs(user_input) < 1) {
			free(user_input);
			continue;
		}

		checkChildTermination();


		tokenize(tokens, user_input);
		runCommands(tokens);
		free(user_input);
	}
	freeList();
	return 0;
}

/* Purpose:	typdef sturcture used to group tgether data in a node datatype. 
 * Source:	CSC 360 tutorial-2 slides.
 */
typedef struct Node {
	pid_t pid;
	char command[1024];
	struct Node* next;
} node;

node* head = NULL;

/* Purpose: 	Execute the various supported commands that the user can run.
 * Parameters:	char** cmd - the command inputted by the user.
 * Returns:	void
 */
void runCommands(char** cmd) {

	if (cmd[0] != NULL){
		if (strcmp(cmd[0], "cd") == 0) {
			changeDirectory(cmd);
		}else if (strcmp(cmd[0], "bg") == 0) {
			add(cmd);
		}else if (strcmp(cmd[0], "bglist") == 0) {
			listNodes();
		}else if (strcmp(cmd[0], "exit") == 0) {
			exit(1);
		}else{
			executeCmd(cmd);
		}
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
 * Parameters: 	char** tokens - location to store the tokenized input.
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


/* 
 * Purpose:    	display the prompt to the terminal.
 * Parameters: 	char* holdPrompt - 
 * Returns:	void	
 */
void displayPrompt(char* holdPrompt) {
	holdPrompt[0] = '\0';

	char hostname[256], curr_dir[256];

	gethostname(hostname, sizeof(hostname));
	getcwd(curr_dir, sizeof(curr_dir));

	snprintf(holdPrompt, 1024, "%s@%s: %s > ", getlogin(), hostname, curr_dir);

}


/*----------Part 2----------*/

/* Purpose: 	Change the current directory depending on the various
 * 		supported user commands.
 * Parameters 	char** user_arg - the users inputted commands.	
 * Returns:	void
 * Source:	CSC 360 tutorial-2 slides.
 */
void changeDirectory(char** user_arg) {
	if (user_arg[1] == NULL || strcmp(user_arg[1], "~") == 0) {
		chdir(getenv("HOME"));
	} else if (!strcmp(user_arg[1], "..")) {
		chdir("../");
	} else if (countArgs(user_arg) > 2) {
		perror("chdir");
	}
       	else {
		chdir(user_arg[1]);
	}
}

/* Purpose:	Allow the user to execute arbitrary commands using the 
 * 		the shell program.
 * Parameters:			
 */
void executeCmd(char** cmd) {
	pid_t pid = fork();
	pid == 0 ? execvp(cmd[0], cmd) : waitpid(pid, NULL, 0);
}


/*----------Part 3----------*/


/* Purpose: 	Check if the child process terminates.
 * Parameters:	void 	
 * Returns:	void
 * Source: 	This function is follows the pseudo implementation learned in tutorial-3.
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
void listNodes(void) {
	for (node* t_head = head; t_head != NULL; t_head = t_head -> next){
		printf("%d: %s\n", t_head -> pid, t_head -> command);
	}
	printf("Total Background Jobs: %d\n", determineListLength());
}	

/* Purpose: 	Add a new node (command) to the front of the linked list.	
 * Parameters: 	char** cmd - the command.
 * Returns:	void
 */
void add(char** cmd) {
	pid_t pid = fork();
/*	if (!pid) {
		execvp(cmd[1], cmd + 1);
	} else {
		createNode(cmd, pid);
	}*/
	pid == 0 ? execvp(cmd[1], cmd + 1) : createNode(cmd, pid);

}

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
