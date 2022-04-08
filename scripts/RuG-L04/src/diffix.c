/*
 * File: diffix.c
 *
 * (c) Peter Kleiweg, 2001, 2004
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2,
 * or (at your option) any later version.
 *
 */

#define diffixVERSION "1.17"

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

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define BUFSIZE 2047

typedef enum { FALSE = 0, TRUE } BOOL_;

typedef enum { cooUNDEF, cooLL, cooXY } COORD_TYPE;

typedef enum { NUMBER, NONUMBER, FIXED } ISNUM_;

typedef struct {
    double
        f;
    ISNUM_
        isnum;
} DIFF_;

typedef struct {
    char
        *s;
    int
        i;
    double
        x,
	y,
	z;
    BOOL_
        coord;
} LBL_;

typedef struct {
    int
        i;
    double
        d;
} OTHER_;

DIFF_
    **diff;

LBL_
    *lbl;

OTHER_
    *other;

COORD_TYPE
    coord_type = cooUNDEF;

char
    buffer [BUFSIZE + 1],
    *filename,
    *outfile = NULL;

int
    size,
    arg_c;

long int
    lineno;

double
    aspect = 0;

FILE
    *fp;

char
    **arg_v,
    *programname,
    *no_mem_buffer,
    out_of_memory [] = "Out of memory";

void
    openread (char *s),
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
    lblscan (void const *, void const *),
    ilblcmp (void const *, void const *),
    slblcmp (void const *, void const *),
    ocmp (void const *, void const *);

BOOL_
    GetLine (BOOL_ required),
    fix (int i, int j);

int main (int argc, char *argv [])
{
    int
	count,
	i,
	j,
	n;
    double
	f,
	x,
	y,
	dx,
	dy;
    FILE
	*fp_out;
    LBL_
	*lptr;

    no_mem_buffer = (char *) malloc (1024);

    get_programname (argv [0]);

    if (argc == 1)
	syntax (0);

    arg_c = argc;
    arg_v = argv;
    process_args ();

    if (coord_type == cooUNDEF)
	syntax (1);

    if (arg_c != 3)
	syntax (1);

    openread (arg_v [1]);
    GetLine (TRUE);
    size = atoi (buffer);
    if (size < 2)
        errit ("Illegal table size in \"%s\", line %lu\n", filename, lineno);
    other = (OTHER_ *) s_malloc (size * sizeof (OTHER_));
    lbl = (LBL_ *) s_malloc (size * sizeof (LBL_));
    for (i = 0; i < size; i++) {
        GetLine (TRUE);
        lbl [i].s = s_strdup (buffer);
        lbl [i].i = i;
	lbl [i].coord = FALSE;
    }
    diff = (DIFF_ **) s_malloc (size * sizeof (DIFF_ *));
    for (i = 0; i < size; i++)
        diff [i] = (DIFF_ *) s_malloc (size * sizeof (DIFF_));
    for (i = 0; i < size; i++) {
	diff [i][i].f = 0.0;
	diff [i][i].isnum = NONUMBER;
        for (j = 0; j < i; j++) {
            GetLine (TRUE);
	    if (sscanf (buffer, "%lf", &f) == 1) {
                diff [i][j].f = diff [j][i].f = f;
                diff [i][j].isnum = diff [j][i].isnum = NUMBER;
            } else
                diff [i][j].isnum = diff [j][i].isnum = NONUMBER;
        }
    }
    fclose (fp);
    qsort (lbl, size, sizeof (LBL_), slblcmp);

    openread (arg_v [2]);
    while (GetLine (FALSE)) {
        if (sscanf (
	        buffer,
                "%lf %lf %lf %lf%n",
                &x,
                &y,
                &dx,
                &dy,
                &n
            ) < 4
        )
            errit ("Missing coordinates in file \"%s\", line %i", filename, lineno);
        while (buffer [n] && isspace ((unsigned char) buffer [n]))
            n++;
	unquote (buffer + n);
        if (! buffer [n])
            errit ("Missing name in file \"%s\", line %i", filename, lineno);
	lptr = bsearch (buffer + n, lbl, size, sizeof (LBL_), lblscan);
	if (lptr) {
	    if (coord_type == cooLL) {
		x = x / 180.0 * M_PI;
		y = y / 180.0 * M_PI;
		lptr->x = sin (x) * cos (y);
		lptr->y = cos (x) * cos (y);
		lptr->z = sin (y);
	    } else {
		lptr->x = x;
		lptr->y = y;
	    }
	    lptr->coord = TRUE;
	}
    }
    fclose (fp);
    qsort (lbl, size, sizeof (LBL_), ilblcmp);

    for (i = 0; i < size; i++)
	if (! lbl [i].coord)
	    errit ("Missing coordinates for \"%s\"", lbl [i].s);

    count = 0;
    for (i = 0; i < size; i++)
	for (j = 0; j < i; j++)
	    if (diff [i][j].isnum == NONUMBER && ! fix (i, j)) {
		fprintf (stderr, "No value for %s / %s\n", lbl [i].s, lbl [j].s);
		count++;
	    }
    if (count)
	errit ("%i missing value%s could not be fixed", count, (count == 1) ? "" : "s");

    if (outfile) {
	fp_out = fopen (outfile, "w");
	if (! fp_out)
	    errit ("Creating file \"%s\": %s", outfile, strerror (errno));
    } else
	fp_out = stdout;

    fprintf (fp_out, "%i\n", size);
    for (i = 0; i < size; i++)
	fprintf (fp_out, "%s\n", lbl [i].s);
    for (i = 0; i < size; i++)
	for (j = 0; j < i; j++)
	    fprintf (fp_out, "%g\n", diff [i][j].f);

    if (outfile)
	fclose (fp_out);

    return 0;
}

