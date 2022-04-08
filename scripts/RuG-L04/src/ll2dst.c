/*
 * File: ll2dst.c
 *
 * (c) Peter Kleiweg 2001, 2007
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2,
 * or (at your option) any later version.
 *
 */

#define ll2dstVERSION "1.19"

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
#  include <io.h>
#else
#  include <errno.h>
#  include <unistd.h>
#endif
#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef enum { FALSE = 0, TRUE = 1} BOOL;

typedef struct {
    char
        *s;
    int
        i;
    double
        x,
        y,
	z;
    BOOL
        defined;
} NAME_;

NAME_
    *names = NULL;

#define BUFSIZE 2048

char
    **arg_v,
    buffer [BUFSIZE + 1],
    *outfile = NULL,
    *lblfile = NULL,
    *programname,
    *no_mem_buffer,
    Out_of_memory [] = "Out of memory!";

int
    arg_c,
    max_names = 0,
    n_names = 0,
    inputline;

float
    aspect;

BOOL
    XYmodus = FALSE;

void
    get_programname (char const *argv0),
    process_args (void),
    errit (char const *format, ...),
    syntax (int err),
    *s_malloc (size_t size),
    *s_realloc (void *block, size_t size);

char
    *get_arg (void),
    *s_strdup (char const *s);

char const
    *unquote (char *s);

int
    icmp (void const *p1, void const *p2),
    scmp (void const *p1, void const *p2),
    lblcmp (void const *key, void const *p);

BOOL
    GetLine (FILE *fp);

FILE
    *r_fopen (char const *filename);

int main (int argc, char *argv [])
{
    int
        i,
        j,
        n;
    double
        x,
        y,
	d,
        dx,
        dy;
    FILE
        *fp,
	*fp_out;
    NAME_
	*n_ptr;

    no_mem_buffer = (char *) malloc (1024);

    get_programname (argv [0]);

    if (argc == 1)
	syntax (0);

    arg_c = argc;
    arg_v = argv;
    process_args ();

    if (arg_c != 2)
	syntax (1);

    if (lblfile) {
	fp = r_fopen (lblfile);
	while (GetLine (fp)) {
	    if (sscanf (buffer, "%i%n", &i, &n) < 1)
		errit ("Missing index number in file \"%s\", line %i", lblfile, inputline);
	    if (i < 1)
		errit ("Illegal value in file \"%s\", line %i", lblfile, inputline);
	    while (buffer [n] && isspace ((unsigned char) buffer [n]))
		n++;    
	    if (! buffer [n])
		errit ("Missing name in file \"%s\", line %i", lblfile, inputline);
	    if (n_names == max_names) {
		max_names += 256;
		names = (NAME_ *) s_realloc (names, max_names * sizeof (NAME_));
	    }
	    names [n_names].i = i;
	    names [n_names].s = s_strdup (unquote (buffer + n));
	    names [n_names++].defined = FALSE;
	}
	fclose (fp);
	qsort (names, n_names, sizeof (NAME_), icmp);
	for (i = 0; i < n_names; i++) {
	    if (names [i].i < i + 1)
		errit ("Duplicate labels found for number %i in file \"%s\"", names [i].i, lblfile);
	    if (names [i].i > i + 1)
		errit ("Missing label for number %i in file \"%s\"", i + 1, lblfile);
	}
	qsort (names, n_names, sizeof (NAME_), scmp);
    }

    fp = r_fopen (arg_v [1]);
    while (GetLine (fp)) {
	if (sscanf (buffer, "%lf %lf %lf %lf%n", &x, &y, &dx, &dy, &n) < 4)
	    errit ("Missing coordinates in file \"%s\", line %i", arg_v [1], inputline);
	while (buffer [n] && isspace ((unsigned char) buffer [n]))
	    n++;
	if (! buffer [n])
	    errit ("Missing name in file \"%s\", line %i", arg_v [1], inputline);
	if (lblfile) {
	    n_ptr = bsearch (unquote (buffer + n), names, n_names, sizeof (NAME_), lblcmp);
	    if (! n_ptr)
		continue;
	} else {
	    if (max_names == n_names) {
		max_names += 256;
		names = (NAME_ *) s_realloc (names, max_names * sizeof (NAME_));
	    }
	    names [n_names].s = s_strdup (unquote (buffer + n));
	    n_ptr = names + n_names++;
	}
	if (XYmodus) {
	    n_ptr->x = x;
	    n_ptr->y = y;
	} else {
	    x = x / 180.0 * M_PI;
	    y = y / 180.0 * M_PI;
	    n_ptr->x = sin (x) * cos (y);
	    n_ptr->y = cos (x) * cos (y);
	    n_ptr->z = sin (y);
	}
	n_ptr->defined = TRUE;
    }
    fclose (fp);

    if (lblfile) {
	qsort (names, n_names, sizeof (NAME_), icmp);
	for (i = 0; i < n_names; i++)
	    if (! names [i].defined)
		errit ("Missing coordinates for \"%s\"", names [i].s);
    }

    if (outfile) {
	fp_out = fopen (outfile, "w");
	if (! fp_out)
	    errit ("Creating file \"%s\": %s", outfile, strerror (errno));
    } else
	fp_out = stdout;

    fprintf (fp_out, "%i\n", n_names);
    for (i = 0; i < n_names; i++)
	fprintf (fp_out, "%s\n", names [i].s);
    for (i = 0; i < n_names; i++)
	for (j = 0; j < i; j++) {
	    if (XYmodus) {
		dx = (names [i].x - names [j].x) / aspect;
		dy = names [i].y - names [j].y;
		d = sqrt (dx * dx + dy * dy);
	    } else {
		d = names [i].x * names [j].x +
		    names [i].y * names [j].y +
		    names [i].z * names [j].z;
		if (d < -1.0)
		    d = -1.0;
		if (d > 1.0)
		    d = 1.0;
		d = acos (d) / M_PI * 20000.0;
	    }
	    fprintf (fp_out, "%g\n", (float) d);
	}

    if (outfile)
	fclose (fp_out);

    return 0;
}

