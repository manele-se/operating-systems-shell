/* This file contains the code for parser used to parse the input
 * given to shell program. You shouldn't need to modify this 
 * file */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "parse.h"

#define PIPE  ('|')
#define BG    ('&')
#define RIN   ('<')
#define RUT   ('>')

#define ispipe(c) ((c) == PIPE)
#define isbg(c)   ((c) == BG)
#define isrin(c)  ((c) == RIN)
#define isrut(c)  ((c) == RUT)
#define isspec(c) (ispipe(c) || isbg(c) || isrin(c) || isrut(c))

static Pgm  cmdbuf[20], *cmds;
static char cbuf[256], *cp;
static char *pbuf[50], **pp;

/*
 * A quick hack to parse a commandline
 */

/* Läser från en sträng (buf) som är kommandoraden
 * Fyller på en struct Command med information om kommandoraden
 */

int
parse (char *buf, Command *c)
{
  int n;

  /* Pekare till en struct Pgm
   * Varje Pgm ("program") som ska köras
   * Till exempel "ls -l" blir ETT "program"
   */
  Pgm *cmd0;

  /* t är en pekare som troligen används för att gå "framåt" i strängen och tolka innehållet */
  char *t = buf;
  /* tok ingen aning vad det kan vara ännu */
  char *tok;

  init();
  /* Ställer in "standard" värde för:
   * rstdin  (redirect standard input)  = NULL (ingen redirect)
   * rstdout (redirect standard output)
   * rstderr (redirect standard error)
   * bakground (starta som bakgrundsprocess)
   * pgm     (program att starta)
   */
  c->rstdin    = NULL;
  c->rstdout   = NULL;
  c->rstderr   = NULL;
  c->bakground = 0; /* false */
  c->pgm       = NULL;

/* vi pratar om det sen - det här är GAMMAL stil av programmering
 * man kan bryta mot reglerna för loopar, if- och switch-satser genom goto
 */
newcmd:
  if ((n = acmd(t, &cmd0)) <= 0) {
    return -1;
  }

  t += n;

  cmd0->next = c->pgm;
  c->pgm = cmd0;

newtoken:
  n = nexttoken(t, &tok);
  if (n == 0) {
    return 1;
  }
  t += n;

  switch(*tok) {
  case PIPE:
    goto newcmd;
    break;
  case BG:
    n = nexttoken(t, &tok);
    if (n == 0) {
      c->bakground = 1;
      return 1;
    }
    else {
      fprintf(stderr, "illegal bakgrounding\n");
      return -1;
    }
    break;
  case RIN:
    if (c->rstdin != NULL) {
      fprintf(stderr, "duplicate redirection of stdin\n");
      return -1;
    }
    if ((n = nexttoken(t, &(c->rstdin))) < 0) {
      return -1;
    }
    if (!isidentifier(c->rstdin)) {
      fprintf(stderr, "Illegal filename: \"%s\"\n", c->rstdin);
      return -1;
    }
    t += n;
    goto newtoken;
    break;
  case RUT:
    if (c->rstdout != NULL) {
      fprintf(stderr, "duplicate redirection of stdout\n");
      return -1;
    }
    if ((n = nexttoken(t, &(c->rstdout))) < 0)  {
      return -1;
    }
    if (!isidentifier(c->rstdout)) {
      fprintf(stderr, "Illegal filename: \"%s\"\n", c->rstdout);
      return -1;
    }
    t += n;
    goto newtoken;
    break;
  default:
    return -1;
  }
  goto newcmd;
}

void init( void )
{
  int i;
  /* Initierar en array av 19 struct Pgm i en array */
  /* Så att cmbbuf[0].next pekar på cmbbuf[1],
            cmbbuf[1].next pekar på cmbbuf[2],
            cmbbuf[2].next pekar på cmbbuf[3],
            ...
            cmbbuf[18].next pekar på cmbbuf[19]
            */
  for (i=0;i<19;i++) {
    cmdbuf[i].next = &cmdbuf[i+1];
  }

  /* Den sista cmbbuf[19].next pekar på ingenting
   * Det brukar signalera att det inte finns någon "nästa" härifrån
   */
  cmdbuf[19].next = NULL;

  cmds = cmdbuf;
  cp = cbuf; /* ??? */
  pp = pbuf; /* ??? */
}

int
nexttoken( char *s, char **tok)
{
  char *s0 = s;
  char c;

  *tok = cp;
  while (isspace(c = *s++) && c);
  if (c == '\0') {
    return 0;
  }
  if (isspec(c)) {
    *cp++ = c;
    *cp++ = '\0';
  }
  else {
    *cp++ = c;
    do {
        c = *cp++ = *s++;
    } while (!isspace(c) && !isspec(c) && (c != '\0'));
    --s;
    --cp;
    *cp++ = '\0';
  }
  return s - s0;
}

int
acmd (char *s, Pgm **cmd)
{
  char *tok;
  int n, cnt = 0;
  Pgm *cmd0 = cmds;
  cmds = cmds->next;
  cmd0->next = NULL;
  cmd0->pgmlist = pp;

next:
  n = nexttoken(s, &tok);
  if (n == 0 || isspec(*tok)) {
    *cmd = cmd0;
    *pp++ = NULL;
    return cnt;
  }
  else {
    *pp++ = tok;
    cnt += n;
    s += n;
    goto next;
  }
}


#define IDCHARS "_-.,/~+"

int
isidentifier (char *s)
{
    while (*s) {
      char *p = strrchr (IDCHARS,  *s);
      if (! isalnum(*s++) && (p == NULL))
        return 0;
    }
    return 1;
}
