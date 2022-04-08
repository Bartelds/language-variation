/*
 * File: difmod.c
 *
 * (c) Peter Kleiweg, 2001, 2004, 2010
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2,
 * or (at your option) any later version.
 *
 */

#define difmodVERSION "1.15"

#define __NO_MATH_INLINES

#ifdef __WIN32__
#define my_PATH_SEP '\\'
#else
#define my_PATH_SEP '/'
#endif

#ifdef __MSDOS__
#ifndef __COMPACT__
#error Memory model COMPACT required
#endif  /* __COMPACT__  */
#include <dir.h>
#endif  /* __MSDOS__  */
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef MAXDOUBLE
#include <float.h>
#define MAXDOUBLE DBL_MAX
#endif

#define BUFSIZE 2047

typedef enum { FALSE = 0, TRUE } BOOL_;

typedef struct {
    double
        f;
    BOOL_
        isnum;
} DIFF_;

typedef struct {
    char
        *s;
    int
        i;
} LBL_;

typedef struct {
    char
	*s;
    int
	n,
	max,
	*i;
} GROUP_;

DIFF_
    **olddiff;

LBL_
    *oldlbl;

GROUP_
    *grp;

BOOL_
    dupcheck = TRUE;

int
    *i1 = NULL,
    *i2 = NULL,
    oldsize,
    newsize,
    maxlen = 0;

long unsigned int
    lineno;

char
    buffer [BUFSIZE + 1],
    *oldfile,
    *transfile,
    *outfile = NULL,
    *filename,
    *programname,
    *no_mem_buffer,
    out_of_memory [] = "Out of memory";

FILE
    *fp;

void
    diff (int i, int j, FILE *fp),
    shift (void),
    openread (char *s),
    get_programname (char const *argv0),
    errit (char const *format, ...),
    syntax (int err),
    *s_malloc (size_t size),
    *s_realloc (void *block, size_t size);
char
    *s_strdup (char const *s);
int
    kgv (int l1, int l2),
    lblcmp (void const *, void const *),
    slblcmp (void const *, void const *);

BOOL_
    GetLine (BOOL_ required);

