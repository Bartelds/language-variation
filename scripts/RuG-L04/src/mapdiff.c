/*
 * File: mapdiff.c
 *
 * (c) Peter Kleiweg
 *     2003 - 2019
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2,
 * or (at your option) any later version.
 *
 */

#define my_VERSION "0.18"

#define __NO_MATH_INLINES

#ifdef __WIN32__
#  define my_PATH_SEP '\\'
#else
#  define my_PATH_SEP '/'
#endif

#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

typedef enum { FALSE = 0, TRUE = 1} BOOL;

typedef enum { SKIP, PRESERVE, USE } USAGE_;

typedef struct {
    BOOL
        data;
    USAGE_
        usage;
    int
        i,
        v;
    char
        *s;
    double
        x,
        y,
        dx,
        dy;
} NAME;

typedef struct point_ {
    int
        i;
    double
        x,
        y;
    struct point_
        *next;
} POINT;

typedef struct {
    double
        x1,
	y1,
	x2,
	y2,
	xc,
	yc;
    int
        this,
        other;
    double
        n;
} LINE_;

NAME
    *names = NULL;

LINE_
    *lines = NULL;

POINT
    **point;

USAGE_
    default_absent = PRESERVE;

#define BUFSIZE 2048

BOOL
    exist_preserve,
    commentsave = FALSE,
    greyscale = FALSE,
    reverse = FALSE,
    useeofill = FALSE,
    uselimit = FALSE,
    uselink = FALSE,
    hasnegative = FALSE;

char
    **comments = NULL,
    **arg_v,
    *diffile,
    *configfile,
    *outfile = NULL,
    *transformfile = NULL,
    *labelfile = NULL,
    *coordfile = NULL,
    *mapfile = NULL,
    *clipfile = NULL,
    *usefile = NULL,
    *fontname = "Helvetica",
    *fontmatrix = "[ 8 0 0 8 0 0 ]",
    buffer [BUFSIZE + 1],
    buf    [BUFSIZE + 1],
    *programname,
    *no_mem_buffer,
    out_of_memory [] = "Out of memory!";

double
    **ilink;

int
    n_comments = 0,
    max_comments = 0,
    pslevel = 1,
    llx = 0,
    lly = 0,
    urx = 595,
    ury = 842,
    limit = 50,
    arg_c,
    itemdefault = 0,
    n_names = 0,
    max_names = 0,
    high_names = 0,
    n_lines = 0,
    max_lines = 0,
    volgende = 1,
    top,
    inputline;

float
    expF  = 1.0,
    baseF = 0.0;

double
    fillmargin = 0.5,
    radius = 2.5,
    white = 1,
    xgap = 4.0,
    ygap = 4.0,
    linewidth = .5,
    borderwidth = 2,
    symbolsize = 5,
    symbollinewidth = .3,
    backred = .9,
    backgreen = .9,
    backblue = .9,
    margin = 1.0e-20,
    aspect = 1.0,
    xp,
    yp;

time_t
    tp;

FILE
    *fp_out;

void
    crop (int, int),
    get_programname (char const *argv0),
    process_args (void),
    errit (char const *format, ...),
    warn (char const *format, ...),
    syntax (void),
    *s_malloc (size_t size),
    *s_realloc (void *block, size_t size),
    Heapsort (
        void *base,
        size_t nelem,
        size_t width,
        int (*fcmp)(void *, void *)
    );

char
    *getstring (char const *s),
    *get_arg (void),
    *s_strdup (char const *s),
    *tostr (int i);

char const
    *unquote (char *s),
    *psstring (unsigned char const *s);

int
    linescmp (void *, void *),
    linescmpa (void *, void *);

BOOL
    has_argument (void),
    GetLine (BOOL required, FILE *fp);

FILE
    *r_fopen (char const *filename);

BOOL EQUAL (double d1, double d2)
{
    return (d1 - d2 <= margin && d2 - d1 <= margin) ? TRUE : FALSE;
}

BOOL ZERO (double d)
{
    return EQUAL (d, 0.0);
}

BOOL RANGE (double d, double d1, double d2)
{
    if ((d >= d1 && d <= d2) || (d >= d2 && d <= d1))
	return TRUE;
    else
	return FALSE;
}

