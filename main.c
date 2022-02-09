#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>


/*Function Prototypes*/
void printPrompt();
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


int main() {
	while(1) {
		char holdPrompt[2024];
		printPrompt(holdPrompt);
		char* tokens[128];
		char* input = readline(holdPrompt);

		if(countArgs(input) < 1) {
			free(input);
			continue;
		}

		checkChildTermination();


		tokenize(tokens, input);
		runCommands(tokens);
	}
	freeList();
	return 0;
}

/* typdef sturcture used to group tgether data in a node datatype. */
typedef struct Node {
	pid_t pid;
	char command[1024];
	struct Node* next;
} node;

node* head = NULL;


void runCommands(char** tokens) {

	if (tokens[0] != NULL){
		if (strcmp(tokens[0], "cd") == 0) {
			changeDirectory(tokens);
		}else if (strcmp(tokens[0], "bg") == 0) {
			add(tokens);
		}else if (strcmp(tokens[0], "bglist") == 0) {
			listNodes();
		}else if (strcmp(tokens[0], "exit") == 0) {
			exit(1);
		}else{
			executeCmd(tokens);
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
 * Purpose:    	display the prompt to the terminal.
 * Parameters: 	char* holdPrompt - 
 * Returns:	void	
 */
void printPrompt(char* holdPrompt) {
	holdPrompt[0] = '\0';

	char hostname[256];
	gethostname(hostname, sizeof(hostname));

	
	char curr_dir[256];
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


/* Purpose: 	Check if the child process terminates
 * Parameters: 	
 * Returns:
 * Source: 	This function is follows the pseudo implementation learned in tutorial-3
 */

void checkChildTermination(void) {
	if (determineListLength() > 0) {
		pid_t pid = waitpid(0, NULL, WNOHANG);

		while (pid > 0) {
			node* t_head = head;
			if (head -> pid == pid) {
				printf("%d: %s has terminated.\n", t_head -> pid, t_head -> command);
				head = head -> next;
			} else {
				while (t_head -> next -> pid != pid) {
					t_head = t_head -> next;
				}
				printf("%d: %s has terminated.\n", t_head -> next -> pid, 
						t_head -> next -> command);
				t_head -> next = t_head -> next -> next;
				free(t_head -> next);
			}
			pid = waitpid(0, NULL, WNOHANG);
		}
	}
}

void listNodes(void) {
	for (node* t_head = head; t_head != NULL; t_head = t_head -> next){
		printf("%d: %s\n", t_head -> pid, t_head -> command);
	}
	printf("Total Background Jobs: %d\n", determineListLength());
}	

void add(char** cmd) {
	pid_t pid = fork();
	if (!pid) {
		execvp(cmd[1], cmd + 1);
	} else {
		node* newNode = (node*)malloc(sizeof(node));
		newNode -> pid = pid;
		newNode -> command[0] = '\0';
		for (int i = 1; cmd[i] != NULL; i++) {
			strcat(newNode -> command, cmd[i]);
			strcat(newNode -> command, " ");
		}
		newNode -> next = head;
		head = newNode;
	}
}
