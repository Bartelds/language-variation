/*
 * File: linc.c
 *
 * (c) Peter Kleiweg 2000, 2001, 2002, 2004, 2006, 2010
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2,
 * or (at your option) any later version.
 *
 */

#define lincVERSION "2.04"

#define __NO_MATH_INLINES

#ifdef __WIN32__
#  define my_PATH_SEP '\\'
#else
#  define my_PATH_SEP '/'
#endif

#ifdef __MSDOS__
#  ifndef __COMPACT__
#    error Memory model COMPACT required
#  endif
#  include <dir.h>
#  include <io.h>
#else
#  include <errno.h>
#  include <unistd.h>
#endif
#include <ctype.h>
#include <float.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef __WIN32__
#  include <sys/timeb.h>
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef enum { FALSE = 0, TRUE = 1} BOOL;

typedef enum { cooUNDEF, cooLL, cooXY, cooDST } COORD_TYPE;

typedef enum { resDEFAULT, resOPTIMAL, resRANDOM, resWORST } RESULT_TYPE;

typedef struct {
    char
        *s;
    double
        x,
        y,
	z;
    BOOL
        coo;
} NAME;

typedef struct {
    double
        geo,
	lnk;
} LNK;

NAME
    *names;

LNK
    **diff = NULL,
    *links;

#define BUFSIZE 2048

COORD_TYPE
    coord_type = cooUNDEF;

RESULT_TYPE
    result_type = resDEFAULT;

BOOL
    longlist = FALSE;

char
    *linkfile,
    *coordfile,
    *dstfile,
    **dst_names,
    buffer [BUFSIZE + 1],
    *programname,
    *no_mem_buffer,
    Out_of_memory [] = "Out of memory!";

int
    *lnkindex,
    *dst_idx,
    n_names,
    n_links,
    n_dst,
    inputline;

double
    aspect,
    noise = 0.0;

void
    average (void),
    randomize_links (void),
    get_programname (char const *argv0),
    errit (char const *format, ...),
    warn (char const *format, ...),
    syntax (int err),
    *s_malloc (size_t size),
    *s_realloc (void *block, size_t size);

int
    linkcmp (void const *p1, void const *p2),
    optimalcmp (void const *p1, void const *p2),
    worstcmp (void const *p1, void const *p2);

double
    diffgeo (int i, int j);

char
    *s_strdup (char const *s);

char const
    *unquote (char *s);

BOOL
    GetLine (FILE *fp, BOOL required, char const *filename);

FILE
    *r_fopen (char const *filename);