int main (int argc, char *argv [])
{
    char
        *s = NULL;
    int
	difsize,
	*difnames,
	*ii,
        i,
	i2,
        j,
        j1,
        j2,
        n,
	v;
    double
        x,
        y,
        dx,
        dy,
	f,
        fmin,
        fmax,
        xmin,
        xmax,
        ymin,
        ymax;
    FILE
        *fp;
    POINT
        *ptmp;
    BOOL
	err;

    no_mem_buffer = (char *) malloc (1024);

    get_programname (argv [0]);

    arg_c = argc;
    arg_v = argv;
    process_args ();

    if (arg_c != 3)
        syntax ();

    configfile = arg_v [1];
    diffile = arg_v [2];

    if (outfile) {
        fp_out = fopen (outfile, "w");
        if (! fp_out)
            errit ("Creating file \"%s\": %s", outfile, strerror (errno));
    } else
        fp_out = stdout;

    fp = r_fopen (configfile);
    while (GetLine (FALSE, fp)) {
        if (! has_argument ())
            continue;
        if (! memcmp (buffer, "aspect:", 7)) {
            aspect = atof (buffer + 7);
            if (aspect < 1.0)
                errit ("Illegal value for 'aspect' in file \"%s\", line %i", configfile, inputline);
        } else if (! memcmp (buffer, "markers:", 6)) {
            itemdefault = 0;
            if (strstr (buffer + 8, "none") != NULL)
                continue;
            if (strstr (buffer + 8, "dot") != NULL)
                itemdefault |= 1;
            if (strstr (buffer + 8, "number") != NULL)
                itemdefault |= 2;
            if (strstr (buffer + 8, "label") != NULL)
                itemdefault |= 4;
            if (strstr (buffer + 8, "poly") != NULL)
                itemdefault |= 8;
        } else if (! memcmp (buffer, "missing:", 8)) {
            i = 0;
            if (strstr (buffer + 8, "ignore") != NULL)
                i = 1;
            if (strstr (buffer + 8, "preserve") != NULL)
                i += 2;
            if (i == 0 || i == 3 )
                errit ("Value for 'missing' must be one of: ignore preserve\nin file \"%s\", line %i", configfile, inputline);
            default_absent = (i == 1) ? SKIP : PRESERVE;
        } else if (! memcmp (buffer, "limit:", 6)) {
            limit = atoi (buffer + 6);
            if (limit < 3)
                errit ("Value for 'limit' too small in file \"%s\", line %i", configfile, inputline);
        } else if (! memcmp (buffer, "uselimit:", 9)) {
            if (strstr (buffer + 9, "yes") != NULL)
                uselimit = TRUE;
	    ;
	} else if (! memcmp (buffer, "boundingbox:", 12)) {
            if (sscanf (buffer + 12, " %i %i %i %i", &llx, &lly, &urx, &ury) != 4)
                errit ("Missing value(s) for 'boundingbox' in file \"%s\", line %i", configfile, inputline);
        } else if (! memcmp (buffer, "fontname:", 9)) {
            fontname = getstring (buffer + 9);
        } else if (! memcmp (buffer, "fontmatrix:", 11)) {
            fontmatrix = getstring (buffer + 11);
        } else if (! memcmp (buffer, "pslevel:", 8)) {
            pslevel = atoi (buffer + 8);
            if (pslevel < 1)
                errit ("Illegal 'pslevel' in file \"%s\", line %i", configfile, inputline);
        } else if (! memcmp (buffer, "fillstyle:", 10)) {
            if (strstr (buffer + 10, "eofill") != NULL)
               useeofill = TRUE;
        } else if (! memcmp (buffer, "fillmargin:", 11)) {
            fillmargin = atof (buffer + 11);
            if (fillmargin <= 0.0)
                errit ("Illegal value for 'fillmargin' in file \"%s\", line %i", configfile, inputline);
        } else if (! memcmp (buffer, "radius:", 7)) {
            radius = atof (buffer + 7);
            if (radius <= 0.0)
                errit ("Illegal value for 'radius' in file \"%s\", line %i", configfile, inputline);
        } else if (! memcmp (buffer, "white:", 6)) {
            white = atof (buffer + 6);
            if (white <= 0.0)
                errit ("Illegal value for 'white' in file \"%s\", line %i", configfile, inputline);
        } else if (! memcmp (buffer, "xgap:", 5)) {
            if (sscanf (buffer + 5, "%lf", &xgap) != 1)
                errit ("Missing values for 'xgap' in file \"%s\", line %i", configfile, inputline);
            if (xgap < 0.0)
                errit ("Illegal value for 'xgap' in file \"%s\", line %i", configfile, inputline);
        } else if (! memcmp (buffer, "ygap:", 5)) {
            if (sscanf (buffer + 5, "%lf", &ygap) != 1)
                errit ("Missing values for 'ygap' in file \"%s\", line %i", configfile, inputline);
            if (ygap < 0.0)
                errit ("Illegal value for 'ygap' in file \"%s\", line %i", configfile, inputline);
        } else if (! memcmp (buffer, "linewidth:", 10)) {
            linewidth = atof (buffer + 10);
            if (linewidth <= 0.0)
                errit ("Illegal value for 'linewidth' in file \"%s\", line %i", configfile, inputline);
	} else if (! memcmp (buffer, "borderwidth:", 12)) {
	    borderwidth = atof (buffer + 12);
            if (borderwidth <= 0.0)
                errit ("Illegal value for 'borderwidth' in file \"%s\", line %i", configfile, inputline);
        } else if (! memcmp (buffer, "backrgb:", 8)) {
            if (sscanf (buffer + 8, " %lf %lf %lf", &backred, &backgreen, &backblue) != 3)
                errit ("Missing value(s) for 'backrgb' in file \"%s\", line %i", configfile, inputline);
            if (backred < 0.0 || backred > 1.0)
                errit ("Red value out of range for 'backrgb' in file \"%s\", line %i", configfile, inputline);
            if (backgreen < 0.0 || backgreen > 1.0)
                errit ("Green value out of range for 'backrgb' in file \"%s\", line %i", configfile, inputline);
            if (backblue < 0.0 || backblue > 1.0)
                errit ("Blue value out of range for 'backrgb' in file \"%s\", line %i", configfile, inputline);
        } else if (! memcmp (buffer, "borderrgb:", 10)) {
	    ;
	} else if (! memcmp (buffer, "symbolsize:", 11)) {
	    symbolsize = atof (buffer + 11);
            if (symbolsize <= 1.0)
                errit ("Illegal value for 'symbolsize' in file \"%s\", line %i", configfile, inputline);
	} else if (! memcmp (buffer, "symbollinewidth:", 16)) {
	    symbollinewidth = atof (buffer + 16);
            if (symbollinewidth <= 0.0)
                errit ("Illegal value for 'symbollinewidth' in file \"%s\", line %i", configfile, inputline);
        } else if (! memcmp (buffer, "transform:", 10)) {
            transformfile = getstring (buffer + 10);
        } else if (! memcmp (buffer, "clipping:", 9)) {
            clipfile = getstring (buffer + 9);
        } else if (! memcmp (buffer, "map:", 4)) {
            mapfile = getstring (buffer + 4);
        } else if (! memcmp (buffer, "labels:", 7)) {
            labelfile = getstring (buffer + 7);
        } else if (! memcmp (buffer, "coordinates:", 12)) {
            coordfile = getstring (buffer + 12);
        } else if (! memcmp (buffer, "othermarkers:", 13)) {
	    usefile = getstring (buffer + 13);
        } else if (! memcmp (buffer, "islandr:", 8)) {
	    ;
        } else if (! memcmp (buffer, "islandlw:", 9)) {
	    ;
        } else
            errit ("Unknown option in file \"%s\", line %i:\n%s\n", configfile, inputline, buffer);
    }
    fclose (fp);

    if (labelfile == NULL)
        errit ("Missing 'labels' in file \"%s\"\n", configfile);

    if (coordfile == NULL)
        errit ("Missing 'coordinates' in file \"%s\"\n", configfile);

    if (transformfile == NULL)
        errit ("Missing 'transform' in file \"%s\"\n", configfile);

    fp = r_fopen (labelfile);
    while (GetLine (FALSE, fp)) {
        if (n_names == max_names) {
            max_names += 256;
            names = (NAME *) s_realloc (names, max_names * sizeof (NAME));
        }
        names [n_names].x = names [n_names].y = 0;
        if (sscanf (buffer, "%i%n", &(names [n_names].i), &n) < 1)
            errit ("Missing index in file \"%s\", line %i", labelfile, inputline);
	if (names [n_names].i < 1)
            errit ("Invalid number in file \"%s\", line %i", labelfile, inputline);
	if (names [n_names].i > high_names)
	    high_names = names [n_names].i;
        while (buffer [n] && isspace ((unsigned char) buffer [n]))
            n++;
        if (! buffer [n])
            errit ("Missing name in file \"%s\", line %i", labelfile, inputline);
        names [n_names].s = s_strdup (unquote (buffer + n));
	names [n_names].data = FALSE;
        names [n_names++].v = -2;
    }
    fclose (fp);
    ii = (int *) s_malloc ((high_names + 1) * sizeof (int));
    for (i = 1; i <= high_names; i++)
	ii [i] = 0;
    for (i = 0; i < n_names; i++)
	ii [names [i].i]++;
    err = FALSE;
    for (i = 1; i <= high_names; i++) {
	if (ii [i] > 1) {
	    fprintf (stderr, "Error %s: duplicate index %i in file \"%s\"\n", programname, i, labelfile);
	    err = TRUE;
	} else if (ii [i] == 0) {
	    fprintf (stderr, "Error %s: missing index %i in file \"%s\"\n", programname, i, labelfile);
	    err = TRUE;
	}
    }
    if (err)
	return 1;
    free (ii);

    fp = r_fopen (coordfile);
    while (GetLine (FALSE, fp)) {
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
            warn (
                "Label \"%s\" in file \"%s\" not found in file \"%s\", skipping",
                buffer + n,
                coordfile,
                labelfile
            );
        else {
            names [i].x = x;
            names [i].y = y;
            names [i].dx = dx;
            names [i].dy = dy;
        }
    }
    fclose (fp);

    if (usefile) {
        fp = r_fopen (usefile);
        while (GetLine (FALSE, fp)) {
            if (sscanf (buffer, "%i%n", &v, &n) < 1)
                errit ("Missing values in file \"%s\", line %i", usefile, inputline);
            if (v < -1 || v > 255)
                errit ("Illegal value in file \"%s\", line %i", usefile, inputline);
            while (buffer [n] && isspace ((unsigned char) buffer [n]))
                n++;
            if (! buffer [n])
                errit ("Missing name in file \"%s\", line %i", usefile, inputline);
            unquote (buffer + n);
            for (i = 0; i < n_names; i++)
                if (! strcmp (names [i].s, buffer + n))
                    break;
            if (i == n_names)
                errit (
                    "Label \"%s\" in file \"%s\" not found in file \"%s\"",
                    buffer + n,
                    usefile,
                    labelfile
                );
            names [i].v = v;
        }
        fclose (fp);
    }

    fp = r_fopen (diffile);
    commentsave = TRUE;
    GetLine (TRUE, fp);
    difsize = atoi (buffer);
    if (difsize < 2)
        errit ("Illegal table size in file \"%s\", line %lu\n", diffile, inputline);
    difnames = (int *) s_malloc (difsize * sizeof (int));
    for (i = 0; i < difsize; i++) {
	GetLine (TRUE, fp);
        for (j = 0; j < n_names; j++)
            if (! strcmp (names [j].s, buffer))
                break;
        if (j == n_names)
            errit (
		"Label \"%s\" in file \"%s\" not found in file \"%s\"",
                buffer,
                diffile,
                labelfile
	    );
	difnames [i] = j;
	names [j].data = TRUE;
    }
    ilink = (double **) s_malloc (n_names * sizeof (double *));
    for (i = 0; i < n_names; i++) {
	ilink [i] = (double *) s_malloc (n_names * sizeof (double));
	for (j = 0; j < n_names; j++)
	    ilink [i][j] = FLT_MAX;
    }
    for (i = 1; i < difsize; i++) {
	i2 = difnames [i];
	for (j = 0; j < i; j++) {
	    j2 = difnames [j];
	    GetLine (TRUE, fp);
	    if (sscanf (buffer, "%lf", &f) < 1)
		errit ("Missing vale in file \"%s\", line %i", diffile, inputline);
	    ilink [i2][j2] = ilink [j2][i2] = f;
	    if (f < 0.0)
		hasnegative = TRUE;
	}
    }
    fclose (fp);
    free (difnames);

    exist_preserve = FALSE;
    for (i = 0; i < n_names; i++) {
        if (names [i].v < -1) {
            if (names [i].data)
                names [i].usage = USE;
            else {
                names [i].usage = default_absent;
		if (default_absent == PRESERVE)
		    exist_preserve = TRUE;
	    }
        } else {
	    if (names [i].v & 64)
		names [i].usage = SKIP;
            else if (names [i].v >= 0) {
		if (names [i].data)
		    names [i].usage = USE;
		else {
		    names [i].usage = PRESERVE;
		    exist_preserve = TRUE;
		}
            } else
                names [i].usage = SKIP;
        }
    }

    if (! exist_preserve)
	backred = backgreen = backblue = 1.0;

    xmin = ymin = FLT_MAX;
    xmax = ymax = -FLT_MAX;
    for (i = 0; i < n_names; i++) {
	names [i].y *= aspect;
	if (names [i].x < xmin)
	    xmin = names [i].x;
	if (names [i].x > xmax)
	    xmax = names [i].x;
	if (names [i].y < ymin)
	    ymin = names [i].y;
	if (names [i].y > ymax)
	    ymax = names [i].y;
    }
    point = (POINT **) s_malloc (n_names * sizeof (POINT *));
    for (i = 0; i < n_names; i++) {
	point [i] = (POINT *) s_malloc (sizeof (POINT));
	point [i]->x = xmin - (xmax - xmin) * fillmargin;
	point [i]->y = ymin - (ymax - ymin) * fillmargin;
	point [i]->next = NULL;
	point [i]->i = -1;

	ptmp = (POINT *) s_malloc (sizeof (POINT));
	ptmp->i = -1;
	ptmp->x = xmax + (xmax - xmin) * fillmargin;
	ptmp->y = ymin - (ymax - ymin) * fillmargin;
	ptmp->next = point [i];
	point [i] = ptmp;

	ptmp = (POINT *) s_malloc (sizeof (POINT));
	ptmp->i = -1;
	ptmp->x = xmax + (xmax - xmin) * fillmargin;
	ptmp->y = ymax + (ymax - ymin) * fillmargin;
	ptmp->next = point [i];
	point [i] = ptmp;

	ptmp = (POINT *) s_malloc (sizeof (POINT));
	ptmp->i = -1;
	ptmp->x = xmin - (xmax - xmin) * fillmargin;
	ptmp->y = ymax + (ymax - ymin) * fillmargin;
	ptmp->next = point [i];
	point [i] = ptmp;
    }
    for (i = 0; i < n_names; i++) {
	if (((n_names - i) % 10) == 0)
	    fprintf (stderr, "\r%i ", n_names - i);
	fflush (stderr);
	if (names [i].usage == USE)
	    for (j1 = i + 1, j2 = i - 1; j1 < n_names || j2 >= 0; j1++, j2--) {
		if (j1 < n_names && (names [j1].usage == USE || names [j1].usage == PRESERVE)) {
		    if (EQUAL (names [i].x, names [j1].x) && EQUAL (names [i].y, names [j1].y))
			warn ("Identical coordinates for \"%s\" and \"%s\"", names [i].s, names [j1].s);
		    else
			crop (i, j1);
		}
		if (j2 >= 0 && (names [j2].usage == USE || names [j2].usage == PRESERVE))
		    if (! (EQUAL (names [i].x, names [j2].x) && EQUAL (names [i].y, names [j2].y)))
			crop (i, j2);
	    }
    }
    fprintf (stderr, "\r    \r");
    for (i = 0; i < n_names; i++) {
	names [i].y /= aspect;
	for (ptmp = point [i]; ptmp != NULL; ptmp = ptmp->next)
	    ptmp->y /= aspect;
    }

    fputs (
        "%!PS-Adobe-3.0 EPSF-3.0\n"
        "%%BoundingBox: ",
        fp_out
    );
    fprintf (
        fp_out,
        "%i %i %i %i\n",
        llx,
        lly,
        urx,
        ury
    );
    fputs (
        "%%Creator: ",
        fp_out
    );
    fprintf (fp_out, "%s, (c) P. Kleiweg 2003 - 2012\n", programname);
    fputs ("%%CreationDate: ", fp_out);
    time (&tp);
    fputs (asctime (localtime (&tp)), fp_out);
    fputs ("%%Title: ", fp_out);
    fprintf (fp_out, "%s\n", diffile);
    if (pslevel > 1) {
        fputs ("%%LanguageLevel: ", fp_out);
        fprintf (fp_out, "%i\n", pslevel);
    }
    if (n_comments) {
	fputs ("% Input file comments:\n", fp_out);
	for (i = 0; i < n_comments; i++)
	    fprintf (fp_out, "%c %s\n", '%', comments [i]);
    }
    fputs (
        "%%EndComments\n"
        "\n"
        "64 dict begin\n"
	"\n"
        "% Aspect ratio between x and y coordinates\n",
	fp_out
    );
    fprintf (
        fp_out,
        "/Aspect %g def\n",
        aspect
    );
    fputs (
         "\n"
        "%%BeginDocument: ",
        fp_out
    );
    fputs (transformfile, fp_out);
    fputs ("\n\n", fp_out);
    fp = r_fopen (transformfile);
    while (fgets (buffer, BUFSIZE, fp))
	fputs (buffer, fp_out);
    fclose (fp);

    fputs (
        "\n"
        "%%EndDocument\n"
        "\n",
        fp_out
    );
    fprintf (
        fp_out,
	"/EXP  %g def\n"
	"/BASE %g def\n"
	"\n"
	"/Backrgb [ %g %g %g ] def\n"
        "/Fontname /%s def\n"
        "/Fontmtrx %s def\n"
        "\n"
        "/Radius %g def  %c Radius of dot\n"
        "/White %g def   %c Linewidth around dot\n"
        "/XGap %g def    %c Horizontal distance between label and center of dot\n"
        "/YGap %g def    %c Vertical   distance between label and center of dot\n"
        "\n"
        "/Linewidth %g def\n"
        "/Graylimit .55 def\n"
        "\n",
	expF,
	baseF,
	backred,
	backgreen,
	backblue,
        fontname,
        fontmatrix,
        radius, '%',
        white, '%',
        xgap, '%',
        ygap, '%',
        linewidth
    );
    if (uselimit || ! clipfile)
        fprintf (
            fp_out,
            "/Limit %i def\n"
            "\n",
            limit
        );

    fprintf (
	fp_out,
	"/Borderwidth %g def\n"
	"\n",
	borderwidth
    );
    fputs (
	"\n"
        "%%%% End of User Options %%%%\n"
	"\n",
	fp_out
    );

    if (hasnegative)
	fputs (
	    "/setmycolor {\n"
	    "    dup 0 lt {\n"
	    "        1 add\n"
	    "        dup 1\n"
	    "    } {\n"
	    "        1 exch 1 exch sub dup\n"
	    "    } ifelse\n"
	    "    setrgbcolor\n"
	    "} bind def\n"
	    "\n",
	    fp_out
	);
    else if (greyscale)
	fputs (
	    "/setmycolor {\n"
	    "    dup 0 lt { pop 0 } if\n"
	    "    dup 1 gt { pop 1 } if\n"
	    "    1 exch sub\n"
	    "    setgray\n"
	    "} bind def\n"
	    "\n",
	    fp_out
	);
    else
	fputs (
	    "/YlGnBl28 [\n"
	    "  [ 255 255 255 ]\n"
	    "  [ 255 255 242 ]\n"
	    "  [ 255 255 230 ]\n"
	    "  [ 255 255 217 ]\n"
	    "  [ 249 253 204 ]\n"
	    "  [ 243 250 190 ]\n"
	    "  [ 237 248 177 ]\n"
	    "  [ 224 243 178 ]\n"
	    "  [ 212 238 179 ]\n"
	    "  [ 199 233 180 ]\n"
	    "  [ 175 224 182 ]\n"
	    "  [ 151 214 185 ]\n"
	    "  [ 127 205 187 ]\n"
	    "  [ 106 197 190 ]\n"
	    "  [  86 190 193 ]\n"
	    "  [  65 182 196 ]\n"
	    "  [  53 170 195 ]\n"
	    "  [  41 157 193 ]\n"
	    "  [  29 145 192 ]\n"
	    "  [  31 128 184 ]\n"
	    "  [  32 111 176 ]\n"
	    "  [  34  94 168 ]\n"
	    "  [  35  80 161 ]\n"
	    "  [  36  66 155 ]\n"
	    "  [  37  52 148 ]\n"
	    "  [  27  44 128 ]\n"
	    "  [  18  37 108 ]\n"
	    "  [   8  29  88 ]\n"
	    "] def\n"
	    "\n"
	    "/setmycolor {\n"
	    "    28 mul cvi\n"
	    "    dup  0 lt { pop  0 } if\n"
	    "    dup 27 gt { pop 27 } if\n"
	    "    YlGnBl28 exch get\n"
	    "    aload pop\n"
	    "    3 { 255 div 3 1 roll } repeat\n"
	    "    setrgbcolor\n"
	    "} bind def\n"
	    "\n",
	    fp_out
	);

    if (pslevel < 2)
        fputs (
            "/ISOLatin1Encoding\n"
            "[/.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef\n"
            "/.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef\n"
            "/.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef\n"
            "/.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef\n"
            "/space /exclam /quotedbl /numbersign /dollar /percent /ampersand\n"
            "/quoteright /parenleft /parenright /asterisk /plus /comma /minus /period\n"
            "/slash /zero /one /two /three /four /five /six /seven /eight /nine\n"
            "/colon /semicolon /less /equal /greater /question /at /A /B /C /D /E /F\n"
            "/G /H /I /J /K /L /M /N /O /P /Q /R /S /T /U /V /W /X /Y /Z /bracketleft\n"
            "/backslash /bracketright /asciicircum /underscore /quoteleft /a /b /c /d\n"
            "/e /f /g /h /i /j /k /l /m /n /o /p /q /r /s /t /u /v /w /x /y /z\n"
            "/braceleft /bar /braceright /asciitilde /.notdef /.notdef /.notdef\n"
            "/.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef\n"
            "/.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /dotlessi /grave\n"
            "/acute /circumflex /tilde /macron /breve /dotaccent /dieresis /.notdef\n"
            "/ring /cedilla /.notdef /hungarumlaut /ogonek /caron /space /exclamdown\n"
            "/cent /sterling /currency /yen /brokenbar /section /dieresis /copyright\n"
            "/ordfeminine /guillemotleft /logicalnot /hyphen /registered /macron\n"
            "/degree /plusminus /twosuperior /threesuperior /acute /mu /paragraph\n"
            "/periodcentered /cedilla /onesuperior /ordmasculine /guillemotright\n"
            "/onequarter /onehalf /threequarters /questiondown /Agrave /Aacute\n"
            "/Acircumflex /Atilde /Adieresis /Aring /AE /Ccedilla /Egrave /Eacute\n"
            "/Ecircumflex /Edieresis /Igrave /Iacute /Icircumflex /Idieresis /Eth\n"
            "/Ntilde /Ograve /Oacute /Ocircumflex /Otilde /Odieresis /multiply\n"
            "/Oslash /Ugrave /Uacute /Ucircumflex /Udieresis /Yacute /Thorn\n"
            "/germandbls /agrave /aacute /acircumflex /atilde /adieresis /aring /ae\n"
            "/ccedilla /egrave /eacute /ecircumflex /edieresis /igrave /iacute\n"
            "/icircumflex /idieresis /eth /ntilde /ograve /oacute /ocircumflex\n"
            "/otilde /odieresis /divide /oslash /ugrave /uacute /ucircumflex\n"
            "/udieresis /yacute /thorn /ydieresis]\n"
            "def\n"
            "\n",
        fp_out
    );

    fputs (
        "/RE {\n"
        "    findfont\n"
        "    dup maxlength dict begin {\n"
        "        1 index /FID ne { def } { pop pop } ifelse\n"
        "    } forall\n"
        "    /Encoding exch def\n"
        "    dup /FontName exch def\n"
        "    currentdict end definefont pop\n"
        "} bind def\n"
        "\n"
        "/Font-ISOlat1 ISOLatin1Encoding Fontname RE\n"
	"/theFont /Font-ISOlat1 findfont Fontmtrx makefont def\n"
	"theFont setfont\n"
        "\n"
        "gsave\n"
        "    newpath\n"
        "    0 0 moveto\n"
        "    (Ag) false charpath\n"
        "    pathbbox\n"
        "grestore\n"
        "dup /CHeight exch def\n"
        "2 index sub /Height exch def\n"
        "pop\n"
        "/Lower exch neg def\n"
        "pop\n"
        "\n"
        "/rectfill where {\n"
        "    pop\n"
        "} {\n"
        "    /rectfill {\n"
        "        gsave\n"
        "            newpath\n"
        "            4 2 roll moveto\n"
        "            1 index 0 rlineto\n"
        "            0 exch rlineto\n"
        "            neg 0 rlineto\n"
        "            closepath\n"
        "            fill\n"
        "        grestore\n"
        "    } bind def\n"
        "} ifelse\n"
        "\n"
        "% [ X Y Map (name) dx dy p ]\n"
        "%\n"
        "% X, Y: Map coordinates\n"
        "% Map: This translates map coordinates into screen coordinates\n"
        "% (name): Label\n"
        "% dx, dy: Position of label\n"
        "%     One should be either -1 (dx: left, dy: bottom) or 1 (dx: right, dy: top)\n"
        "%     The other should be between -1 and 1\n"
        "% p: What to do with it, the sum of these values:\n"
        "%     1: put dot\n"
        "%     2: put number\n"
        "%     4: put label\n"
        "%     8: draw polygram\n"
	"%    16: put symbol\n",
        fp_out
    );
    fprintf (
        fp_out,
        "/default %i def\n"
        "/PP [\n",
        itemdefault
    );
    for (i = 0; i < n_names; i++) {
        if (names [i].usage == USE) {
            if (names [i].v < -1)
                s = "default";
            else
                s = tostr (names [i].v);
            fprintf (
                fp_out,
                "  [ %g %g Map (%s) %g %g %s ]\n",
                names [i].x,
                names [i].y,
                psstring ((unsigned char *)names [i].s),
                names [i].dx,
                names [i].dy,
                s
            );
        } else
            fputs ("  [ 0 0 () 0 0 0 ]\n", fp_out);
    }

    fputs (
        "] def\n"
        "\n"
        "/NR PP length 1 sub def\n"
        "\n"
        "/Str 60 string def\n"
        "/dot {\n"
        "    dup /nmbr exch def\n"
        "    PP exch get aload pop\n"
        "    /p exch def\n"
        "    /dy exch def\n"
        "    /dx exch def\n"
        "    /n exch def\n"
        "    /y exch def\n"
        "    /x exch def\n"
	"    p 1 and 0 ne {\n"
	"        x Radius add y moveto\n"
	"        x y Radius 0 360 arc\n"
	"        closepath\n"
	"        gsave\n"
	"            White 2 mul setlinewidth\n"
	"            1 setgray\n"
	"            stroke\n"
	"        grestore\n"
	"        0 setgray\n"
	"        fill\n"
	"    } if\n"
	"\n"
	"    p 2 and 0 ne {\n"
	"        x y moveto\n"
	"        nmbr 1 add Str cvs\n"
	"        dup stringwidth pop 2 div neg CHeight 2 div neg rmoveto\n"
	"        show\n"
	"    } if\n"
        "\n"
        "    p 4 and 0 ne {\n"
        "        x y moveto\n"
        "        n stringwidth pop dx 1 sub 2 div mul dx XGap mul add\n"
        "        dy .5 mul .5 sub Height mul dy YGap mul add Lower add\n"
        "        rmoveto\n"
        "        gsave\n"
        "            1 setgray\n"
        "            currentpoint Lower sub\n"
        "            n stringwidth pop Height 1 add\n"
        "            rectfill\n"
        "        grestore\n"
        "        n show\n"
        "    } if\n"
	"\n",
	fp_out
    );

    fputs (
        "} bind def\n"
        "\n",
        fp_out
    );

    if (clipfile) {
        fp = r_fopen (clipfile);
        fputs ("%%BeginDocument: ", fp_out);
        fputs (clipfile, fp_out);
        fputs ("\n\n", fp_out);
        while (fgets (buffer, BUFSIZE, fp))
            fputs (buffer, fp_out);
        fclose (fp);
        fputs (
            "\n"
            "%%EndDocument\n"
            "\n",
            fp_out
	);
	fprintf (
	    fp_out,
            "gsave Backrgb aload pop setrgbcolor %sfill grestore\n"
            "\n"
            "gsave %sclip newpath\n\n",
	    useeofill ? "eo" : "",
	    useeofill ? "eo" : ""
        );
    }

    if (clipfile && exist_preserve) {
	fputs (
	    "/poly {\n"
	    "    newpath\n"
	    "    Map moveto\n"
	    "    counttomark 2 idiv { Map lineto } repeat\n"
	    "    closepath\n"
	    "    gsave stroke grestore fill\n"
	    "    pop\n"
	    "} bind def\n"
	    ".1 setlinewidth\n"
	    "1 setgray\n",
	    fp_out
	);
	for (i = 0; i < n_names; i++)
	    if (names [i].usage == USE) {
		fputs ("mark\n", fp_out);
		for (ptmp = point [i]; ptmp != NULL; ptmp = ptmp->next)
		    fprintf (fp_out, "%g %g\n", ptmp->x, ptmp->y);
		fputs ("poly\n", fp_out);
	    }
	fputs ("0 setgray\n", fp_out);
    }

    fputs (
	"\n"
	"Borderwidth setlinewidth\n"
	"1 setlinecap\n",
	fp_out
    );

    max_lines = n_names * 4;
    lines = (LINE_ *) s_malloc (max_lines * sizeof (LINE_));
    for (i = 0; i < n_names; i++)
	if (names [i].usage == USE)
	    for (ptmp = point [i]; ptmp != NULL; ptmp = ptmp->next)
		if (ptmp->i > i && names [ptmp->i].usage == USE && ilink [i][ptmp->i] != FLT_MAX)
		{
		    if (n_lines == max_lines) {
			max_lines += n_names;
			lines = (LINE_ *) s_realloc (lines, max_lines * sizeof (LINE_));
		    }
		    lines [n_lines].x1 = ptmp->x;
		    lines [n_lines].y1 = ptmp->y;
		    if (ptmp->next) {
			lines [n_lines].x2 = ptmp->next->x;
			lines [n_lines].y2 = ptmp->next->y;
		    } else {
			lines [n_lines].x2 = point [i]->x;
		        lines [n_lines].y2 = point [i]->y;
		    }
		    lines [n_lines].xc = names [i].x;
		    lines [n_lines].yc = names [i].y;
		    lines [n_lines].this = i;
		    lines [n_lines].other = ptmp->i;
		    lines [n_lines++].n = ilink [i][ptmp->i];
		}

    if (reverse) {
	if (hasnegative) {
	    fmin = fmax = 0.0;
	    for (i = 0; i < n_lines; i++) {
		if (lines [i].n <= 0.0) {
		    fmin = fmax = lines [i].n;
		    break;
		}
	    }
	    for (i = 1; i < n_lines; i++) {
		if (lines [i].n <= 0.0) {
		    if (lines [i].n < fmin)
			fmin = lines [i].n;
		    if (lines [i].n > fmax)
			fmax = lines [i].n;
		}
	    }
	    for (i = 0; i < n_lines; i++)
		if (lines [i].n <= 0.0)
		    lines [i].n = fmax + fmin - lines [i].n;
	    fmin = fmax = 0.0;
	    for (i = 0; i < n_lines; i++) {
		if (lines [i].n >= 0.0) {
		    fmin = fmax = lines [i].n;
		    break;
		}
	    }
	    for (i = 1; i < n_lines; i++) {
		if (lines [i].n >= 0.0) {
		    if (lines [i].n < fmin)
			fmin = lines [i].n;
		    if (lines [i].n > fmax)
			fmax = lines [i].n;
		}
	    }
	    for (i = 0; i < n_lines; i++)
		if (lines [i].n >= 0.0)
		    lines [i].n = fmax + fmin - lines [i].n;
	} else {
	    fmin = fmax = lines [0].n;
	    for (i = 1; i < n_lines; i++) {
		if (lines [i].n < fmin)
		    fmin = lines [i].n;
		if (lines [i].n > fmax)
		    fmax = lines [i].n;
	    }
	    for (i = 0; i < n_lines; i++)
		lines [i].n = fmax + fmin - lines [i].n;
	}
    }

    Heapsort (lines, n_lines, sizeof (LINE_), linescmp);
    fprintf (fp_out, "/MINIMUM %g def\n", lines [0].n);
    fprintf (fp_out, "/MAXIMUM %g def\n", lines [n_lines - 1].n);
    if (hasnegative)
	Heapsort (lines, n_lines, sizeof (LINE_), linescmpa);
    if (uselink)
	fputs (
	    "/B {\n"
	    "    dup 0 lt /ng exch def\n"
	    "    ng {\n"
	    "        MINIMUM\n"
	    "    } {\n"
	    "        MAXIMUM\n"
	    "    } ifelse\n"
	    "    div\n"
	    "    EXP exp\n"
	    "    1 BASE sub mul BASE add\n"
	    "    ng { neg } if\n"
	    "    setmycolor\n"
	    "    PP exch get dup 0 get exch 1 get moveto\n"
	    "    PP exch get dup 0 get exch 1 get lineto\n"
	    "    stroke\n"
	    "} bind def\n"
            "\n",
	    fp_out
	);
    else {
	fputs (
	    "/B {\n"
	    "    dup 0 lt /ng exch def\n"
	    "    ng {\n"
	    "        MINIMUM\n"
	    "    } {\n"
	    "        MAXIMUM\n"
	    "    } ifelse\n"
	    "    div\n"
	    "    EXP exp\n"
	    "    1 BASE sub mul BASE add\n"
	    "    ng { neg } if\n"
	    "    setmycolor\n",
	    fp_out
	);
	if (uselimit || ! clipfile)
	    fputs (
	        "    gsave\n"
		"        Map\n"
		"        2 copy\n"
		"        exch Limit add exch\n"
		"        moveto\n"
		"        Limit 0 360 arc\n"
		"        closepath\n"
		"        clip\n"
		"        newpath\n"
		"        Map moveto Map lineto stroke\n"
		"    grestore\n",
		fp_out
	    );
	else
	    fputs (
	        "    Map moveto Map lineto stroke\n",
		fp_out
	    );
	fputs (
	    "} bind def\n"
	    "\n",
	    fp_out
	);
    }
    for (i = 0; i < n_lines; i++)
	if (uselink)
	    fprintf (
		fp_out,
		"%i %i %g B\n",
		lines [i].this,
		lines [i].other,
		lines [i].n
	    );
	else if (clipfile && !uselimit) 
	    fprintf (
		fp_out,
		"%g %g %g %g %g B\n",
		lines [i].x1,
		lines [i].y1,
		lines [i].x2,
		lines [i].y2,
		lines [i].n
	    );
	else
	    fprintf (
		fp_out,
		"%g %g %g %g %g %g %g B\n",
		lines [i].x1,
		lines [i].y1,
		lines [i].x2,
		lines [i].y2,
		lines [i].xc,
		lines [i].yc,
		lines [i].n
	    );

    fputs ("\n1 setlinewidth\n0 setlinecap\n0 setgray\n\n", fp_out);

    if (clipfile)
        fputs (
            "grestore newpath\n"
            "\n",
            fp_out
        );

    if (mapfile) {
        fputs ("%%BeginDocument: ", fp_out);
        fputs (mapfile, fp_out);
        fputs ("\n\n", fp_out);
        fp = r_fopen (mapfile);
        while (fgets (buffer, BUFSIZE, fp))
            fputs (buffer, fp_out);
        fclose (fp);
        fputs ("\n%%EndDocument\n\n", fp_out);
    }

    fputs (
	"0 setgray\n"
	"theFont setfont\n"
        "0 1 NR { dot } for\n"
        "\n"
        "end\n"
        "showpage\n"
        "%%EOF\n",
         fp_out
    );

    if (outfile)
        fclose (fp_out);

    return 0;
}