int main (int argc, char *argv [])
{
    int
	i,
	j,
	k,
	n,
	m;
    double
	f;
    FILE
	*fp_out;
    LBL_
	*lptr;

    no_mem_buffer = (char *) malloc (1024);

    get_programname (argv [0]);

    if (argc == 1)
	syntax (0);

    while (argc > 1 && argv [1][0] == '-') {
	switch (argv [1][1]) {
	    case 'd':
		dupcheck = FALSE;
		break;
	    case 'o':
		if (argv [1][2])
		    outfile = argv [1] + 2;
		else {
		    if (argc < 3)
			errit ("Missing argument for option -s");
		    argv++;
		    argc--;
		    outfile = argv [1];
		}
		break;
	    default:
		syntax (1);
	}
	argv++;
	argc--;
    }

    if (argc != 3)
	syntax (1);

    oldfile = argv [1];
    transfile = argv [2];

    openread (oldfile);
    GetLine (TRUE);
    oldsize = atoi (buffer);
    if (oldsize < 2)
	errit ("Illegal table size in \"%s\", line %lu\n", oldfile, lineno);
    oldlbl = (LBL_ *) s_malloc (oldsize * sizeof (LBL_));
    for (i = 0; i < oldsize; i++) {
	GetLine (TRUE);
	oldlbl [i].s = s_strdup (buffer);
	oldlbl [i].i = i;
    }
    olddiff = (DIFF_ **) s_malloc (oldsize * sizeof (DIFF_ *));
    for (i = 0; i < oldsize; i++)
	olddiff [i] = (DIFF_ *) s_malloc (oldsize * sizeof (DIFF_));
    for (i = 0; i < oldsize; i++) {
	olddiff [i][i].f = 0.0;
	olddiff [i][i].isnum = TRUE;
	for (j = 0; j < i; j++) {
	    GetLine (TRUE);
	    if (sscanf (buffer, "%lf", &f) == 1) {
		olddiff [i][j].f = olddiff [j][i].f = f;
		olddiff [i][j].isnum = olddiff [j][i].isnum = TRUE;
	    } else {
		olddiff [i][j].f = MAXDOUBLE;
		olddiff [i][j].isnum = olddiff [j][i].isnum = FALSE;
	    }
	}
    }
    fclose (fp);
    qsort (oldlbl, oldsize, sizeof (LBL_), lblcmp);

    newsize = 0;
    grp = (GROUP_ *) s_malloc (oldsize * sizeof (GROUP_));
    openread (transfile);
    while (GetLine (FALSE)) {
	switch (buffer [0]) {
	    case ':':
		if (newsize == oldsize)
		    errit ("Too many new labels in \"%s\", line %lu", filename, lineno);
		shift ();
		grp [newsize].s = s_strdup (buffer); 
		grp [newsize].n = 0;
		grp [newsize].max = 256;
		grp [newsize].i = (int *) s_malloc (grp [newsize].max * sizeof (int));
		newsize++;
		break;
	    case '-':
		if (newsize == 0)
		    errit ("Missing new label before first old label in \"%s\", line %lu", filename, lineno);
		shift ();
		lptr = bsearch (buffer, oldlbl, oldsize, sizeof (LBL_), slblcmp);
		if (lptr == NULL)
		    errit ("Label \"%s\" in \"%s\", line %lu not found in \"%s\"", buffer, filename, lineno, oldfile);
		if (grp [newsize - 1].n == grp [newsize - 1].max) {
		    grp [newsize - 1].max += 256;
		    grp [newsize - 1].i = (int *) s_realloc (grp [newsize - 1].i, grp [newsize - 1].max * sizeof (int));
		}
		grp [newsize - 1].i [grp [newsize - 1].n++] = lptr->i;
		break;
  	    default:
		errit ("Syntax error in \"%s\", line %lu", transfile, lineno);
	}
    }
    fclose (fp);

    for (i = 0; i < newsize; i++) {
	if (grp [i].n == 0)
	    errit ("No old labels for new label \"%s\"", grp [i].s);
	if (dupcheck)
	    for (j = 0; j < i; j++) {
		if (! strcmp (grp [i].s, grp [j].s))
		    errit ("New label used more then once: \"%s\"", grp [i].s);
		for (n = 0; n < grp [i].n; n++)
		    for (m = 0; m < grp [j].n; m++)
			if (grp [i].i [n] == grp [j].i [m])
			    for (k = 0; k < oldsize; k++)
				if (oldlbl [k].i == grp [i].i [n])
				    errit (
				        "Old label \"%s\" used for two new labels, \"%s\" and \"%s\"",
					oldlbl [k].s,
					grp [i].s,
					grp [j].s
				    );
	    }
    }

    if (outfile) {
	fp_out = fopen (outfile, "w");
	if (! fp_out)
	    errit ("Creating file \"%s\": %s", outfile, strerror (errno));
    } else
	fp_out = stdout;

    fprintf (fp_out, "%i\n", newsize);
    for (i = 0; i < newsize; i++)
	fprintf (fp_out, "%s\n", grp [i].s);
    for (i = 0; i < newsize; i++)
	for (j = 0; j < i; j++)
	    diff (i, j, fp_out);

    if (outfile)
	fclose (fp_out);

    return 0;
}

void diff (int grp1, int grp2, FILE *fp)
{
    int
	i,
	j,
	n,
	nv1,
	nv2,
	size;
    double
	sum;
    BOOL_
	found;

    size = kgv (grp [grp1].n, grp [grp2].n);
    if (size > maxlen) {
	maxlen = 2 * size;
	i1 = (int *) s_realloc (i1, maxlen * sizeof (int));
	i2 = (int *) s_realloc (i2, maxlen * sizeof (int));
    }
    for (i = 0; i < size; i++) {
	i1 [i] = grp [grp1].i [i % grp [grp1].n];
	i2 [i] = grp [grp2].i [i % grp [grp2].n];
    }

    for (found = TRUE; found; ) {
	found = FALSE;
	for (i = 0; i < size; i++)
	    for (j = i + 1; j < size; j++) {
		nv1 = nv2 = 0;
		if (! olddiff [i1 [i]][i2 [i]].isnum)
		    nv1++;
		if (! olddiff [i1 [j]][i2 [j]].isnum)
		    nv1++;
		if (! olddiff [i1 [i]][i2 [j]].isnum)
		    nv2++;
		if (! olddiff [i1 [j]][i2 [i]].isnum)
		    nv2++;
		if (nv1 > nv2 ||
		    (nv1 == 0 && nv2 == 0 &&
 		     olddiff [i1 [i]][i2 [i]].f + olddiff [i1 [j]][i2 [j]].f >
                     olddiff [i1 [i]][i2 [j]].f + olddiff [i1 [j]][i2 [i]].f)
		) {
		    n = i2 [i];
		    i2 [i] = i2 [j];
		    i2 [j] = n;
		    found = TRUE;
		}
	    }
    }
    sum = 0.0;
    n = 0;
    for (i = 0; i < size; i++)
	if (olddiff [i1 [i]][i2 [i]].isnum) {
	    sum += olddiff [i1 [i]][i2 [i]].f;
	    n++;
	}
    if (n)
	fprintf (fp, "%g\n", sum / n);
    else {
	fprintf (fp, "NA\n");
	fprintf (stderr, "No value for: %s \\ %s\n", grp [grp1].s, grp [grp2].s);
    }
}