int main (int argc, char *argv [])
{
    int
        i,
	ii,
        j,
	jj,
	k,
        n;
    double
        f,
        x,
        y,
        dx,
        dy,
	act,
	opt,
	sgeo,
	sum,
	ssum,
	sd;
    FILE
        *fp;
#ifdef __WIN32__
    struct timeb
        tb;
#endif

    no_mem_buffer = (char *) malloc (1024);

    get_programname (argv [0]);

    if (argc == 1)
	syntax (0);

    while (argc > 1 && argv [1][0] == '-') {
        switch (argv [1][1]) {
	    case 'D':
		coord_type = cooDST;
		break;
	    case 'L':
		coord_type = cooLL;
		break;
	    case 'a':
		coord_type = cooXY;
	        if (argv [1][2])
                    aspect = atof (argv [1] + 2);
		else {
                    argc--;
                    argv++;
                    if (argc < 2)
                        errit ("Missing arg for -a");
                    aspect = atof (argv [1]);
                }
		if (aspect < 1)
		    errit ("Illegal value for aspect, must be 1 or greater");
                break;
	    case 'l':
		longlist = TRUE;
		break;
	    case 'n':
	        if (argv [1][2])
                    noise = atof (argv [1] + 2);
		else {
                    argc--;
                    argv++;
                    if (argc < 2)
                        errit ("Missing arg for -n");
                    noise = atof (argv [1]);
                }
		if (noise <= 0.0)
		    errit ("Illegal value for noise, must be positive");
		break;
	    case 'o':
		result_type = resOPTIMAL;
		break;
	    case 'r':
		result_type = resRANDOM;
		break;
	    case 'w':
		result_type = resWORST;
		break;
            default:
                syntax (1);
	}
        argc--;
        argv++;
    }

    if (coord_type == cooUNDEF)
	syntax (1);

    if (result_type == resOPTIMAL) {
	fputs ("0.0\n", stdout);
	return 0;
    }

    if (argc != 3)
        syntax (1);

    if (result_type == resRANDOM)
	srand ((unsigned int) time (NULL));

    linkfile = argv [1];
    if (coord_type == cooDST)
	dstfile = argv [2];
    else
	coordfile = argv [2];

    fp = r_fopen (linkfile);
    GetLine (fp, TRUE, linkfile);
    n_names = atoi (buffer);
    if (n_names < 2)
        errit ("Illegal number of items in \"%s\": %i", linkfile, n_names);
    names = (NAME *) s_malloc (n_names * sizeof (NAME));
    for (i = 0; i < n_names; i++) {
        GetLine (fp, TRUE, linkfile);
        names [i].s = s_strdup (buffer);
	names [i].coo = FALSE;
    }

    diff = (LNK **) s_malloc (n_names * sizeof (LNK *));
    for (i = 0; i < n_names; i++)
        diff [i] = (LNK *) s_malloc (n_names * sizeof (LNK));
    for (i = 0; i < n_names; i++) {
	diff [i][i].lnk = 0.0;
        for (j = 0; j < i; j++) {
            GetLine (fp, TRUE, linkfile);
            if (sscanf (buffer, "%lf", &f) != 1) {
                warn ("Missing value [%i,%i], file \"%s\", line %i", j + 1, i + 1, linkfile, inputline);
		f = FLT_MAX;
	    }
            diff [i][j].lnk = diff [j][i].lnk = f;
	}
    }
    fclose (fp);

    if (noise > 0.0) {

#ifdef __WIN32__
        ftime (&tb);
        srand (tb.millitm ^ (tb.time << 8));
#else
        srand (time (NULL) ^ (getpid () << 8));
#endif

        n = 0;
        sum = ssum = 0.0;
	for (i = 0; i < n_names; i++)
	    for (j = 0; j < i; j++) {
                sum += diff [i][j].lnk;
                ssum += diff [i][j].lnk * diff [i][j].lnk;
                n++;
            }
        sd = sqrt ((ssum - sum * sum / (float) n) / (float) (n - 1));
	for (i = 0; i < n_names; i++)
	    for (j = 0; j < i; j++) {
		f = noise * sd * (((float) rand ()) / (float) RAND_MAX);
		diff [i][j].lnk += f;
		diff [j][i].lnk += f;
	    }
    }

    if (coord_type == cooDST) {
	fp = r_fopen (dstfile);

	GetLine (fp, TRUE, dstfile);
	n_dst = atoi (buffer);
	if (n_dst < 2)
	    errit ("Illegal number of items in \"%s\": %i", dstfile, n_dst);

	dst_names = (char **) s_malloc (n_dst * sizeof (char *));
	for (i = 0; i < n_dst; i++) {
	    GetLine (fp, TRUE, dstfile);
	    dst_names [i] = s_strdup (buffer);
	}

	dst_idx = (int *) s_malloc (n_dst * sizeof (int));
	for (i = 0; i < n_dst; i++)
	    dst_idx [i] = -1;
	for (i = 0; i < n_names; i++) {
	    for (j = 0; j < n_dst; j++)
		if (! strcmp (names [i].s, dst_names [j]))
		    break;
	    if (j == n_dst)
		errit ("Label \"%s\" in file \"%s\" not found in file \"%s\"", names [i].s, linkfile, dstfile);
	    dst_idx [j] = i;
	}

	for (i = 0; i < n_dst; i++) {
	    ii = dst_idx [i];
	    for (j = 0; j < i; j++) {
		jj = dst_idx [j];
		GetLine (fp, TRUE, dstfile);
		if (sscanf (buffer, "%lf", &f) != 1) {
		    errit ("Missing value [%i,%i], file \"%s\", line %i", j + 1, i + 1, dstfile, inputline);
		}
		if (ii >= 0 && jj >= 0)
		    diff [ii][jj].geo = diff [jj][ii].geo = f;
	    }
	}
	fclose (fp);

    } else {
	fp = r_fopen (coordfile);
	while (GetLine (fp, FALSE, NULL)) {
	    if (sscanf (buffer, "%lf %lf %lf %lf%n", &x, &y, &dx, &dy, &n) < 4)
		errit ("Missing coordinates in file \"%s\", line %i", coordfile, inputline);
	    while (buffer [n] && isspace ((unsigned char) buffer [n]))
		n++;
	    if (! buffer [n])
		errit ("Missing name in file \"%s\", line %i", coordfile, inputline);
	    unquote (buffer + n);
	    for (i = 0; i < n_names; i++)
		if (! strcmp (names [i].s, buffer + n))
		    break;
	    if (i == n_names)
		continue;
	    if (coord_type == cooLL) {
		x = x / 180.0 * M_PI;
		y = y / 180.0 * M_PI;
		names [i].x = sin (x) * cos (y);
		names [i].y = cos (x) * cos (y);
		names [i].z = sin (y);
	    } else {
		names [i].x = x / aspect;
		names [i].y = y;
	    }
	    names [i].coo = TRUE;
	}
	fclose (fp);

	for (i = 0; i < n_names; i++)
	    if (! names [i].coo)
		errit ("No position defined for \"%s\"", names [i].s);

	for (i = 0; i < n_names; i++) {
	    diff [i][i].geo = 0.0;
	    for (j = 0; j < i; j++)
		diff [i][j].geo = diff [j][i].geo = diffgeo (i, j);
	}
    }

    sgeo = 0.0;
    n_links = n_names - 1;
    links = (LNK *) s_malloc (((n_links < 8) ? 8 : n_links) * sizeof (LNK));
    for (i = 0; i < 8; i++)
        links [i].geo = 0;
    for (i = 0; i < n_names; i++) {
	k = 0;
	for (j = 0; j < n_names; j++)
	    if (i != j)
		links [k++] = diff [i][j];
	qsort (links, n_links, sizeof (LNK), optimalcmp);
	opt = links [0].geo +
	      links [1].geo * .70710678 +
	      links [2].geo * .50000000 +
	      links [3].geo * .35355339 +
	      links [4].geo * .25000000 +
	      links [5].geo * .17677670 +
              links [6].geo * .12500000 +
              links [7].geo * .08838835;
	switch (result_type) {
	    case resDEFAULT:
		qsort (links, n_links, sizeof (LNK), linkcmp);
		average ();
		break;
	    case resRANDOM:
		randomize_links ();
		break;
	    case resWORST:
		qsort (links, n_links, sizeof (LNK), worstcmp);
		break;
	    case resOPTIMAL:
		/* not used */
		break;
	}
	act = links [0].geo +
	      links [1].geo * .70710678 +
	      links [2].geo * .50000000 +
	      links [3].geo * .35355339 +
	      links [4].geo * .25000000 +
	      links [5].geo * .17677670 +
              links [6].geo * .12500000 +
              links [7].geo * .08838835;
	if (longlist)
	    printf ("%7.3f   %s\n", (act - opt) / opt, names [i].s);
	sgeo += (act - opt) / opt;
    }

    if (longlist)
	printf ("\nAverage local incoherence: ");
    printf ("%g\n", sgeo / n_names);

    return 0;
}

