#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
// You may not include additional header files.

#include "pish_history.h"
#define MAX_COMMAND_LENGTH 256

/*
 * Script mode flag. If set to 0, the shell reads from stdin. If set to 1,
 * the shell reads from a file from argv[1].
 */
static int script_mode = 0;

/*
 * Prints a prompt IF NOT in script mode (see script_mode global flag).
 */
void prompt(void) {
    if (!script_mode) {
        char *working_dir = getcwd(NULL, 0);
        struct passwd *user = getpwuid(getuid());
#ifdef PISH_AUTOGRADER
        printf("%s@pish %s$\n", user->pw_name, working_dir);
#else
        printf("\e[0;35m%s@pish \e[0;34m%s\e[0m$ ", user->pw_name, working_dir);
#endif
        fflush(stdout);
        free(working_dir);
    }
}

void usage_error(void) {
    fprintf(stderr, "pish: Usage error\n");
    fflush(stderr);
}

/*
 * Break down a line of input by whitespace, and put the results into
 * a struct pish_arg to be used by other functions.
 *
 * @param command   A char buffer containing the input command
 * @param arg       Broken down args will be stored here
 */
void parse_command(char *command, struct pish_arg *arg) {
    // TODO
    arg->argc = 0;

    //strip the trailing newline
    command[strcspn(command, "\n")] = '\0';

    char *token = strtok(command, " \t");
    while (token != NULL && arg->argc < MAX_ARGC - 1) {
       arg->argv[arg->argc++] = token;
       token = strtok(NULL, " \t");
    }
    arg->argv[arg->argc] = NULL; // NULL-terminate for execvp
}

/*
 * Run a command.
 *
 * Built-in commands are handled internally by the pish program.
 * Otherwise, use fork/exec to create child processes to run the program.
 *
 * If the command is empty, do nothing.
 * If NOT in script mode, add the command to history file.
 */
void run(struct pish_arg *arg) {
    // TODO
    //If empty command, then do nothing
    if (arg->argc == 0) {
       return;
    }

    //Write to history, only in interactive mode
    if (!script_mode) {
       add_history(arg);
    }

    char *cmd = arg->argv[0];

    //exit
    if (strcmp(cmd, "exit") == 0) {
        if (arg->argc != 1) {
           usage_error();
           return;
        }
        exit(EXIT_SUCCESS);
    }

    //change directory (cd)
	else if (strcmp(cmd, "cd") == 0) {
	    if (arg->argc != 2) {
	        usage_error();
	        return;
	    }
	    static char prev_dir[1024] = {'\0'};
	    char curr_dir[1024];
	    getcwd(curr_dir, sizeof(curr_dir));
	
	    if (strcmp(arg->argv[1], "-") == 0) {
	        // If no previous cd, print and stay in current dir
	        if (prev_dir[0] == '\0') {
	            printf("%s\n", curr_dir);
	        } else {
	            printf("%s\n", prev_dir);
	            char tmp[1024];
	            strncpy(tmp, prev_dir, sizeof(tmp));
	            strncpy(prev_dir, curr_dir, sizeof(prev_dir));
	            if (chdir(tmp) != 0) {
	                perror("cd");
	            }
	        }
	    } else {
	        strncpy(prev_dir, curr_dir, sizeof(prev_dir));
	        if (chdir(arg->argv[1]) != 0) {
	            perror("cd");
	        }
	    }
	}

    //history
    else if (strcmp(cmd, "history") == 0) {
       if (arg->argc == 1) {
          print_history();
       } else if (arg->argc == 2 && strcmp(arg->argv[1], "-c") == 0) {
          clear_history();
       } else {
          usage_error();
       }
    }

    //forc + exec
    else{
       pid_t pid = fork();
       if (pid < 0) {
          perror("fork");
       } else if (pid == 0) {
          //child process
          execvp(cmd, arg->argv);
          perror(cmd);  //only reached if execvp failed
          exit(EXIT_FAILURE);
       } else {
          //parent proccess
          wait(NULL);
       }
    }
}

/*
 * The main loop of pish. Repeat until the "exit" command or EOF:
 * 1. Print the prompt
 * 2. Read command from fp (which can be stdin or a script file)
 * 3. Execute the command
 *
 * Assume that each command never exceeds MAX_COMMAND_LENGTH-1 chars.
 */
int pish(FILE *fp) {
    // TODO
    char command[MAX_COMMAND_LENGTH];
    struct pish_arg arg;

    while (1) {
       prompt();
       if (fgets(command, MAX_COMMAND_LENGTH, fp) == NULL) {
          exit(EXIT_SUCCESS);
       }
       parse_command(command, &arg);
       run(&arg);
    }
    return 0;
}

/*
 * The entry point of the pish program.
 *
 * - If the program is called with no additional arguments (like "./pish"),
 *   process commands from stdin.
 * - If the program is called with one additional argument
 *   (like "./pish script.sh"), process commands from the file specified by the
 *   additional argument under script mode.
 * - If there are more arguments, call usage_error() and exit with status 1.
 */
int main(int argc, char *argv[]) {
    // TODO
    if (argc > 2) {
    	usage_error();
	return EXIT_FAILURE;
    }

    if (argc == 2) {
    	script_mode = 1;
	FILE *fp = fopen(argv[1], "r");
	if (fp == NULL) {
	   perror(argv[1]);
	   return EXIT_FAILURE;
	}
	int result = pish(fp);
	fclose(fp);
	return result;
    }

    return pish(stdin);
}