char *tostr (int i)
{
    sprintf (buf, "%i", i);
    return buf;
}

/*
 * (x1, y1) and (x2, y2) are endpoints of short line
 * (x, y) is point on long line
 * (rx, ry) is direction of long line
 * result in (xp, yp)
 */
BOOL docross (double x1, double y1, double x2, double y2, double x, double y, double rx, double ry)
{
    double
        a,
        aa,
        b,
        bb,
        rrx,
        rry;

    if (ZERO (rx) && ZERO (ry))
        return FALSE;

    /* direction of short line */
    rrx = x2 - x1;
    rry = y2 - y1;

    if (ZERO (rrx) && ZERO (rry))
        return FALSE;

    if (EQUAL (rrx * ry, rx * rry))
	return FALSE;    /* parallel lines */

    /* long line vertical */
    if (ZERO (rx)) {
        xp = x;
        yp = rry / rrx * (xp - x1) + y1;
        return RANGE (xp, x1, x2);
    }

    /* short line vertical */
    if (ZERO (rrx)) {
        xp = x1;
        yp = ry / rx * (x1 - x) + y;
        return RANGE (yp, y1, y2);
    }

    a  = ry  / rx;
    aa = rry / rrx;
    b  = y  - a  * x;
    bb = y1 - aa * x1;
    xp = (bb - b) / (a - aa);
    yp = a * xp + b;
    return RANGE (xp, x1, x2);
}