void average ()
{
    int
	i,
	j;
    double
	av;

    for (i = 0; i < n_links; i++) {
	j = 1;
	av = links [i].geo;
	while (i + j < n_links && links [i].lnk == links [i + j].lnk)
	    av += links [i + j++].geo;
	if (j > 1) {
	    av /= j;
	    for ( ; j; j--)
		links [i++].geo = av;
	    i--;
	}
    }
}

void randomize_links ()
{
    int
	i,
	j;
    double
	f;

    for (i = n_links - 1; i; i--) {
	j = rand () % (i + 1);
	f = links [i].geo;
	links [i].geo = links [j].geo;
	links [j].geo = f;
    }
}

int linkcmp (void const *p1, void const *p2)
{
    double
	f1,
	f2;
    f1 = ((LNK *)p1)->lnk;
    f2 = ((LNK *)p2)->lnk;
    if (f1 < f2)
	return -1;
    if (f1 > f2)
	return 1;

    f1 = ((LNK *)p1)->geo;
    f2 = ((LNK *)p2)->geo;
    if (f1 < f2)
	return -1;
    if (f1 > f2)
	return 1;
    return 0;
}

int optimalcmp (void const *p1, void const *p2)
{
    double
	f1,
	f2;

    f1 = ((LNK *)p1)->geo;
    f2 = ((LNK *)p2)->geo;
    if (f1 < f2)
	return -1;
    if (f1 > f2)
	return 1;
    return 0;
}

