/***************************************************************************
 *	Title: Runtime environment 
 * -------------------------------------------------------------------------
 *		Purpose: Runs commands
 *		Author: Stefan Birrer
 *		Version: $Revision: 1.3 $
 *		Last Modification: $Date: 2009/10/12 20:50:12 $
 *		File: $RCSfile: runtime.c,v $
 *		Copyright: (C) 2002 by Stefan Birrer
 ***************************************************************************/
/***************************************************************************
 *	ChangeLog:
 * -------------------------------------------------------------------------
 *		$Log: runtime.c,v $
 *		Revision 1.3	2009/10/12 20:50:12	jot836
 *		Commented tsh C files
 *
 *		Revision 1.2	2009/10/11 04:45:50	npb853
 *		Changing the identation of the project to be GNU.
 *
 *		Revision 1.1	2005/10/13 05:24:59	sbirrer
 *		- added the skeleton files
 *
 *		Revision 1.6	2002/10/24 21:32:47	sempi
 *		final release
 *
 *		Revision 1.5	2002/10/23 21:54:27	sempi
 *		beta release
 *
 *		Revision 1.4	2002/10/21 04:49:35	sempi
 *		minor correction
 *
 *		Revision 1.3	2002/10/21 04:47:05	sempi
 *		Milestone 2 beta
 *
 *		Revision 1.2	2002/10/15 20:37:26	sempi
 *		Comments updated
 *
 *		Revision 1.1	2002/10/15 20:20:56	sempi
 *		Milestone 1
 *
 ***************************************************************************/
#define __RUNTIME_IMPL__

/************System include***********************************************/
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/param.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

/************Private include**********************************************/
#include "runtime.h"
#include "io.h"

/************Defines and Typedefs*****************************************/
/*	#defines and typedefs should have their names in all caps.
 *	Global variables begin with g. Global constants with k. Local
 *	variables should be in all lower case. When initializing
 *	structures and arrays, line everything up in neat columns.
 */

/************Global Variables*********************************************/

#define NBUILTINCOMMANDS (sizeof BuiltInCommands / sizeof(char*))

typedef struct bgjob_l
{
	pid_t pid;
	struct bgjob_l* next;
	char* name;
	int jid;
} bgjobL;

/* the pids of the background processes */
bgjobL *bgjobs = NULL;

/************Function Prototypes******************************************/
/* run command */
static void
RunCmdFork(commandT*, bool);
/* runs an external program command after some checks */
static void
RunExternalCmd(commandT*, bool);
/* resolves the path and checks for exutable flag */
static bool
ResolveExternalCmd(commandT*);
/* forks and runs a external program */
static void
Exec(commandT*, bool);
/* runs a builtin command */
static void
RunBuiltInCmd(commandT*);
/* checks whether a command is a builtin command */
static bool
IsBuiltIn(char*);
/* checks whether a given file path exists */
static int
fileExists(const char * filename);
/* returns the full filepath for a given file name */
char*
getFullPath(char * filename);
/* converts argv[0] of a command to just the command name (e.g. /bin/ls -> ls) */
void
convertFirstArgToCommandName(commandT* cmd);
void
ChangeStdIn(char* filePath);
void
ChangeStdInToFid(int fid);
void
ChangeStdOut(char* filePath);
void
ChangeStdOutToFid(int fid);
int
AddJob(int pid, commandT* cmd);
bgjobL*
CreateJob(int pid, commandT* cmd);
void
RemoveJob(int pid);
int
GetNextJobNumber();
bgjobL*
GetJob(int jid);
bgjobL*
GetLastJob();
/************External Declaration*****************************************/

/**************Implementation***********************************************/


/*
 * RunCmd
 *
 * arguments:
 *	 commandT *cmd: the command to be run
 *
 * returns: none
 *
 * Runs the given command.
 */
void
RunCmd(commandT* cmd) {
	if (*cmd->argv[cmd->argc - 1] == '&') {
		RunCmdBg(cmd);
		return;
	}
	
	int i;
	for (i = 0; i < cmd->argc; ++i) {
		if (*cmd->argv[i] == '>') {
			cmd->argv[i] = 0;
			if (i != cmd->argc) {
				RunCmdRedirOut(cmd, cmd->argv[i + 1]);
			} else {
				// error: no file was specified
			}
			return;
		}
	}
	
	for (i = 0; i < cmd->argc; ++i) {
		if (*cmd->argv[i] == '<') {
			cmd->argv[i] = 0;
			if (i != cmd->argc) {
				RunCmdRedirIn(cmd, cmd->argv[i + 1]);
			} else {
				// error: no file was specified
			}
			return;
		}
	}
	
	RunCmdFork(cmd, TRUE);
} /* RunCmd */