BOOL_ fix (int i1, int i2)
{
    int
	i,
	n;
    double
	dx,
	dy,
	sum;

    n = 0;
    sum = 0.0;

    for (i = 0; i < size; i++) {
	if (coord_type == cooLL)
	    other [i].d = acos (lbl [i].x * lbl [i2].x + lbl [i].y * lbl [i2].y + lbl [i].z * lbl [i2].z);
	else {
	    dx = (lbl [i].x - lbl [i2].x) / aspect;
	    dy = lbl [i].y - lbl [i2].y;
	    other [i].d = dx * dx + dy * dy;
	}
	other [i].i = i;
    }
    qsort (other, size, sizeof (OTHER_), ocmp);
    for (i = 0; i < size && n < 4; i++)
	if (diff [i1][other [i].i].isnum == NUMBER) {
	    sum += diff [i1][other [i].i].f;
	    n++;
	}
    if (n < 4)
	return FALSE;

    for (i = 0; i < size; i++) {
	if (coord_type == cooLL)
	    other [i].d = acos (lbl [i].x * lbl [i1].x + lbl [i].y * lbl [i1].y + lbl [i].z * lbl [i1].z);
	else {
	    dx = (lbl [i].x - lbl [i1].x) / aspect;
	    dy = lbl [i].y - lbl [i1].y;
	    other [i].d = dx * dx + dy * dy;
	}
	other [i].i = i;
    }
    qsort (other, size, sizeof (OTHER_), ocmp);
    for (i = 0; i < size && n < 8; i++)
	if (diff [i2][other [i].i].isnum == NUMBER) {
	    sum += diff [i2][other [i].i].f;
	    n++;
	}
    if (n < 8)
	return FALSE;

    sum /= n;
    diff [i1][i2].f = diff [i2][i1].f = sum;
    diff [i1][i2].isnum = diff [i2][i1].isnum = FIXED;

    return TRUE;
}

int lblscan (const void *s, const void *p)
{
    return strcmp ((char *) s, ((LBL_ *) p)->s);
}

int slblcmp (const void *p1, const void *p2)
{
    return strcmp (((LBL_ *) p1)->s, ((LBL_ *) p2)->s);
}

int ilblcmp (const void *p1, const void *p2)
{
    return ((LBL_ *) p1)->i - ((LBL_ *) p2)->i;
}

int ocmp (const void *p1, const void *p2)
{
    double
	d;
    d = ((OTHER_ *) p1)->d - ((OTHER_ *) p2)->d;
    if (d < 0)
	return -1;
    if (d > 0)
	return 1;
    return 0;
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

void process_args ()
{
    while (arg_c > 1 && arg_v [1][0] == '-') {
        switch (arg_v [1][1]) {
	    case 'L':
		coord_type = cooLL;
		break;
            case 'a':
		coord_type = cooXY;
                aspect = atof (get_arg ());
		if (aspect < 1.0)
		    errit ("Illegal aspect %g, must be 1 or greater", aspect);
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
	"Fixing of a difference table that has data missing, Version " diffixVERSION "\n"
	"(c) P. Kleiweg 2001, 2004\n"
	"\n"
	"Usage: %s -L       [-o filename] difference_table_file location_file\n"
	"\n"
	"Usage: %s -a float [-o filename] difference_table_file location_file\n"
	"\n"
        "\t-L : Coordinates are in longitude/latitude\n"
        "\t-a : Coordinates are in x/y, with the given aspect ratio, (>= 1)\n"
	"\t-o : Output file\n"
	"\n",
	programname,
	programname
    );
    exit (err);
}