int worstcmp (void const *p1, void const *p2)
{
    double
	f1,
	f2;

    f1 = ((LNK *)p1)->geo;
    f2 = ((LNK *)p2)->geo;
    if (f1 < f2)
	return 1;
    if (f1 > f2)
	return -1;
    return 0;
}

double diffgeo (int i, int j)
{
    double
        dx,
	dy;

    if (coord_type == cooLL)
	return acos (names [i].x * names [j].x + names [i].y * names [j].y + names [i].z * names [j].z);
    else {
	dx = names [i].x - names [j].x;
	dy = names [i].y - names [j].y;
	return sqrt (dx * dx + dy * dy);
    }
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

BOOL GetLine (FILE *fp, BOOL required, char const *filename)
/* Lees een regel in
 * Plaats in buffer
 * Negeer lege regels en regels die beginnen met #
 */
{
    int
        i;

    for (;;) {
        if (fgets (buffer, BUFSIZE, fp) == NULL) {
            if (required)
                errit ("Unexpected end of file in \"%s\"", filename);
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

void warn (char const *format, ...)
{
    va_list
	list;

    fprintf (stderr, "Warning %s: ", programname);

    va_start (list, format);
    vfprintf (stderr, format, list);

    fprintf (stderr, "\n");
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

void syntax (int err)
{
    fprintf (
	err ? stderr : stdout,
        "\n"
	"Local Incoherence, Version " lincVERSION "\n"
	"(c) P. Kleiweg 2000, 2001, 2002, 2004, 2006, 2010\n"
	"\n"
	"Usage: %s -D       [-n float] [-o | -r | -w] [-l] diff_table_file distance_table_file\n"
	"\n"
	"Usage: %s -L       [-n float] [-o | -r | -w] [-l] diff_table_file location_file\n"
	"\n"
	"Usage: %s -a float [-n float] [-o | -r | -w] [-l] diff_table_file location_file\n"
	"\n"
	"\t-D : Geographic distances are given in distance tabel file\n"
	"\t-L : Coordinates are in longitude/latitude\n"
        "\t-a : Coordinates are in x/y, with the given aspect ratio, (>= 1)\n"
        "\t-l : Long output\n"
	"\t-n : Add noise\n"
	"\t-o : Calculate value for optimal solution\n"
	"\t-r : Calculate value for a random `solution'\n"
	"\t-w : Calculate value for worst solution\n"
	"\n",
	programname,
	programname,
	programname
    );
    exit (err);
}
