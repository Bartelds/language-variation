/*
 * File: difinfo.c
 *
 * (c) Peter Kleiweg, 2004
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2,
 * or (at your option) any later version.
 *
 */

#define difinfoVERSION "0.02"

#define __NO_MATH_INLINES

#ifdef __WIN32__
#  define my_PATH_SEP '\\'
#else
#  define my_PATH_SEP '/'
#endif

#ifdef __MSDOS__
#ifndef __COMPACT__
#error Memory model COMPACT required
#endif  /* __COMPACT__  */
#include <dir.h>
#endif  /* __MSDOS__  */
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFSIZE 2047

typedef enum { FALSE = 0, TRUE } BOOL_;

char
    buffer [BUFSIZE + 1],
    *filename,
    *programname,
    *no_mem_buffer,
    out_of_memory [] = "Out of memory";

long int
    lineno;

FILE
    *fp;

void
    get_programname (char const *argv0),
    errit (char const *format, ...),
    syntax (int err),
    *s_malloc (size_t size),
    GetLine (void);

int
    cmpd (void const *, void const *);

double
    quant (double *d, int len, double q);

int main (int argc, char *argv [])
{
    int
	i,
	j,
	l,
	n [10],
	len,
	max,
	missing,
	size;
    double
	f,
	f1,
	*d,
	sum,
	ssum;

    no_mem_buffer = (char *) malloc (1024);

    get_programname (argv [0]);

    if (argc != 2)
	syntax (0);

    filename = argv [1];
    fp = fopen (filename, "r");
    if (! fp)
        errit ("Opening file \"%s\": %s", filename, strerror (errno));
    lineno = 0;

    GetLine ();
    size = atoi (buffer);
    if (size < 3)
        errit ("Illegal table size in \"%s\", line %lu\n", filename, lineno);

    for (i = 0; i < size; i++)
	GetLine ();

    len = missing = 0;
    d = (double *) s_malloc ((size * size - size) / 2 * sizeof (double));

    for (i = 1; i < size; i++)
        for (j = 0; j < i; j++) {
            GetLine ();
	    if (sscanf (buffer, "%lf", &f) == 1)
		d [len++] = f;
	    else
		missing++;
	}

    fclose (fp);

    qsort (d, len, sizeof (double), cmpd);

    sum = ssum = 0;
    for (i = 0; i < len; i++) {
	sum += d [i];
	ssum += d [i] * d [i];
    }

    printf (
	"missing  : %i\n"
	"\n"
	"mean     : %g\n"
	"std.dev. : %g\n"
	"\n"
	"min.     : %g\n"
	"1st qu.  : %g\n"
	"median   : %g\n"
	"3rd qu.  : %g\n"
	"max.     : %g\n"
	"\n",
	missing,
	sum / (double) len,
	sqrt ((ssum - sum * sum / (double) len) / (double) (len - 1)),
	d [0],
	quant (d, len, .25),
	quant (d, len, .5),
	quant (d, len, .75),
	d [len - 1]
    );

    for (i = 0; i < 10; i++)
	n [i] = 0;
    f = (d [len - 1] - d [0]) / 9.0 * 10.0;
    f1 = d [0] - f / 20.0;
    for (i = 0; i < len; i++)
	n [(int) ((d [i] - f1) / f * 10)]++;
    max = 0;
    for (i = 0; i < 10; i++)
	if (n [i] > max)
	    max = n [i];
    for (i = 0; i < 11; i++) {
	sprintf (buffer, "%g", f1 + f / 10 * i);
	if (i == 10)
	    printf ("%16s\n", buffer);
	else {
	    printf ("%16s : ", buffer);
	    l = n [i] * 60 / max;
	    for (j = 0; j < l; j++)
		printf ("*");
	    printf ("\n");
	}
    }


    return 0;
}

double quant (double *d, int len, double q)
{
    int
	i;
    double
	f,
	r;

    f = q * (double) (len - 1);
    i = (int) f;
    r = f - i;
    
    if (r < .2)
	return d [i];
    if (r < .4)
	return .75 * d [i] + .25 * d [i + 1];
    if (r < .7)
	return .5 * d [i] + .5 * d [i + 1];
    return .25 * d [i] + .75 * d [i + 1];
}

int cmpd (void const *p1, void const *p2)
{
    double
        f;

    f = *((double *)p1) - *((double *)p2);
    if (f < 0.0)
        return -1;
    else if (f > 0.0)
        return 1;
    else
        return 0;
}

void GetLine ()
{
    int
        i;

    for (;;) {
        if (fgets (buffer, BUFSIZE, fp) == NULL)
	    errit ("Premature end of file: %s", filename);
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

void syntax (int err)
{
    fprintf (
	err ? stderr : stdout,
	"\n"
	"Give info on a difference table, Version " difinfoVERSION "\n"
	"(c) P. Kleiweg 2004\n"
	"\n"
	"Usage: %s difference_table_file\n"
	"\n",
	programname
    );
    exit (err);
}
