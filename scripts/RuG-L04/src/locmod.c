/*
 * File: locmod.c
 *
 * (c) Peter Kleiweg, 2001, 2004
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2,
 * or (at your option) any later version.
 *
 */

#define locmodVERSION "1.17"

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
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define BUFSIZE 2047

typedef enum { FALSE = 0, TRUE } BOOL_;

typedef enum { cooUNDEF, cooLL, cooXY } COORD_TYPE;

typedef struct {
    double
        x,
	y,
	z;
    char
        *s;
} NAME_;

NAME_
    *names = NULL;

COORD_TYPE
    coord_type = cooUNDEF;

int
    n_names = 0,
    max_names = 0;

long unsigned int
    lineno;

char
    buffer [BUFSIZE + 1],
    qbuffer [2 * BUFSIZE + 2],
    newlabel [BUFSIZE + 1],
    *oldfile,
    *transfile,
    *outfile = NULL,
    *filename,
    *programname,
    *no_mem_buffer,
    out_of_memory [] = "Out of memory";

FILE
    *fp,
    *fp_out;

void
    result (double x, double y, double z, int n),
    shift (void),
    openread (char *s),
    get_programname (char const *argv0),
    errit (char const *format, ...),
    syntax (int err),
    *s_malloc (size_t size),
    *s_realloc (void *block, size_t size);
char
    *s_strdup (char const *s);
char const
    *quote (char const *s),
    *unquote (char *s);
int
    lblcmp (void const *, void const *),
    slblcmp (void const *, void const *);
BOOL_
    GetLine (BOOL_ required);

int main (int argc, char *argv [])
{
    int
	i = 0,
	n;
    double
	x,
	y,
	fx,
	fy,
	fz = 0;
    BOOL_
	started;
    NAME_
        *lptr;
 
    no_mem_buffer = (char *) malloc (1024);

    get_programname (argv [0]);

    if (argc == 1)
	syntax (0);

    while (argc > 1 && argv [1][0] == '-') {
	switch (argv [1][1]) {
	    case 'L':
		coord_type = cooLL;
		break;
	    case 'a':
		coord_type = cooXY;
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

    if (coord_type == cooUNDEF)
	syntax (1);

    if (argc != 3)
	syntax (1);

    oldfile = argv [1];
    transfile = argv [2];

    openread (oldfile);
    while (GetLine (FALSE)) {
        if (n_names == max_names) {
            max_names += 256;
            names = (NAME_ *) s_realloc (names, max_names * sizeof (NAME_));
        }
        names [n_names].x = names [n_names].y = 0;
        if (sscanf (
                buffer,
                "%lf %lf %lf %lf%n",
                &x,
                &y,
                &fx,
                &fy,
                &n
            ) < 4
        )
            errit ("Missing coordinates in file \"%s\", line %i", oldfile, lineno);
	if (coord_type == cooLL) {
	    x = x / 180.0 * M_PI;
	    y = y / 180.0 * M_PI;
	    names [n_names].x = sin (x) * cos (y);
	    names [n_names].y = cos (x) * cos (y);
	    names [n_names].z = sin (y);
	} else {
	    names [n_names].x = x;
	    names [n_names].y = y;
	    names [n_names].z = 0;
	}
        while (buffer [n] && isspace ((unsigned char) buffer [n]))
            n++;
	unquote (buffer + n);
        if (! buffer [n])
            errit ("Missing name in file \"%s\", line %i", oldfile, lineno);
        names [n_names].s = s_strdup (buffer + n);
        n_names++;
    }
    fclose (fp);
    qsort (names, n_names, sizeof (NAME_), lblcmp);

    if (outfile) {
	fp_out = fopen (outfile, "w");
	if (! fp_out)
	    errit ("Creating file \"%s\": %s", outfile, strerror (errno));
    } else
	fp_out = stdout;

    started = FALSE;
    openread (transfile);
    while (GetLine (FALSE)) {
	switch (buffer [0]) {
	    case ':':
		if (started && i)
		    result (fx, fy, fz, i);
		started = TRUE;
		i = 0;
		fx = fy = fz = 0.0;
		shift ();
		strcpy (newlabel, quote (buffer));
		break;
	    case '-':
		if (! started)
		    errit ("Missing new label before first old label in \"%s\", line %lu", filename, lineno);
		shift ();
		lptr = bsearch (buffer, names, n_names, sizeof (NAME_), slblcmp);
		if (lptr == NULL)
		    errit ("Label \"%s\" in \"%s\", line %lu not found in \"%s\"", buffer, filename, lineno, oldfile);
		fx += lptr->x;
		fy += lptr->y;
		fz += lptr->z;
		i++;
		break;
  	    default:
		errit ("Syntax error in \"%s\", line %lu", transfile, lineno);
	}
    }
    if (started && i)
	result (fx, fy, fz, i);

    fclose (fp);

    if (outfile)
	fclose (fp_out);

    return 0;
}

void result (double x, double y, double z, int n)
{
    double
	fx,
	fy,
	xy;

    if (coord_type == cooXY) {
	fprintf (fp_out, "%g\t%g\t1\t0\t%s\n", x / n, y / n, newlabel);
	return;
    }

    if (x > -.0001 && x < .0001 &&
	y > -.0001 && y < .0001
    )
	fx = 0;
    else
	fx = atan2 (x, y) / M_PI * 180.0;

    xy = sqrt(x * x + y * y);
    if (xy > -.0001 && xy < .0001 &&
	z  > -.0001 && z  < .0001
    )
	fy = 0;
    else
	fy = atan2 (z, xy) / M_PI * 180.0;

    fprintf (fp_out,  "%g\t%g\t1\t0\t%s\n", fx, fy, newlabel);

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
    return strcmp (((NAME_ *) p1)->s, ((NAME_ *) p2)->s);
}

int slblcmp (const void *s, const void *p)
{
    return strcmp ((char *) s, ((NAME_ *) p)->s);
}

/*
 * add quotes if necessary
 */
char const *quote (char const *s)
{
    int
        i,
        j;

    for (;;) {
        if (s [0] == '"')
            break;
        for (i = 0; s [i]; i++)
            if (isspace ((unsigned char) s [i]))
                break;
        if (s [i])
            break;

        return s;
    }

    j = 0;
    qbuffer [j++] = '"';
    for (i = 0; s [i]; i++) {
        if (s [i] == '"' || s [i] == '\\')
            qbuffer [j++] = '\\';
        qbuffer [j++] = s [i];
    }
    qbuffer [j++] = '"';
    qbuffer [j] = '\0';
    return qbuffer;
}

/*
 * remove quotes from string, overwrite with result
 */
char const *unquote (char *s)
{
    int
	i,
	j;

    if (s [0] != '"')
	return s;
    j = 0;
    for (i = 1; s [i]; i++) {
	if (s [i] == '"')
	    break;
	if (s [i] == '\\') {
	    if (s [i + 1])
		s [j++] = s [++i];
	} else
	    s [j++] = s [i];
    }
    s [j] = '\0';
    return s;
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
	"Conversion of a file with coordinates, Version " locmodVERSION "\n"
	"(c) P. Kleiweg 2001, 2004\n"
	"\n"
	"Usage: %s -L [-o filename] old_location_file translation_file\n"
	"\n"
	"Usage: %s -a [-o filename] old_location_file translation_file\n"
	"\n"
	"\t-L : Coordinates are in longitude/latitude\n"
	"\t-a : Coordinates are in x/y\n"
	"\t-o : output file\n"
	"\n",
	programname,
	programname
    );
    exit (err);
}
