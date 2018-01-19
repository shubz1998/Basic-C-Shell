#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <termios.h>
#include <ctype.h>

#define LIMIT 256 // max number of tokens for a command
#define MAXLINE 1024 // max number of characters from user input
#define MAXHISTORY 100
#define TRUE 1
#define FALSE !TRUE


//global vars 
static char* currentDirectory;
extern char** environ;

int no_reprint_prmpt;
pid_t pid;

int count = 0;
char **history;

//prototypes
void signalHandler_int(int p);
int changeDirectory(char * args[]);
void signalHandler_child(int p);
int commandHandler(char * args[]);

//Method used to print the welcome screen of our shell
void welcomeScreen(){
    printf("\n Shell by group 23 \n");
    printf("\n 150101068 Shivam Gupta \n");
    printf("\n 150101069 Shivram Gowtham \n");
    printf("\n 150101072 Shubham Singhal \n");
}

//SIGNAL HANDLERS
//signal handler for SIGCHLD
void signalHandler_child(int p){
	/* Wait for all dead processes.
	 * We use a non-blocking call (WNOHANG) to be sure this signal handler will not
	 * block if a child was cleaned up in another part of the program. */
	while (waitpid(-1, NULL, WNOHANG) > 0) {
	}
	printf("\n");
}

//Signal handler for SIGINT
void signalHandler_int(int p){
	// We send a SIGTERM signal to the child process
	
	if (kill(pid,SIGTERM) == 0){
		printf("\nProcess %d received a SIGINT signal %d\n",pid,getpid());
		no_reprint_prmpt = 1;			
	}else{
		printf("\n");
	}
}
void alarm_handler(int p){
	// We send a SIGTERM signal to the child process
	kill(getpid(),SIGTERM);		
}

/**
 *	Displays the prompt for the shell
 */
void shellPrompt(){
	// We print the prompt in the form "<user>@<host> <cwd> >"
	char hostn[1204] = "";
	gethostname(hostn, sizeof(hostn));
	printf("%s@%s %s > ", getenv("LOGNAME"), hostn, getcwd(currentDirectory, 1024));
}

/**
 * Method to change directory
 */
int changeDirectory(char* args[]){
	// If we write no path (only 'cd'), then go to the home directory
	if (args[1] == NULL) {
		chdir(getenv("HOME")); 
		return 1;
	}
	// Else we change the directory to the one specified by the argument, if possible
	else{ 
		if (chdir(args[1]) == -1) {
			printf(" %s: no such directory\n", args[1]);
            return -1;
		}
	}
	return 0;
}

/**
 * Method used to manage the environment variables with different options
 */ 
int manageEnviron(char * args[], int option){
	char **env_aux;
	switch(option){
		// Case 'environ': we print the environment variables along with their values
		case 0: 
			for(env_aux = environ; *env_aux != 0; env_aux ++){
				printf("%s\n", *env_aux);
			}
			break;
		// Case 'setenv': we set an environment variable to a value
		case 1: 
			if((args[1] == NULL) && args[2] == NULL){
				printf("%s","Not enought input arguments\n");
				return -1;
			}
			// We use different output for new and overwritten variables
			if(getenv(args[1]) != NULL){
				printf("%s", "The variable has been overwritten\n");
			}else{
				printf("%s", "The variable has been created\n");
			}
			// If we specify no value for the variable, we set it to ""
			if (args[2] == NULL){
				setenv(args[1], "", 1);
			// We set the variable to the given value
			}else{
				setenv(args[1], args[2], 1);
			}
			break;
		// Case 'unsetenv': we delete an environment variable
		case 2:
			if(args[1] == NULL){
				printf("%s","Not enought input arguments\n");
				return -1;
			}
			if(getenv(args[1]) != NULL){
				unsetenv(args[1]);
				printf("%s", "The variable has been erased\n");
			}else{
				printf("%s", "The variable does not exist\n");
			}
		break;
	}
	return 0;
}
 