void crop (int i, int j)
{
    double
        x,
        y,
        rx,
        ry,
        xp1,
        yp1,
        xp2,
        yp2;
    POINT
        *p,
        *p1,
        *p2,
        *p3,
        *p4;
    BOOL
	p1outside,
	p2outside,
	p3outside,
	p4outside;

    x = (names [i].x + names [j].x) / 2.0;
    y = (names [i].y + names [j].y) / 2.0;
    rx = names [j].y - names [i].y;
    ry = names [i].x - names [j].x;

    for (p1 = point [i]; p1->next != NULL; p1 = p1->next) {
        if (p1 == p1->next)
            errit ("p1 == p1->next: THIS IS A BUG");
        p2 = p1->next;
        if (docross (p1->x, p1->y, p2->x, p2->y, x, y, rx, ry))
            break;
    }
    if (! p1->next)
         return;
    xp1 = xp;
    yp1 = yp;

    p3 = p2;
    if (EQUAL (p2->x, xp) && EQUAL (p2->y, yp))
	p3 = p3->next;
    for ( ; p3 != NULL; p3 = p3->next) {
        if (p3 == p3->next)
            errit ("p3 == p3->next: THIS IS A BUG");
        p4 = p3->next;
        if (! p4)
            p4 = point [i];
        if (docross (p3->x, p3->y, p4->x, p4->y, x, y, rx, ry))
            if (! (EQUAL (xp1, xp) && EQUAL (yp1, yp)))
                break;
    }
    if (! p3)
        return;
    xp2 = xp;
    yp2 = yp;

    /* outside actually means: outside or on the line */
    p1outside = docross (p1->x, p1->y, names [i].x, names [i].y, xp1, yp1, xp2 - xp1, yp2 - yp1);
    p2outside = docross (p2->x, p2->y, names [i].x, names [i].y, xp1, yp1, xp2 - xp1, yp2 - yp1);
    p3outside = docross (p3->x, p3->y, names [i].x, names [i].y, xp1, yp1, xp2 - xp1, yp2 - yp1);
    p4outside = docross (p4->x, p4->y, names [i].x, names [i].y, xp1, yp1, xp2 - xp1, yp2 - yp1);


    if (p3outside == p4outside && ! ((EQUAL (xp2, p3->x) && EQUAL (yp2, p3->y)) || (EQUAL (xp2, p4->x) && EQUAL (yp2, p4->y)))) {
#ifdef DEBUG
        fprintf (stderr, "\r    %i %s - %i %s - p3 %s p4 %s\n",
                 i + 1, names [i].s,
                 j + 1, names [j].s,
                 p3outside ? "outside" : "inside",
                 p4outside ? "outside" : "inside");
#endif
        return;
    }

    if (EQUAL (p1->x, xp1) && EQUAL (p1->y, yp1)) {
	/* p1 on line */

        if (p2outside) {
	    p1->i = j;
            if (EQUAL (xp2, p4->x) && EQUAL (yp2, p4->y))
                p1->next = p4;
            else {
                p1->next = p3;
                p3->x = xp2;
                p3->y = yp2;
            }
        } else {
            /* p2 at inside */
            if (EQUAL (xp2, p3->x) && EQUAL (yp2, p3->y)) {
		p3->i = j;
                p3->next = NULL;
	    } else if (p3->next) {
		p4->i = j;
                p4->x = xp2;
                p4->y = yp2;
                p4->next = NULL;
            }
        }

    } else if (EQUAL (p2->x, xp1) && EQUAL (p2->y, yp1)) {
	/* p2 on line */

        if (p2 == p3) {
            warn ("p2 on line and p2 == p3: THIS IS A BUG (i:%s j:%s)", names[i].s, names[j].s);
	    return;
	}
        if (p1outside) {
            point [i] = p2;
            if (! p3->next)
                p3->next = p4 = (POINT *) s_malloc (sizeof (POINT));
	    p4->i = j;
            p4->x = xp2;
            p4->y = yp2;
            p4->next = NULL;
        } else {
            /* p1 at inside */
            if (EQUAL (xp2, p3->x) && EQUAL (yp2, p3->y))
		/* this shouldn't happen */
                return;
	    p2->i = j;
            if (EQUAL (xp2, p4->x) && EQUAL (yp2, p4->y))
                p2->next = p3->next;
            else {
                p2->next = p3;
                p3->x = xp2;
                p3->y = yp2;
            }
        }

    } else {

	if (p1outside == p2outside) {
#ifdef DEBUG
	    fprintf (stderr, "\r    %i %s - %i %s - p1 %s p2 %s\n",
		     i + 1, names [i].s,
		     j + 1, names [j].s,
		     p1outside ? "outside" : "inside",
		     p2outside ? "outside" : "inside");
#endif
	    return;
	}

	if (p1outside) {
	    point [i] = p1;
	    p1->x = xp1;
	    p1->y = yp1;
	    if (! p3->next)
		p4 = p3->next = (POINT *) s_malloc (sizeof (POINT));
	    p4->i = j;
	    p4->x = xp2;
	    p4->y = yp2;
	    p4->next = NULL;
	} else {
	    /* p1 at inside */
	    p2->x = xp1;
	    p2->y = yp1;
	    if (EQUAL (p4->x, xp2) && EQUAL (p4->y, yp2)) {
		p2->next = p3->next;
	    } else {
		if (p2 == p3) {
		    p = p3->next;
		    p3 = (POINT *) s_malloc (sizeof (POINT));
		    p3->next = p;
		    p3->i = p2->i;
		}
		p2->next = p3;
		p3->x = xp2;
		p3->y = yp2;
	    }
	    p2->i = j;
	}
    }
}

