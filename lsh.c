/*
 * Main source code file for lsh shell program
 *
 * You are free to add functions to this file.
 * If you want to add functions in a separate file
 * you will need to modify Makefile to compile
 * your additional functions.
 *
 * Add appropriate comments in your code to make it
 * easier for us while grading your assignment.
 *
 * Submit the entire lab1 folder as a tar archive (.tgz).
 * Command to create submission archive:
      $> tar cvf lab1.tgz lab1/
 *
 * All the best
 */

/*
 * Elena Marzi and Johannes Magnusson
 * 2018-09-?
 */

#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "parse.h"
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/resource.h>

#define TRUE 1
#define FALSE 0
#define MAX_SIZE 1024
#define READ_END 0
#define WRITE_END 1

/*
 * Function declarations
 */
void RunCommand(int, Command *);
void RunPgm(Pgm *, int *pipe_right, Command *cmd);
void stripwhite(char *);
void child_terminated();
void ctrl_c_pressed();
void run_cd(char *);

/* When non-zero (true), this global means the user is done using this program. */
int done = 0;

/*
 * Description: main function. It reads a command from the keyboard and start those programs
 * that the user typed
 */
int main(void)
{
  /*
   * command that holds all information about which command to run
   */
  Command cmd;
  int n;

  /*
   * Register a callback function to be called automatically each
   * time a child process terminates. This allows us to immediatley
   * clean up the child process to avoid zombies
   */
  signal(SIGCHLD, child_terminated);

  /*
   * Register a callback function to be called automatically each
   * time Ctrl-C is pressed. That function does nothing! This makes
   * lsh ignore Ctrl-C.
   */
  signal(SIGINT, ctrl_c_pressed);

  /*
   * this lopp is the main loop in the program. It runs until the user wants to quit the shell
   */
  while (!done) {

    char *line;

    /*
     * readline prints a prompt and then reads and returns a single line of text from the user
     * that is stored in 'line'
     */
    line = readline("> ");

    /*
     * If we've have reached the end of the input stream (if (line == NULL) )-> if line points to null
     * (there is no place in memory where a string could be)
     * the input stream can be the input from the keyboard or from a file.
     * readline returns NULL if stream reaches the end.
     * In C stream is called stdin
     */
    if (!line) {
      /* Encountered EOF at top level */
      /*Set done to 1 (true) --> stop the while loop */
      done = 1;
    }

    else {
      /*
       * Remove leading and trailing whitespace from the line
       * Then, if there is anything left, add it to the history list
       * and execute it.
       * stripwhite is defined further down*/
      stripwhite(line);

      /*
       * line is apointer to a array of char in memory.
       * if string is not empty (if(line[0] != '\0')) -> if the first position in the array
       * is not the end of the string (there is a  place in memory for a string but it is empty!)
       */
      if(*line) {
        /*
         * This is a command to show what the use has typed before.
         * If you want the user to be able to get at the line later, (with C-p for example), you must
         * call add_history() to save the line away in a history list of such lines.
         */
        add_history(line);

        /*
        * execute it
        * TO DO!
        * parse(..) interprets a string the user has typed and create a structure representation in the cmd structure
        * parse is defined in parse.c
         */
        n = parse(line, &cmd);

/*The build in function 'exit'*/
if (strcmp(cmd.pgm->pgmlist[0], "exit") == 0) { /*Checks if the first argument is equal to the string 'exit' */
        //kill(0, SIGKILL); /*If pid (1st argument) is equals 0, then sig (2nd argument) is sent to every process in the process group
        //                  of the calling process. The SIGKILL signal is used to cause immediate program termination */
        exit(0); /*Arg: status code. 0 -> execution succeeded completely */
        done = 1;
}



        RunCommand(n, &cmd);
      }
    }

    /*
     * If (line != NULL) -> if readline return a pointer to a string, free memory.
     * readline allocate memory and main  must free it!
     */
    if(line) {
      /* deallocates the memory previously allocated*/
      free(line);
    }
  }
  return 0;
}

/*
 * Name: RunCommand
 *
 * Description: Runs a Command structure as returned by parse on stdout.
 *
 */