int icmp (void const *p1, void const *p2)
{
    return ((NAME_ *)p1)->i - ((NAME_ *)p2)->i;
}

int scmp (void const *p1, void const *p2)
{
    return strcmp (((NAME_ *)p1)->s, ((NAME_ *)p2)->s);
}

int lblcmp (void const *key, void const *p)
{
    return strcmp ((char *) key, ((NAME_ *)p)->s);
}

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

FILE *r_fopen (char const *filename)
{
    FILE
        *fp;

    inputline = 0;
    fp = fopen (filename, "r");
    if (! fp)
        errit ("Opening file \"%s\": %s", filename, strerror (errno));
    return fp;
}

BOOL GetLine (FILE *fp)
/* Lees een regel in
 * Plaats in buffer
 * Negeer lege regels en regels die beginnen met #
 */
{
    int
        i;

    for (;;) {
        if (fgets (buffer, BUFSIZE, fp) == NULL) {
            return FALSE;
	}

        inputline++;
        i = strlen (buffer);
        while (i && isspace ((unsigned char) buffer [i - 1]))
            buffer [--i] = '\0';
        i = 0;
        while (buffer [i] && isspace ((unsigned char) buffer [i]))
            i++;
        if (buffer [i] == '#')
            continue;
        if (buffer [i]) {
            memmove (buffer, buffer + i, strlen (buffer) + 1);
            return TRUE;
        }
    }
}

void *s_malloc (size_t size)
{
    void
        *p;

    p = malloc (size);
    if (! p) {
        free (no_mem_buffer);
        errit (Out_of_memory);
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
        errit (Out_of_memory);
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

void process_args ()
{
    while (arg_c > 1 && arg_v [1][0] == '-') {
        switch (arg_v [1][1]) {
	    case 'a':
		aspect = atof (get_arg ());
		if (aspect < 1.0)
		    errit ("Illegal value for aspect mode");
		XYmodus = TRUE;
		break;
            case 'l':
                lblfile = get_arg ();
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

void syntax (int err)
{
    fprintf (
	err ? stderr : stdout,
        "\n"
	"Longitude / latitude to distance convertor, Version " ll2dstVERSION "\n"
	"(c) P. Kleiweg 2001, 2007\n"
	"\n"
	"Usage: %s [-a float] [-l filename] [-o filename] location_file\n"
	"\n"
	"\t-a : switch to XY modus, using aspect ratio (>= 1.0)\n"
	"\t-l : file with numbered labels\n"
	"\t-o : output file\n"
	"\n",
	programname
    );
    exit (err);
}