char const *psstring (unsigned char const *s)
{
    int
        i,
        j;

    j = 0;
    for (i = 0; s [i]; i++) {
        if (j + 4 > BUFSIZE)
            errit ("String too long: \"%s\"", s);
        if (s [i] == '(' ||
            s [i] == ')' ||
            s [i] == '\\'
        ) {
            buffer [j++] = '\\';
            buffer [j++] = s [i];
        } else if (s [i] < 32 || s [i] > 126) {
            buffer [j++] = '\\';
            sprintf (buffer + j, "%3o", (unsigned) s [i]);
            j += 3;
        } else
            buffer [j++] = s [i];
    }
    buffer [j] = '\0';
    return buffer;
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

BOOL has_argument ()
{
    char
        *p;

    p = strchr (buffer, ':');
    if (p == NULL)
        return FALSE;
    p++;
    while (p [0] && isspace ((unsigned char) p [0]))
        p++;
    return p [0] ? TRUE : FALSE;
}

char *getstring (char const *s)
{
    while (s [0] && isspace ((unsigned char) s [0]))
        s++;
    return s_strdup (s);
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

BOOL GetLine (BOOL required, FILE *fp)
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
                errit ("Missing data");
            return FALSE;
        }

        inputline++;
        i = strlen (buffer);
        while (i && isspace ((unsigned char) buffer [i - 1]))
            buffer [--i] = '\0';
        i = 0;
        while (buffer [i] && isspace ((unsigned char) buffer [i]))
            i++;
        if (buffer [i] == '#') {
	    if (commentsave) {
		if (n_comments == max_comments) {
		    max_comments += 256;
		    comments = (char **) s_realloc (comments, max_comments * sizeof (char *));
		}
		comments [n_comments++] = s_strdup (buffer + i);
	    }
            continue;
	}
        if (buffer [i]) {
            memmove (buffer, buffer + i, strlen (buffer) + 1);
	    commentsave = FALSE;
            return TRUE;
        }
    }
}