/*
 * RunCmdFork
 *
 * arguments:
 *	 commandT *cmd: the command to be run
 *	 bool fork: whether to fork
 *
 * returns: none
 *
 * Runs a command, switching between built-in and external mode
 * depending on cmd->argv[0].
 */
void
RunCmdFork(commandT* cmd, bool fork) {
	if (cmd->argc <= 0)
		return;
		
	// If there's a |, call RunCmdPipe with the left and right halves
		
	if (IsBuiltIn(cmd->argv[0])) {
		RunBuiltInCmd(cmd);
	} else {
		RunExternalCmd(cmd, fork);
	}
} /* RunCmdFork */


/*
 * RunCmdBg
 *
 * arguments:
 *	 commandT *cmd: the command to be run
 *
 * returns: none
 *
 * Runs a command in the background.
 */
void
RunCmdBg(commandT* cmd) {
	int cpid;
	
	int oldStdIn = dup(STDIN_FILENO);
	// Set stdin to /dev/null
	ChangeStdIn("/dev/null");
	
	if ((cpid = fork()) < 0){
		perror("fork failed");
	} else {
		sigset_t x;
		sigemptyset (&x);
		sigaddset(&x, SIGCHLD);
		sigprocmask(SIG_BLOCK, &x, NULL);
	
		if (cpid == 0) { // child
			cmd->argc--;
			cmd->argv[cmd->argc] = 0;
			
			setpgid(0, 0);
			sigprocmask(SIG_UNBLOCK, &x, NULL);
			RunExternalCmd(cmd, FALSE);
		} else { // parent
			printf("[%i] %i\n", AddJob(cpid, cmd), cpid);
			sigprocmask(SIG_UNBLOCK, &x, NULL);
			ChangeStdInToFid(oldStdIn);
		}
	}
} /* RunCmdBg */

/*
 * RunCmdPipe
 *
 * arguments:
 *	 commandT *cmd1: the commandT struct for the left hand side of the pipe
 *	 commandT *cmd2: the commandT struct for the right hand side of the pipe
 *
 * returns: none
 *
 * Runs two commands, redirecting standard output from the first to
 * standard input on the second.
 */
void
RunCmdPipe(commandT* cmd1, commandT* cmd2) {
	// save file descripts
	// create our pipe
	// set file descriptors to pipe
	// fork once for cmd1
	//		IN CHILD:
	//			close the unused end of the pipe (read end)
	//			RunExternalCmd(unforked)
	// 		IN PARENT:
	//			fork AGAIN for cmd2
	//			IN CHILD:
	//				close the unused end of the pipe (write end)
	// 				RunCmdFork(unforked);
	//			IN PARENT:
	// 				restore file descriptors
	//				wait on cmd1 child
} /* RunCmdPipe */


/*
 * RunCmdRedirOut
 *
 * arguments:
 *	 commandT *cmd: the command to be run
 *	 char *file: the file to be used for standard output
 *
 * returns: none
 *
 * Runs a command, redirecting standard output to a file.
 */
void
RunCmdRedirOut(commandT* cmd, char* file) {
	int oldStdOut = dup(STDOUT_FILENO);
	ChangeStdOut(file);
	RunExternalCmd(cmd, TRUE);
	ChangeStdOutToFid(oldStdOut);
} /* RunCmdRedirOut */


/*
 * RunCmdRedirIn
 *
 * arguments:
 *	 commandT *cmd: the command to be run
 *	 char *file: the file to be used for standard input
 *
 * returns: none
 *
 * Runs a command, redirecting a file to standard input.
 */
void
RunCmdRedirIn(commandT* cmd, char* file) {
	int oldStdIn = dup(STDIN_FILENO);
	ChangeStdIn(file);
	RunExternalCmd(cmd, TRUE);
	ChangeStdInToFid(oldStdIn);
}	/* RunCmdRedirIn */


/*
 * RunExternalCmd
 *
 * arguments:
 *	 commandT *cmd: the command to be run
 *	 bool fork: whether to fork
 *
 * returns: none
 *
 * Tries to run an external command.
 */
