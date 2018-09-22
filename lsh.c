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
        PrintCommand(n, &cmd);
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
 * Name: PrintCommand
 *
 * Description: Prints a Command structure as returned by parse on stdout.
 *
 */
void PrintCommand (int n, Command *cmd)
{
  printf("Parse returned %d:\n", n);
  /* ? : => it means: if  cmd->rstdin is NOT NULL or 0, use cmd->rstdin, otherwise use "<none>"
  * Those ternary expressions are one argument to the printf function. %s will show the result
  *  of the ternary expression on the screen 
  */ 
  printf("   stdin : %s\n", (cmd->rstdin  ? cmd->rstdin  : "<none>") );
  printf("   stdout: %s\n", (cmd->rstdout ? cmd->rstdout : "<none>") );
  printf("   bg    : %s\n", (cmd->bakground ? "yes" : "no"));
  /*
   * this prints all programs and arguments to run and pipe together
   */
  PrintPgm(cmd->pgm);
}

/*
 * Name: PrintPgm
 *
 * Description: Prints a list of Pgm:s. It takes a pointer to a program and print the details about it 
 *
 */
void PrintPgm (Pgm *p)
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
     * p->pgmlist:  is that array given by the parse function. Parser fills the command struct, printCommand
     * takes the pgm part of the command strunct and sent it to printPgm. 
     */
    char **pl = p->pgmlist;

    /* The list is in reversed order (I don't know why- design choice) so print
     * it reversed to get right.
     * p is the pointer to the last program in the chain of pipes. printPgm calls recursively. 
     * (it is like a postorder print is a tree: first print all the rest of the list recurively then print myself)
     *  printing example: [wc -l]
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