void process_args ()
{
    while (arg_c > 1 && arg_v [1][0] == '-') {
        switch (arg_v [1][1]) {
            case 'c':
                expF = atof (get_arg ());
		if (expF <= 0.0)
		    errit ("Value for -c must be positive");
                break;
            case 'C':
                baseF = atof (get_arg ());
		if (baseF >= 1.0)
		    errit ("Value for -C must be less than 1");
                break;
            case 'g':
                greyscale = TRUE;
                break;
	    case 'l':
		uselink = TRUE;
		break;
            case 'o':
                outfile = get_arg ();
                break;
            case 'r':
                reverse = TRUE;
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

void warn (char const *format, ...)
{
    va_list
        list;

    fprintf (stderr, "\nWarning %s: ", programname);

    va_start (list, format);
    vfprintf (stderr, format, list);

    fprintf (stderr, "\n\n");
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

int linescmp (void *p1, void *p2)
{
    double
	f;
    f = ((LINE_ *)p1)->n - ((LINE_ *)p2)->n;
    if (f < 0.0)
	return -1;
    if (f > 0.0)
	return 1;
    return 0;
}

int linescmpa (void *p1, void *p2)
{
    double
	f;
    f = fabs(((LINE_ *)p1)->n) - fabs(((LINE_ *)p2)->n);
    if (f < 0.0)
	return -1;
    if (f > 0.0)
	return 1;
    return 0;
}

void Heapsort
    (
        void *base,
        size_t nelem,
        size_t width,
        int (*fcmp)(void *, void *)
    )
{
    unsigned
        L, R, S, J;
    char
        *A,
        *heapswap;

    if (nelem < 2)
        return;

    A = (char *) base;
    heapswap = (char *) s_malloc (width);

    A -= width;                 /* table: A[1]..        */
    R = nelem;                  /*              ..A[R]  */
    L = (R >> 1) + 1;
    while (R > 1) {

        if (L > 1)
            /* generate reverse partial ordered tree */
            --L;
        else {
            /* delete max from reverse partial ordered list
               and append in front of ordered list    */
            memcpy (heapswap, A + width, width);
            memcpy (A + width, A + R * width, width);
            memcpy (A + R-- * width, heapswap, width);
        }

        /* A[L+1]..A[R] is reverse partial ordered tree
           insert A[L] to make A[L]..A[R] reverse partial ordered tree  */
        memcpy (heapswap, A + L * width, width);
        J = L;
        for (;;) {
            S = J;              /* mother         */
            J <<= 1;            /* left daughter  */
            if (J > R)          /* no daughters   */
                break;
            if ( (J < R) && (fcmp (A + J * width, A + (J + 1) * width) < 0) )
                J++;            /*  right daughter  */
            if (fcmp (heapswap, A + J * width) >= 0)
                break;
            memcpy (A + S * width, A + J * width, width);
        }
        memcpy (A + S * width, heapswap, width);
    }

    free (heapswap);
}

void syntax ()
{
    fprintf (
        stderr,
        "\n"
        "Difference Line Map Drawing, Version " my_VERSION "\n"
        "(c) P. Kleiweg 2003 - 2019\n"
        "\n"
        "Usage: %s [-c float] [-C float] [-g] [-l] [-o filename]\n\tconfigfile differencefile\n"
	"\n"
	"\t-c : contrast parameter 1 (now: %g)\n"
	"\t-C : contrast parameter 2 (now: %g)\n"
	"\t-g : use greyscale\n"
	"\t-l : draw links instead of borders\n"
        "\t-o : output file\n"
	"\t-r : reverse light/dark\n"
        "\n",
        programname,
	expF,
	baseF
    );
    exit (1);
}