static void
RunExternalCmd(commandT* cmd, bool fork) {
	if (ResolveExternalCmd(cmd))
		Exec(cmd, fork);
}	/* RunExternalCmd */


/*
 * ResolveExternalCmd
 *
 * arguments:
 *	 commandT *cmd: the command to be run
 *
 * returns: bool: whether the given command exists
 *
 * Determines whether the command to be run actually exists.
 */
static bool
ResolveExternalCmd(commandT* cmd) {
	char* fullPath = getFullPath(cmd->name);
	
	if (fullPath != NULL) {
		cmd->name = fullPath;
		
		return TRUE;
	}
	
	free(fullPath);
	return FALSE;
} /* ResolveExternalCmd */


/*
 * Exec
 *
 * arguments:
 *	 commandT *cmd: the command to be run
 *	 bool forceFork: whether to fork
 *
 * returns: none
 *
 * Executes a command.
 */
static void
Exec(commandT* cmd, bool forceFork) {
	int cpid;
	
	if(!forceFork) {
		convertFirstArgToCommandName(cmd);
		execv(cmd->name, cmd->argv);
		perror("exec failed");
	} else {
		sigset_t x;
		sigemptyset (&x);
		sigaddset(&x, SIGCHLD);
		sigprocmask(SIG_BLOCK, &x, NULL);
	
		if ((cpid = fork()) < 0){
			perror("fork failed");
		} else {
			if (cpid == 0) { // child
				setpgid(0, 0);
				convertFirstArgToCommandName(cmd);
				sigprocmask(SIG_UNBLOCK, &x, NULL);
				execv(cmd->name, cmd->argv);
				perror("exec failed");
			} else { // parent
				fgCid = cpid;
				int* stat = 0;
				sigprocmask(SIG_UNBLOCK, &x, NULL);
				waitpid(cpid, stat, 0);
				fgCid = 0;
			}
		}
	}
	
	free(cmd->name);
} /* Exec */


/*
 * convertFirstArgToCommandName
 *
 * arguments:
 *   commandT* cmd: command to convert
 *
 * returns: none
 *
 * Converts the first element in the argument matrix (argv[0]) from
 * a command path to just the name of the command.
 *
 * e.g:
 * 	/bin/ls -> ls
 *	ls -> ls
 *	/trol -> trol
 */
void convertFirstArgToCommandName(commandT* cmd) {
	char* command = cmd->argv[0];
	
	int starting = -1;
	int i;
	for (i = strlen(command); i >= 0; --i) {
		if (command[i] == '/') {
			starting = i;
			break;
		}
	}
	
	if (starting != -1) {
		char* commandName = malloc((strlen(command) - starting + 1) * sizeof(char));
		memcpy(commandName, command + (starting + 1) * sizeof(char), (strlen(command) - starting + 1) * sizeof(char));
		free(cmd->argv[0]);
		cmd->argv[0] = commandName;
	}
}

/*
 * IsBuiltIn
 *
 * arguments:
 *	 char *cmd: a command string (e.g. the first token of the command line)
 *
 * returns: bool: TRUE if the command string corresponds to a built-in
 *								command, else FALSE.
 *
 * Checks whether the given string corresponds to a supported built-in
 * command.
 */
static bool
IsBuiltIn(char* cmd) {
	if (strcmp(cmd, "echo") == 0 ||
		strcmp(cmd, "exit") == 0 ||
		strcmp(cmd, "cd") == 0 ||
		strcmp(cmd, "bg") == 0 ||
		strcmp(cmd, "jobs") == 0 ||
		strcmp(cmd, "fg") == 0) {
		return TRUE;
	}
	
	// Look for a VAR=var type of command.
	int i;
	for (i = 0; i < strlen(cmd); ++i) {
		if (cmd[i] == '=') {
			return TRUE;
		}
	}
	
	return FALSE;
} /* IsBuiltIn */


/*
 * RunBuiltInCmd
 *
 * arguments:
 *	 commandT *cmd: the command to be run
 *
 * returns: none
 *
 * Runs a built-in command.
 */
