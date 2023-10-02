/*
 *  Top users/processes display for Unix
 *  Version 3
 *
 *  This program may be freely redistributed,
 *  but this entire comment MUST remain intact.
 *
 *  Copyright (c) 1984, 1989, William LeFebvre, Rice University
 *  Copyright (c) 1989, 1990, 1992, William LeFebvre, Northwestern University
 */

/* prototypes for functions found in utils.c */

int atoiwi();
char *itoa(register int val);
char *itoa7();
int digits();
char *strecpy();
char **argparse();
long Percentages(int cnt,
                 int *out,
                 register long *new,
                 register long *old,
                 long*        diffs);
char *errmsg();
char *format_time(long seconds);
char *format_k();