void RunCommand (int n, Command *cmd)
{
  /*
   * this runs all programs and arguments to run and pipe together
   */
  RunPgm(cmd->pgm, NULL, cmd);
}

/*
 * Name: RunPgm
 *
 * Description: Runs a list of Pgm:s.
 * It takes a pointer to a program and runs it.
 * Funktionen tar också en pipe (två integers)
 * Om pipe är NULL är detta "sista" programmet i pipe-kedjan.
 *
 */
void RunPgm (Pgm *p, int *pipe_right, Command *cmd)
{
  /*
   *  if there is no information about the program, do nothing
   */
  if (p == NULL) {
    return;
  }
  else {
    /*
     * pl points to an array of strings where the first one is the name of the program to run and
     * the rest are the arguments to pass to the programm
     * (!! in C a string is an pointer to the first charater to the string  that it means that
     * there is an array in which each element is pointer to a char, this is why there is dubbelpointer
     * -->char * * is like a String[] in Java)
     * p->pgmlist:  is that array given by the parse function. Parser fills the command struct, RunCommand
     * takes the pgm part of the command strunct and sent it to RunPgm.
     */
    char **pl = p->pgmlist;

    /* Check to see if user wants to run a built in command here
     * We can't do this earlier because this is where we get
     * the program name and arguments (the pl array)
     * We can't do this later because built in commands should
     * not be handling pipes, redirection, background ...
     */
    if (strcmp("cd", pl[0]) == 0) {
      /*run the built in command cd, and pass the first parameter */
      run_cd(pl[1]);
      return;
    }







    /* The list is in reversed order (I don't know why- design choice) so print
     * it reversed to get right.
     * p is the pointer to the last program in the chain of pipes. RunPgm calls recursively.
     * (it is like a postorder print is a tree: first print all the rest of the list recurively then print myself)
     *  printing example: [wc -l]
     */

    /* If there is a program to run before this in the pipe chain,
     * create a new pipe and give it to the program to run before this
     */
    int *pipe_left = NULL;
    int new_pipe[2];

    if (p->next != NULL) {
      if (pipe(new_pipe) == -1) {
        fprintf(stderr, "Pipe creation failed!");
        return;
      }
      pipe_left = new_pipe;
      RunPgm(p->next, pipe_left, cmd);
    }

    /*get a copy of the PATH variable*/
    char *path = getenv("PATH");
    char *all_path = strdup(path);
    char  full_path [MAX_SIZE];

    /* get the program name*/
    char *program_name = pl[0];

    /*tokanize */
    char *dir = strtok(all_path, ":");
    int found = FALSE;

    /* as long as there are more path to look in and program is not found*/
    while (dir != NULL && !found) {
	    snprintf(full_path, MAX_SIZE, "%s/%s", dir, program_name);

      /*if file exists print its location*/
	    if(access(full_path, R_OK | X_OK) != -1) {
        pid_t pid = fork();
        if(pid < 0){
          fprintf(stderr, "Fork failed");
          return;
        }/*if this is the child process*/
        else if(pid ==0) {
          if (pipe_right != NULL) {
            /* Redirect stdout, if a pipe was sent in from the outside */
            close(STDOUT_FILENO);       // Close stdout
            dup(pipe_right[WRITE_END]);    // Replace stdout with a copy of the received pipe
            close(pipe_right[READ_END]);   // Close the pipe
            close(pipe_right[WRITE_END]);
          }
          else if (cmd->rstdout != NULL) {
            /* Redirect stdout if this is the program to the right and if a filename was given */
            int redirect_stdout = open(cmd->rstdout, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
            /* If file could not be created or opened for writing */
            if (redirect_stdout == -1) {
              fprintf(stderr, "Could not write to file %s\n", cmd->rstdout);
              return;
            }
            dup2(redirect_stdout, 1);
            close(redirect_stdout);
          }

          if (pipe_left != NULL) {
            /* Redirect stdin, if a new pipe was sent to the program before this */
            close(STDIN_FILENO);        // Close stdin
            dup(pipe_left[READ_END]);     // Replace stdin with a copy of the new pipe
            close(pipe_left[WRITE_END]);  // Close the pipe
            close(pipe_left[READ_END]);
          }
          else if (cmd->rstdin != NULL) {
            /* Redirect stdin if this is the program to the left and if a filename was given */
            int redirect_stdin = open(cmd->rstdin, O_RDONLY);
            /* If file could not be created or opened for writing */
            if (redirect_stdin == -1) {
              fprintf(stderr, "Could not read from file %s\n", cmd->rstdin);
              return;
            }
            dup2(redirect_stdin, 0);
            close(redirect_stdin);
          }

          /* background processes must ALSO ignore Ctrl-C ! */
          if (cmd->bakground) {
            /* Each background process gets its own new process group */
            setpgid(pid, 0);
          }

          /*execute a program*/
          execvp(full_path, pl);
        }
        else{
          /* If we have created a new pipe, close it! */
          if (pipe_left != NULL) {
            close(pipe_left[0]);
            close(pipe_left[1]);
          /*check if background task*/
          }
          if (!cmd->bakground){
              wait(NULL);
          }
        }
		    found = TRUE;
      }

      dir = strtok (NULL, ":") ;
    }
    if (! found){
      printf("Command not found: %s\n", program_name);
    }
    free(all_path);


  }
}

/*
 * Name: stripwhite
 *
 * Description: Strip whitespace from the start and end of STRING.
 */
void stripwhite (char *string)
{
  register int i = 0;
/*
 * isspace(int c) checks whether the passed character is white-space.
 * This function returns a non-zero  value(true) if c is a white-space character
 * else, zero (false)
 * Here: move i foward as long as there are spaces. After the loop, i is the index of hte first non-space
 * character.
 */
  while (isspace( string[i] )) {
    i++;
  }

 /*
  * i is the index of the array
  * if (i > 0) ->if i has moved foward more than zero steps, copy the string from string+i to string.
  * That is means that we move the relevant part of the string to the beginning of the string
  * [_ _ _ cat -l] --> [cat -l]
  */
  if (i) {
    strcpy (string, string + i);
  }

  /*
   * Now we have a command in string whiout leading whitespace.
   * Here are trailing white space removed.
   *  strlen return length of the string. Result of the function get substahacted by 1.
   * t.e.x strlen return 12: it means that there is 12 character in the string.
   * the first index is always 0. The last is always length -1. (11).
   * i = 11 in the exampel.
   * in the loop: as long as there is a whitespace at position i, substahact i with 1
   */
  i = strlen( string ) - 1;
  while (i > 0 && isspace (string[i])) {
    i--;
  }
 /*
  * now i is the index of the last non space character.
  * i increased by 1 and set an string terminator in that position
  */
  string [++i] = '\0';
}


/*
 * Name: child_terminated
 *
 * Is called automatically each time a child process terminates
 */
void child_terminated() {
		int exit_status;
    pid_t pid;

    /*
     * loop as long as it finds more terminated child processes
     */
    do {
      /*
       * Check if there is a child process that is terminated
       */
      pid = wait3(&exit_status, WNOHANG, NULL);

      if (pid > 0) {
        /*
         * Here we could also print the exit code which
         * is in some 8 bits of the exit_status
         */
        fprintf(stdout, "Process %d exited\n", pid);
      }
    } while (pid > 0);
}


/*
 * Name: ctrl_c_pressed
 *
 * Is called automatically each time Ctrl-C is pressed.
 * This function does nothing, so Ctrl-C is ignored!
 */
void ctrl_c_pressed() {
}

/*
 * Name: run_cd
 *
 * The built in command cd
 */
void run_cd(char *directory) {
  if (directory == NULL) {
    /* cd without an arguments changes directory to the user's HOME directory */
    chdir(getenv("HOME"));
  }
  else {
    /* try to change current directory */
    int result = chdir(directory);

    if (result == -1) {
      /* something went wrong */
      fprintf(stderr, "Could not change directory to \"%s\"\n", directory);
    }
  }
}