static void
RunBuiltInCmd(commandT* cmd) {
	if (strcmp(cmd->name, "echo") == 0) {
		int i;
		for (i = 1; i < cmd->argc; ++i) {
			// If the parameter is not an environment variable
			if (cmd->argv[i][0] != '$') {
				printf("%s ", cmd->argv[i]);
			} else {
				char* varName = malloc(strlen(cmd->argv[i])*sizeof(char));
				memcpy(varName, cmd->argv[i] + sizeof(char), strlen(cmd->argv[i]) * sizeof(char));
				printf("%s ", getenv(varName));
				free(varName);
			}
			
		}
		printf("\n");
		return;
	}
	
	if (strcmp(cmd->name, "exit") == 0) {
		return;
	}
	
	if (strcmp(cmd->name, "cd") == 0) {
		int res = 0;
		if (cmd->argc > 1) {
			res = chdir(cmd->argv[1]);
		} else {
			res = chdir(getenv("HOME"));
		}
		// 0: success, -1: failure
		if (res != 0) {
			perror("cd failed");
		}
		return;
	}
	
	if (strcmp(cmd->name, "bg") == 0) {
		bgjobL* curr = bgjobs;
		
		if (curr == NULL) {
			// error: no bg jobs
			return;
		}
			
		if (cmd->argc == 1) {			
			curr = GetLastJob();
		} else {
			curr = GetJob(atoi(cmd->argv[1]));
			if (curr == NULL) {
				// error: job not found
				return;
			}
		}
		
		kill(curr->pid, SIGCONT);
		return;
	}
	
	if (strcmp(cmd->name, "jobs") == 0) {
		bgjobL* curr = bgjobs;
		
		while (curr != NULL) {
			printf("[%i]\tRunning\t\t%s\n", curr->jid, curr->name);
			curr = curr->next;
		}
		return;
	}
	
	if (strcmp(cmd->name, "fg") == 0) {
		bgjobL* curr = bgjobs;
		
		if (curr == NULL) {
			// error: no bg jobs
			return;
		}
			
		if (cmd->argc == 1) {			
			curr = GetLastJob();
		} else {
			curr = GetJob(atoi(cmd->argv[1]));
			if (curr == NULL) {
				// error: job not found
				return;
			}
		}
		
		tcsetpgrp(curr->pid, STDIN_FILENO);
		fgCid = curr->pid;
		RemoveJob(curr->jid);
		int* stat = 0;
		waitpid(fgCid, stat, 0);
		fgCid = 0;
		return;
	}
	
	// This is a VAR=var thing if we reach here.
	int i;
	int foundPos = -1;
	for (i = 0; strlen(cmd->name); ++i) {
		if (cmd->name[i] == '=') {
			foundPos = i;
			break;
		}
	}
	
	char* varName = malloc((foundPos + 1) * sizeof(char));
	memcpy(varName, cmd->name, foundPos*sizeof(char));
	varName[foundPos] = '\0';
	char* var = malloc((strlen(cmd->name) - foundPos + 1) * sizeof(char));
	memcpy(var, cmd->name + (foundPos + 1) * sizeof(char), (strlen(cmd->name) - foundPos + 1) * sizeof(char));
	setenv(varName, var, 1);
	free(varName);
	free(var);
	
} /* RunBuiltInCmd */


/*
 * CheckJobs
 *
 * arguments: none
 *
 * returns: none
 *
 * Checks the status of running jobs.
 */
void
CheckJobs() {
} /* CheckJobs */

char*
getCurrentWorkingDir() {
	char* path = malloc(MAXPATHLEN*sizeof(char*));
	return getcwd(path, MAXPATHLEN);
}

/*
 * fileExists
 *
 * arguments:
 *   char* filename: file to check if it exists
 */
int fileExists(const char * filename) {
	FILE* file;
	if ((file = fopen(filename, "r"))) {
		fclose(file);
		return TRUE;
	}
	return FALSE;
}

/*
 * getFullPath
 *
 * arguments:
 *   char* filename: file to look up the full path for
 */
