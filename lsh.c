/*
 *
 * Submit the entire lab1 folder as a tar archive (.tgz).
 * Command to create submission archive:
      $> tar cvf lab1.tgz lab1/
 */

/*
 * Elena Marzi and Johannes Magnusson
 * 2018-09-30
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

void RunCommand(int, Command *);
int RunPgm(Pgm *, int *pipe_right, Command *cmd);
void stripwhite(char *);
void redirect_stdout(int *pipe_right, char *rstdout);
void redirect_stdin(int *pipe_left, char *rstdin);
void child_terminated();
void ctrl_c_pressed();
void run_cd(char *);

/* When non-zero (true), this global means the user is done using this program. */
int done = 0;

/* When TRUE, lsh is in the phase of waiting for foreground process(-es) to finish.
 * Then SIGCHLD should be ignored, so it doesn't disturb the waiting */
int waiting = FALSE;

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
   * time Ctrl-C is pressed.
   */
  signal(SIGINT, ctrl_c_pressed);

  while (!done) {
    char *line;
    line = readline("> ");

    /*
     * readline returns NULL if stream reaches the end.
     */
    if (!line) {
      /*Set done to 1 (true) --> stop the while loop */
      done = 1;
    }

    else {
      /*
       * Remove leading and trailing whitespace from the line
       */
      stripwhite(line);

      /*
       * if string is not empty
       */
      if(line[0] != '\0') {
        add_history(line);

        /*
        * parse(..) interprets a string the user has typed and create a structure representation in the cmd structure
        */
        n = parse(line, &cmd);

        /*The build in function 'exit'*/
        /*Checks if the first argument is equal to the string 'exit' */
        if (strcmp(cmd.pgm->pgmlist[0], "exit") == 0) { 
          exit(0);
        }

        RunCommand(n, &cmd);
      }
    }

    if(line != NULL) {
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
  
  /*check if foreground task*/
  if (!cmd->bakground){
    int wstatus, i;
    waiting = TRUE;

    /* Wait for the recently started child process(-es) to exit */

    /* First count the number of processes to wait for */
    int count = 0;
    Pgm *wait_pgm = cmd->pgm;
    while (wait_pgm != NULL) {
      wait_pgm = wait_pgm->next;
      ++count;
    }

    /* Then wait the right number of times */
    for (i = 0; i < count; i++) {
      pid_t pid = waitpid(0, &wstatus, 0);
    }

    waiting = FALSE;

    /* Give child processes a chance to get cleaned up */
    child_terminated();
  }
}

/*
 * Name: RunPgm
 *
 * Description: Runs a list of Pgm:s.
 * It takes a pointer to a program and runs it.
 * It also takes an array of file descriptors for a pipe.
 * If pipe is NULL, this is the program to the far right of the pipe chain
 */
int RunPgm (Pgm *p, int *pipe_right, Command *cmd)
{
  /*
   *  if there is no information about the program, do nothing
   */
  if (p == NULL) {
    return FALSE;
  }

  /*
    * pl points to an array of strings where the first one is the name of the program to run and
    * the rest are the arguments to pass to the program
    */
  char **pl = p->pgmlist;

  /*
    * Check to see if user wants to run a built in command here, but only
    * really run it if this is the ONLY program in a pipe chain
    */
  if (strcmp("cd", pl[0]) == 0 && pipe_right == NULL && p->next == NULL) {
    /*run the built in command cd, and pass the first parameter */
    run_cd(pl[1]);
    return TRUE;
  }


  /*
    * p is the pointer to the last program in the chain of pipes. RunPgm calls recursively.
    */

  /* If there is a program to run before this in the pipe chain,
    * create a new pipe and give it to the program to run before this
    */
  int *pipe_left = NULL;
  int new_pipe[2];

  /*if there is at least one more program to the left of this program, create a pipe*/
  if (p->next != NULL) {
    /*if the creation of a new pipe fails*/ 
    if (pipe(new_pipe) == -1) {
      fprintf(stderr, "Pipe creation failed!");
      return FALSE;
    }
    pipe_left = new_pipe;
    /*if the program to the left fails, close the pipe without running the current program*/
    if (!RunPgm(p->next, pipe_left, cmd)) {
      close(pipe_left[READ_END]);
      close(pipe_left[WRITE_END]);
      return FALSE;
    }
  }

  /* get the program name*/
  char *program_name = pl[0];

  /* Create a child process */
  pid_t pid = fork();

  if(pid < 0){
    fprintf(stderr, "Fork failed");
    return FALSE;
  }
  /*if this is the child process*/
  else if(pid == 0) {
    /*output from this program might be redirected*/
    redirect_stdout(pipe_right, cmd->rstdout);
    /*input to this program might be redirected*/
    redirect_stdin(pipe_left, cmd->rstdin);

    /* background processes must ignore Ctrl-C */
    if (cmd->bakground) {
      /* Each background process gets its own new process group */
      setpgid(pid, 0);
    }

    /* execute a built-in command in a pipe chain */
    if (strcmp("cd", program_name) == 0) {
      run_cd(pl[1]);
      exit(0);
    }

    /*execute a program, find it in the PATH variable. If something is wrong returns -1*/
    if (execvp(program_name, pl) == -1) {
      fprintf(stderr, "Command not found: %s\n", program_name);
      /*exit the child process*/
      exit(1);
    }
  }
  else {
    /* If we have created a new pipe, close it! */
    if (pipe_left != NULL) {
      close(pipe_left[READ_END]);
      close(pipe_left[WRITE_END]);
    }
  }
  
  return TRUE;
}

/*
 * Name: stripwhite
 *
 * Description: Strip whitespace from the start and end of STRING.
 */
void stripwhite (char *string)
{
  register int i = 0;

  while (isspace( string[i] )) {
    i++;
  }

  if (i) {
    strcpy (string, string + i);
  }

  i = strlen( string ) - 1;
  while (i > 0 && isspace (string[i])) {
    i--;
  }

  string [++i] = '\0';
}

/*
 * This function redirects stdout either to a pipe or to a named file
 */
void redirect_stdout(int *pipe_right, char *rstdout) {
  if (pipe_right != NULL) {
    /* Redirect stdout (close it) to a pipe*/
    close(STDOUT_FILENO);
    /*connect the write end of the pipe to stdout for this process*/
    dup(pipe_right[WRITE_END]);    
    close(pipe_right[READ_END]);
    close(pipe_right[WRITE_END]);
  }
  else if (rstdout != NULL) {
    /* Redirect stdout to a file if this is the program to the right.
    *  Open a file for writing, truncate if the file exists, create if not.
    *  Set the security bits*/
    int redirect_stdout = open(rstdout, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
    /* If file could not be created or opened for writing */
    if (redirect_stdout == -1) {
      fprintf(stderr, "Could not write to file %s\n", rstdout);
      exit(1);
    }
    /*connect the created file to the redirected stdout for this porcess*/
    dup2(redirect_stdout, 1);
    close(redirect_stdout);
  }
}

/*
 * This function redirects stdin either to a pipe or to a named file
 */
void redirect_stdin(int *pipe_left, char *rstdin) {
  if (pipe_left != NULL) {
    /*close the stdin stream and substitute it with the output end of the pipe */
    close(STDIN_FILENO);        
    dup(pipe_left[READ_END]);
    close(pipe_left[WRITE_END]);
    close(pipe_left[READ_END]);
  }
  else if (rstdin != NULL) {
    /* Redirect stdin to a file if this is the program to the left */
    int redirect_stdin = open(rstdin, O_RDONLY);
    /* If file could not be created or opened for writing */
    if (redirect_stdin == -1) {
      fprintf(stderr, "Could not read from file %s\n", rstdin);
      exit(1);
    }
    dup2(redirect_stdin, 0);
    close(redirect_stdin);
  }
}


/*
 * Name: child_terminated
 *
 * Is called automatically each time a child process terminates
 */
void child_terminated() {
		int exit_status;
    pid_t pid;

    /* If we are waiting for foreground processes to quit, ignore background processes, for now */
    if (waiting) return;
    
    /*
     * loop as long as it finds more terminated child processes
     */
    do {

      /*
       * Check if there is a child process that is terminated and clean up all the zombies
       */
      pid = wait3(&exit_status, WNOHANG, NULL);
      if (pid > 0) {
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
    /* cd without an arguments changes directory to the user's HOME directory
     * chdir is a system call that changes the current directory
     * getenv is a system call that reads the value of the system variable */
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