/**
* Method for launching a program.
*/ 
int checkdigit(char *s)
{
	for (int i = 0; s[i]!=NULL; ++i)
		if((int)(s[i]) <48 || (int)(s[i]) >57) return 0;
	return 1;
}
void launchProg(char **args){	 
	 int err = -1;
	 
	 if((pid=fork())==-1){
		 printf("Child process could not be created\n");
		 return;
	 }
	 // pid == 0 implies the following code is related to the child process
	if(pid==0){
		// We set the child to ignore SIGINT signals (we want the parent
		// process to handle them with signalHandler_int)	
		int i=0;
		while(args[i]!=NULL) i++;
		i--;
		if(checkdigit(args[i])) 
		{
			alarm(atoi(args[i]));
			signal(SIGALRM, alarm_handler);
			args[i]=NULL;
		}
		signal(SIGINT, SIG_IGN);

		// We set parent=<pathname>/simple-c-shell as an environment variable
		// for the child
		setenv("parent",getcwd(currentDirectory, 1024),1);	
		// If we launch non-existing commnads we end the process
		if (execvp(args[0],args)==err){
			printf("Command not found");
			kill(getpid(),SIGTERM);
		}
	 }
	// we wait for the child to finish.
	 waitpid(pid,NULL,0); 
}
 
/**
* Method used to manage I/O redirection
*/ 
void fileIO(char * args[], char* inputFile, char* outputFile, int option){
	 
	int err = -1;
	
	int fileDescriptor; // between 0 and 19, describing the output or input file
	
	if((pid=fork())==-1){
		printf("Child process could not be created\n");
		return;
	}
	if(pid==0){
		// Option 0: output redirection
		if (option == 0)
		{
			// We open (create) the file truncating it at 0, for write only
			fileDescriptor = open(outputFile, O_CREAT | O_TRUNC | O_WRONLY, 0600); 
			// We replace de standard output with the appropriate file
			dup2(fileDescriptor, STDOUT_FILENO); 
			close(fileDescriptor);
		// Option 1: input and output redirection
		}
		else if (option == 1)
		{
			// We open file for read only (it's STDIN)
			fileDescriptor = open(inputFile, O_RDONLY, 0600);  
			// We replace de standard input with the appropriate file
			dup2(fileDescriptor, STDIN_FILENO);
			close(fileDescriptor);
			// Same as before for the output file
			fileDescriptor = open(outputFile, O_CREAT | O_TRUNC | O_WRONLY, 0600);
			dup2(fileDescriptor, STDOUT_FILENO);
			close(fileDescriptor);		 
		}
		// Option 2: input redirection
		else if (option == 2){
			// We open file for read only (it's STDIN)
			fileDescriptor = open(inputFile, O_RDONLY, 0600);  
			// We replace de standard input with the appropriate file
			dup2(fileDescriptor, STDIN_FILENO);
			close(fileDescriptor);
		}

		setenv("parent",getcwd(currentDirectory, 1024),1);
		if (execvp(args[0],args)==err)
		{
			printf("err");
			kill(getpid(),SIGTERM);
		}		 
	}
	waitpid(pid,NULL,0);
}
	
void printhistory(char *historylines)
{
	int historyline = atoi(historylines);
	/*printf("%d\n",historyline );
	for (int i = 1; i<=MAXHISTORY; ++i){
		printf("%d --> %s\n",i,history[i-1]);
	}*/
	for (int i = 1; i<=historyline; ++i){
		if(history[(count - i  + MAXHISTORY)%MAXHISTORY]!=NULL)
			printf("%s\n",history[(count - i + MAXHISTORY)%MAXHISTORY]);
	}
}