char* getFullPath(char* filename)  {

	char* paths = getenv("PATH");
	char* result = malloc(MAXPATHLEN*sizeof(char*));
	bool found = FALSE;

	// If the file name is an absolute path.
	if (filename[0] == '/') {
		// Just look it up based on the provided path.
		if (fileExists(filename)) {
			strcpy(result, filename);
			found = TRUE;
		}
	} else {
		// Otherwise see if it exists in the home directory.
		char* home = getenv("HOME");
		char* homePath = malloc(MAXPATHLEN*sizeof(char*));
		strcpy(homePath, home);
		strcat(homePath, "/");
		strcat(homePath, filename);
		if (fileExists(homePath)) {
			strcpy(result, homePath);
			found = TRUE;
		} else {
			// Otherwise see if it exists in the current directory.
			char* workingDir = getCurrentWorkingDir();
			char* fullWorkingDir = malloc(MAXPATHLEN*sizeof(char*));
			strcpy(fullWorkingDir, workingDir);
			strcat(fullWorkingDir, "/");
			strcat(fullWorkingDir, filename);
			if (fileExists(fullWorkingDir)) {
				strcpy(result, fullWorkingDir);
				found = TRUE;
			} else {
				// Otherwise see if it exists in any of the folders in our path.
				char* pathCopy = malloc(MAXPATHLEN*sizeof(char*));
				strcpy(pathCopy, paths);
				char* path = strtok(pathCopy, ":");
				while (path != NULL) {
					char* fullPath = malloc(MAXPATHLEN*sizeof(char*));
					strcpy(fullPath, path);
					strcat(fullPath, "/");
					strcat(fullPath, filename);
					if (fileExists(fullPath)) {
						strcpy(result, fullPath);
						found = TRUE;
					}
					path = strtok(NULL, ":");
					free(fullPath);
				}
				free(pathCopy);
			}
			free(fullWorkingDir);
			free(workingDir);
		}
		free(homePath);
	}
	
	if (found) {
		return result;
	} else {
		// We'll always be line 1 because we don't support reading from files. If we did, this would need a new parameter (line #);
		strcpy(result, "line 1: ");
		strcat(result, filename);
		PrintPError(result);
		free(result);
		return NULL;
	}
}

void ChangeStdIn(char* filePath) {
	int fid = open(filePath, O_RDONLY);
	ChangeStdInToFid(fid);
	close(fid);
}

void ChangeStdInToFid(int fid) {
	dup2(fid, 0);
}

void ChangeStdOut(char* filePath) {
	int fid = open(filePath, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	ChangeStdOutToFid(fid);
	close(fid);
}

void ChangeStdOutToFid(int fid) {
	dup2(fid, 1);
}

int AddJob(int pid, commandT* cmd) {
	bgjobL* curr = bgjobs;
	if (curr == NULL) {
		bgjobs = CreateJob(pid, cmd);
		return bgjobs->jid;
	}
	
	while (curr->next != NULL) {
		curr = curr->next;
	}
	
	curr->next = CreateJob(pid, cmd);
	
	return curr->next->jid;
}

bgjobL* CreateJob(int pid, commandT* cmd) {
	bgjobL* job = malloc(sizeof(bgjobL));
	job->pid = pid;
	job->next = NULL;
	job->name = malloc(sizeof(*cmd->argv) + sizeof(char) * cmd->argc);
	strcpy(job->name, cmd->argv[0]);
	int i;
	for (i = 1; i < cmd->argc; ++i) {
		strcat(job->name, " ");
		strcat(job->name, cmd->argv[i]);
	}
	job->jid = GetNextJobNumber();
	return job;
}

void RemoveJob(int jid) {
	bgjobL* curr = bgjobs;
	if (curr == NULL) {
		return;
	}
	
	if (curr->jid == jid) {
		bgjobL* next = curr->next;
		free(curr);
		bgjobs = next;
		return;
	}
	
	while (curr->next != NULL && curr->next->jid != jid) {
		curr = curr->next;
	}
	
	if (curr->next == NULL) {
		return;
	}
	
	bgjobL* jobToEnd = curr->next;
	curr->next = jobToEnd->next;
	free(jobToEnd->name);
	free(jobToEnd);
}

int GetNextJobNumber() {
	bgjobL* curr = bgjobs;
	
	if (curr == NULL) {
		return 1;
	}
	
	while (curr->next != NULL) {
		curr = curr->next;
	}
	
	return curr->jid + 1;
}

bgjobL*
GetJob(int jid) {
	bgjobL* curr = bgjobs;
	
	while (curr != NULL) {
		curr = curr->next;
		if (curr->jid == jid) {
			break;
		}
	}

	return curr;
}

bgjobL*
GetLastJob() {
	bgjobL* curr = bgjobs;
	while (curr->next != NULL) {
		curr = curr->next;
	}
	return curr;
}