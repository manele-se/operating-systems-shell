
/*
 * info om varje program. Det finns en variabel pgmlist som är array of strängar. Där den försata elementet är
 * namnet på programmet och den andra är argumentet till praogrammet. Testat detta igenom att skriva t.ex ls på shellen
 * Det visas stdin: <none>
 *           stdout:<none>
 *           bg: no
 *           [ls]
 * testa med ls < a visar stdin:a vilket betyder att parse förstår att input ska komma från filen a . Titta i printCommand!
 * 
 */
typedef struct c {
  char **pgmlist;
  struct c *next;
} Pgm;

/*
 * Command är en representation av en hel kommando. En kommando är en hel rad som användaren har matat in. 
 * Ett kommando Kan innebära att många programm startas. En kommando har information om: 
 * - redirect stdin (den inkommande data strömmen) <
 * - redirect stdout (den utgående dataströmmen) >
 * - stderr (om någonting går fel, skrivs ett felmedellandet) 2>
 * - backgrund (process activerat eller ej.)  &
 * - pgm (vilka programm som ska startas-> pipe kedja) |
 */
typedef struct node { 
  Pgm  *pgm;
  char *rstdin;
  char *rstdout;
  char *rstderr;
  int bakground;
} Command;

extern void init( void );
extern int parse ( char *, Command *);
extern int nexttoken( char *, char **);
extern int acmd( char *, Pgm **);
extern int isidentifier( char * );