void issuecmd(char *nthcmd)
{
	int n = atoi(nthcmd);
	char * line = history[(count - n + MAXHISTORY)%MAXHISTORY];
	char *tokens[LIMIT];
	if((tokens[0] = strtok(line," \n\t")) == NULL){
		printf("%s\n", "not enough commands in history");
		return ;
	}		
	int numTokens = 1;
	while((tokens[numTokens] = strtok(NULL, " \n\t")) != NULL) numTokens++;
	commandHandler(tokens);
}

void rmexcept(char *args[]){
	char *cmd = (char*)calloc(500,sizeof(char));

	strcat(cmd, "find .");
	for (int i = 1; args[i]!=NULL; ++i)
	{
		strcat(cmd," ! -name ");
		strcat(cmd,args[i]);
	}
	strcat(cmd," -type f -exec rm -f {} +");
	char * tokens[LIMIT]; // array for the different tokens in the command
	int numTokens;

	tokens[0] = strtok(cmd," \n\t");
	numTokens = 1;
	while((tokens[numTokens] = strtok(NULL, " \n\t")) != NULL) numTokens++;
	int i = 0;
	while(!tokens[i])
	{
		printf("%s\n",tokens[i]);
		i++;
	}
	commandHandler(tokens);
}
/**
* Method used to handle the commands entered via the standard input
*/ 
int commandHandler(char * args[]){
	int i = 0;
	int j = 0;
	
	int fileDescriptor;
	int standardOut;
	
	int aux;
	
	char *args_aux[256];
	
	// We look for the special characters and separate the command itself
	// in a new array for the arguments
	while ( args[j] != NULL){
		if ( (strcmp(args[j],">") == 0) || (strcmp(args[j],"<") == 0) || (strcmp(args[j],"&") == 0)){
			break;
		}
		args_aux[j] = args[j];
		j++;
	}
	
	// 'exit' command quits the shell
	if(strcmp(args[0],"exit") == 0) exit(0);
	// 'pwd' command prints the current directory
	else if(strcmp(args[0],"history") == 0){
		if(args[1]==NULL){
			printf("%s\n","provide enough arguments");
			return -1;
		}
		printhistory(args[1]);
	}
	else if(strcmp(args[0],"issue") == 0){
		if(args[1]==NULL){
			printf("%s\n","provide enough arguments");
			return -1;
		}
		issuecmd(args[1]);
	}
	else if(strcmp(args[0],"rmexcept") == 0){
		rmexcept(args);
	}
 	else if (strcmp(args[0],"pwd") == 0){
		if (args[j] != NULL){
			// If we want file output
			if ( (strcmp(args[j],">") == 0) && (args[j+1] != NULL) ){
				fileDescriptor = open(args[j+1], O_CREAT | O_TRUNC | O_WRONLY, 0600); 
				// We replace de standard output with the appropriate file
				standardOut = dup(STDOUT_FILENO); 	// first we make a copy of stdout
													// because we'll want it back
				dup2(fileDescriptor, STDOUT_FILENO); 
				close(fileDescriptor);
				printf("%s\n", getcwd(currentDirectory, 1024));
				dup2(standardOut, STDOUT_FILENO);
			}
		}else{
			printf("%s\n", getcwd(currentDirectory, 1024));
		}
	} 
 	// 'clear' command clears the screen
	else if (strcmp(args[0],"clear") == 0) system("clear");
	// 'cd' command to change directory
	else if (strcmp(args[0],"cd") == 0) changeDirectory(args);
	// 'environ' command to list the environment variables
	else if (strcmp(args[0],"environ") == 0){
		if (args[j] != NULL){
			// If we want file output
			if ( (strcmp(args[j],">") == 0) && (args[j+1] != NULL) ){
				fileDescriptor = open(args[j+1], O_CREAT | O_TRUNC | O_WRONLY, 0600); 
				// We replace de standard output with the appropriate file
				standardOut = dup(STDOUT_FILENO); 	// first we make a copy of stdout
													// because we'll want it back
				dup2(fileDescriptor, STDOUT_FILENO); 
				close(fileDescriptor);
				manageEnviron(args,0);
				dup2(standardOut, STDOUT_FILENO);
			}
		}else{
			manageEnviron(args,0);
		}
	}
	// 'setenv' command to set environment variables
	else if (strcmp(args[0],"setenv") == 0) manageEnviron(args,1);
	// 'unsetenv' command to undefine environment variables
	else if (strcmp(args[0],"unsetenv") == 0) manageEnviron(args,2);
	else{
		// If none of the preceding commands were used We have to detect if I/O redirection,
		while (args[i] !=NULL){
			// If '<' is detected, we have Input and Output redirection.
			// First we check if the structure given is the correct one,
			// and if that is the case we call the appropriate method
			if (strcmp(args[i],"<") == 0)
			{
				aux = i+1;
				if(args[aux] != NULL && args[aux+1] == NULL)
				{
					fileIO(args_aux,args[i+1],NULL,2);
					return 1;
				}
				else if(args[aux] != NULL && (strcmp(args[aux+1],">") != 0))
				{
					fileIO(args_aux,args[i+1],NULL,2);
					return 1;
				}
				
				else if (args[aux] == NULL || args[aux+1] == NULL || args[aux+2] == NULL )
				{
					printf("Not enough input arguments\n");
					return -1;
				}
				else
				{
					if (strcmp(args[aux+1],">") != 0)
					{
						printf("Usage: Expected '>' and found %s\n",args[aux+1]);
						return -2;
					}
				}
				fileIO(args_aux,args[i+1],args[i+3],1);
				return 1;
			}
			// If '>' is detected, we have output redirection.
			// First we check if the structure given is the correct one,
			// and if that is the case we call the appropriate method
			else if (strcmp(args[i],">") == 0){
				if (args[i+1] == NULL){
					printf("Not enough input arguments\n");
					return -1;
				}
				fileIO(args_aux,NULL,args[i+1],0);
				return 1;
			}
			i++;
		}
		args_aux[i] = NULL;
		launchProg(args_aux);
	}
	return 1;
}

