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


#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "parse.h"

/*
 * Function declarations
 */
void PrintCommand(int, Command *);
void PrintPgm(Pgm *);
void stripwhite(char *);

/* When non-zero, this global means the user is done using this program. */
int done = 0;

/*
 * Name: main
 *
 * Description: Gets the ball rolling...
 *
 */
int main(void)
{
  Command cmd;
  int n;

  while (!done) {

    char *line;
    line = readline("> "); /* readline prints a prompt prompt and then reads
                            * and returns a single line of text from the user
                            * that is stored in 'line' */

    if (!line) { /* If we've reached the end of the line */
      /* Encountered EOF at top level */
      done = 1; /*Set done to 1 (true) --> stop the while loop */
    }
    else {
      /*
       * Remove leading and trailing whitespace from the line
       * Then, if there is anything left, add it to the history list
       * and execute it.
       */
      stripwhite(line); /* stripwhite is defined further done*/

      if(*line) { 
        add_history(line);
        /* execute it */
        n = parse(line, &cmd);
        PrintCommand(n, &cmd);
      }
    }

    if(line) {
      free(line);
    }
  }
  return 0;
}

/*
 * Name: PrintCommand
 *
 * Description: Prints a Command structure as returned by parse on stdout.
 *
 */
void
PrintCommand (int n, Command *cmd)
{
  printf("Parse returned %d:\n", n);
  printf("   stdin : %s\n", cmd->rstdin  ? cmd->rstdin  : "<none>" );
  printf("   stdout: %s\n", cmd->rstdout ? cmd->rstdout : "<none>" );
  printf("   bg    : %s\n", cmd->bakground ? "yes" : "no");
  PrintPgm(cmd->pgm);
}

/*
 * Name: PrintPgm
 *
 * Description: Prints a list of Pgm:s
 *
 */
void
PrintPgm (Pgm *p)
{
  if (p == NULL) {
    return;
  }
  else {
    char **pl = p->pgmlist;

    /* The list is in reversed order so print
     * it reversed to get right
     */
    PrintPgm(p->next);
    printf("    [");
    while (*pl) {
      printf("%s ", *pl++);
    }
    printf("]\n");
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

  while (isspace( string[i] )) { /* isspace(int c) checks whether the passed character
                                  * is white-space. This function returns a non-zero
                                  * value(true) if c is a white-space character
                                  * else, zero (false) */


    i++;
  }

  if (i) {
    strcpy (string, string + i); /* copies string pointed to by source (right argument) into the destination (left argument) */
  }

  i = strlen( string ) - 1; /*returns the length of the string passed as an argument and stores it in i*/
  while (i> 0 && isspace (string[i])) {
    i--;
  }

  string [++i] = '\0';
}