int kgv (int l1, int l2)
{
    int
        ll1,
        ll2;

    ll1 = l1;
    ll2 = l2;
    for (;;) {
        if (ll1 == ll2)
            return ll1;
        while (ll1 < ll2)
            ll1 += l1;
        while (ll2 < ll1)
            ll2 += l2;
    }
}

void shift ()
{
    int
	i;

    i = 1;
    while (buffer [i] && isspace ((unsigned char) buffer [i]))
	i++;
    if (buffer [i] == '\0')
	errit ("Missing label in \"%s\", line %lu", filename, lineno);
    memmove (buffer, buffer + i, strlen (buffer + i) + 1);
}

int lblcmp (const void *p1, const void *p2)
{
    return strcmp (((LBL_ *) p1)->s, ((LBL_ *) p2)->s);
}

int slblcmp (const void *s, const void *p)
{
    return strcmp ((char *) s, ((LBL_ *) p)->s);
}

void openread (char *s)
{
    fp = fopen (s, "r");
    if (! fp)
	errit ("Opening file \"%s\": %s", s, strerror (errno));
    lineno = 0;
    filename = s;
}

BOOL_ GetLine (BOOL_ required)
{
    int
	i;

    for (;;) {
	if (fgets (buffer, BUFSIZE, fp) == NULL) {
	    if (required)
		errit ("Premature end of file: %s", filename);
	    else
		return FALSE;
	}
	lineno++;
	i = 0;
	while (buffer [i] && isspace ((unsigned char) buffer [i]))
	    i++;
	if (buffer [i] != '\0' && buffer [i] != '#')
	    break;
    }

    if (i)
	memmove (buffer, buffer + i, strlen (buffer + i) + 1);

    i = strlen (buffer) - 1;
    while (i >= 0)
	if (isspace ((unsigned char) buffer [i]))
	    buffer [i--] = '\0';
	else
	    break;

    return TRUE;
}

void errit (char const *format, ...)
{
    va_list
	list;

    fprintf (stderr, "\nError %s: ", programname);

    va_start (list, format);
    vfprintf (stderr, format, list);

    fprintf (stderr, "\n\n");

    exit (1);
}

void get_programname (char const *argv0)
{
#ifdef __MSDOS__
    char
        name [MAXFILE];
    fnsplit (argv0, NULL, NULL, name, NULL);
    programname = strdup (name);
#else
    char
        *p;
    p = strrchr (argv0, my_PATH_SEP);
    if (p)
        programname = strdup (p + 1);
    else
        programname = strdup (argv0);
#endif    
}

void *s_malloc (size_t size)
{
    void
	*p;

    p = malloc (size);
    if (! p) {
        free (no_mem_buffer);
	errit (out_of_memory);
    }
    return p;
}

void *s_realloc (void *block, size_t size)
{
    void
	*p;

    p = realloc (block, size);
    if (! p) {
        free (no_mem_buffer);
	errit (out_of_memory);
    }
    return p;
}

char *s_strdup (char const *s)
{
    char
	*s1;

    if (s) {
	s1 = (char *) s_malloc (strlen (s) + 1);
	strcpy (s1, s);
    } else {
	s1 = (char *) s_malloc (1);
	s1 [0] = '\0';
    }
    return s1;
}

void syntax (int err)
{
    fprintf (
	err ? stderr : stdout,
	"\n"
	"Conversion of a difference table, Version " difmodVERSION "\n"
	"(c) P. Kleiweg 2001, 2004, 2010\n"
	"\n"
	"Usage: %s [-d] [-o filename] old_difference_table_file translation_file\n"
	"\n"
	"\t-d : disable check for duplicates\n"
	"\t-o : output file\n"
	"\n",
	programname
    );
    exit (err);
}