//Main method of our shell
int main(int argc, char *argv[], char ** envp) {
	char line[MAXLINE]; // buffer for the user input
	char * tokens[LIMIT]; // array for the different tokens in the command
	int numTokens;
	char hline[MAXLINE];

	//history array
	history = (char**) malloc(MAXHISTORY*sizeof(char*));
	no_reprint_prmpt = 0; 	// to prevent the printing of the shell after certain methods
	pid = -10; // we initialize pid to an pid that is not possible

	// We call the method of initialization and the welcome screen
	//init();
	currentDirectory = (char*) calloc(1024, sizeof(char));
	
	signal(SIGCHLD, signalHandler_child);
	signal(SIGINT, signalHandler_int);

	welcomeScreen();
    
    // We set our extern char** environ to the environment
	environ = envp;
	// Main loop, where the user input will be read and the prompt will be printed
	while(TRUE){
		// We print the shell prompt if necessary
		if (no_reprint_prmpt == 0) shellPrompt();
		no_reprint_prmpt = 0;
		memset ( line, '\0', MAXLINE );
		fgets(line, MAXLINE, stdin);

		strcpy(hline,line);
		if((tokens[0] = strtok(line," \n\t")) == NULL) continue;
		// If nothing is written, the loop is executed again
		// We read all the tokens of the input and pass it to our commandHandler as the argument
		numTokens = 1;
		while((tokens[numTokens] = strtok(NULL, " \n\t")) != NULL) numTokens++;
		commandHandler(tokens);

		//storing in history
		if(strcmp(tokens[0],"issue")!=0 && strcmp(tokens[0],"history")!=0)
		{  
			history[count] = (char*) malloc(MAXLINE*sizeof(char));
			strcpy(history[count] , hline);
			count = (count+1)%MAXHISTORY;
		}
	}          
	exit(0);
}
