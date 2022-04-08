/*
 * File: difsum.c
 *
 * (c) Peter Kleiweg
 *     Wed Jan 28 12:46:21 2004
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2,
 * or (at your option) any later version.
 *
 */

#define my_VERSION "0.01"

#define __NO_MATH_INLINES

#ifdef __WIN32__
#  define my_PATH_SEP '\\'
#else
#  define my_PATH_SEP '/'
#endif

#ifdef __MSDOS__
#  ifndef __COMPACT__
#    error Memory model COMPACT required
#  endif  /* __COMPACT__  */
#  include <dir.h>
#endif  /* __MSDOS__  */
#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum { FALSE = 0, TRUE } BOOL_;

#define BUFSIZE 4095

BOOL_
    average = FALSE;

double
    *sum,
    power = 1.0,
    multi = 1.0;

int
    size,
    len,
    infiles = 0,
    arg_c;

long int
    lineno;

double
    *d,
    *m;

char
    buffer [BUFSIZE + 1],
    **comments,
    **names,
    *filename,
    **arg_v,
    *programname,
    *no_mem_buffer,
    out_of_memory [] = "Out of memory",
    *outfile = NULL;

FILE
    *fp_in;

void
    GetLine (void),
    process_file (char *filename),
    get_programname (char const *argv0),
    process_args (void),
    errit (char const *format, ...),
    syntax (void),
    *s_malloc (size_t size);
char
    *get_arg (void),
    *s_strdup (char const *s);

int main (int argc, char *argv [])
{
    int
	i;
    FILE
	*fp;

    no_mem_buffer = (char *) malloc (1024);

    get_programname (argv [0]);

    arg_c = argc;
    arg_v = argv;
    process_args ();

    if (arg_c == 1)
	syntax ();

    comments = (char **) s_malloc (arg_c * sizeof (char *));

    power = multi = 1.0;
    for (i = 1; i < arg_c; i++) {
	if (arg_v [i][0] == '^') {
	    power = atof (arg_v [i] + 1);
	    if (power <= 0.0)
		errit ("Illegal power value: %f", power);
	} else if (arg_v [i][0] == '.' || isdigit ((unsigned char) arg_v [i][0])) {
	    multi = atof (arg_v [i]);
	    if (multi <= 0.0)
		errit ("Illegal multiplication value: %f", multi);
	} else {
	    process_file (arg_v [i]);
	    power = multi = 1.0;
	}
    }

    if (outfile)
	fp = fopen (outfile, "w");
    else
	fp = stdout;

    for (i = 0; i < infiles; i++)
	fprintf (fp, "# %s\n", comments [i]);
    if (average)
	fprintf (fp, "# average\n");
    fprintf (fp, "%i\n", size);
    for (i = 0; i < size; i++)
	fprintf (fp, "%s\n", names [i]);
    for (i = 0; i < len; i++) {
	if (m [i] > 0.0)
	    fprintf (fp, "%g\n", average ? d [i] / m [i] : d [i]);
	else
	    fprintf (fp, "NA\n");
    }

    if (outfile)
	fclose (fp);

    return 0;
}

void process_file (char *file)
{
    int
	i;
    double
	f;

    fp_in = fopen (file, "r");
    filename = file;
    lineno = 0;

    GetLine ();
    i = atoi (buffer);

    if (infiles == 0) {
	size = i;
	len = (size * size - size) / 2;
	if (size < 2)
	    errit ("Illegal table size in file \"%s\", line %li", filename, lineno);
	names = (char **) s_malloc (size * sizeof (char*));
	for (i = 0; i < size; i++) {
	    GetLine ();
	    names [i] = s_strdup (buffer);
	}
	d = (double *) s_malloc (len * sizeof (double));
	m = (double *) s_malloc (len * sizeof (double));
	for (i = 0; i < len; i++)
	    d [i] = m [i] = 0.0;
    } else {
	if (i != size)
	    errit ("Table size mismatch in file \"%s\", line %li", filename, lineno);
	for (i = 0; i < size; i++) {
	    GetLine ();
	    if (strcmp (buffer, names [i]))
		errit ("Label mismatch in file \"%s\", line %li", filename, lineno);
	}
    }

    for (i = 0; i < len; i++) {
	GetLine ();
	if (sscanf (buffer, "%lf", &f) == 1) {
	    if (power != 1.0)
		f = pow (f, power);
	    d [i] += multi * f;
	    m [i] += multi;
	}
    }

    fclose (fp_in);

    sprintf (buffer, "%g * %s ^ %g", multi, file, power);
    comments [infiles] = s_strdup (buffer);

    infiles++;
}

void GetLine ()
{
    int
        i;

    for (;;) {
        if (fgets (buffer, BUFSIZE, fp_in) == NULL)
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

void process_args ()
{
    while (arg_c > 1 && arg_v [1][0] == '-') {
        switch (arg_v [1][1]) {
            case 'a':
                average = TRUE;
                break;
            case 'o':
                outfile = get_arg ();
                break;
            default:
                errit ("Illegal option '%s'", arg_v [1]);
        }
	arg_c--;
	arg_v++;
    }
}

char *get_arg ()
{
    if (arg_v [1][2])
        return arg_v [1] + 2;

    if (arg_c == 2)
        errit ("Missing argument for '%s'", arg_v [1]);

    arg_v++;
    arg_c--;
    return arg_v [1];
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

void syntax ()
{
    fprintf (
	stderr,
	"\n"
	"Version " my_VERSION "\n"
	"\n"
	"Usage: %s [-a] [-o filename]\n"
	"\t[^float] [float] difference_table_file\n"
	"\t...\n"
	"\n"
	"\t-a : average\n"
	"\t-o : output file\n"
	"\n",
	programname
    );
    exit (1);
}
