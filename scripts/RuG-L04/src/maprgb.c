/*
 * File: maprgb.c
 *
 * (c) Peter Kleiweg
 *     1998 - 2010
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2,
 * or (at your option) any later version.
 *
 */

#define my_VERSION "0.34"

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
        red,
        green,
        blue,
        x,
        y,
        dx,
        dy;
} NAME;

typedef struct point_ {
    double
        x,
        y;
    struct point_
        *next;
} POINT;

NAME
    *names = NULL;

POINT
    **point;

USAGE_
    default_absent = PRESERVE;

#define BUFSIZE 2048

char
    **arg_v,
    *configfile,
    *vecfile,
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

int
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
    inputline;

double
    fillmargin = 0.5,
    radius = 2.5,
    white = 1,
    xgap = 4.0,
    ygap = 4.0,
    linewidth = .5,
    backred = .9,
    backgreen = .9,
    backblue = .9,
    margin = 1.0e-20,
    aspect = 1.0,
    islandr = 10,
    islandlw = 1,
    xp,
    yp;

BOOL
    equiscale = FALSE,
    noscale = FALSE,
    uselimit = FALSE,
    useeofill = FALSE;

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
    *s_realloc (void *block, size_t size);

char
    *getstring (char const *s),
    *get_arg (void),
    *s_strdup (char const *s),
    *tostr (int i);

char const
    *unquote (char *s),
    *psstring (unsigned char const *s);

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
	*ii,
        i,
        j,
        j1,
        j2,
        n,
        v;
    double
        f,
        rgb [3],
        rmin, rmax,
        gmin, gmax,
        bmin, bmax,
        x,
        y,
        dx,
        dy,
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
    vecfile = arg_v [2];

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
        } else if (! memcmp (buffer, "backrgb:", 8)) {
            if (sscanf (buffer + 8, " %lf %lf %lf", &backred, &backgreen, &backblue) != 3)
                errit ("Missing value(s) for 'backrgb' in file \"%s\", line %i", configfile, inputline);
	    if (backred < 0.0 || backred > 1.0)
		errit ("Red value out of range for 'backrgb' in file \"%s\", line %i", configfile, inputline);
	    if (backgreen < 0.0 || backgreen > 1.0)
		errit ("Green value out of range for 'backrgb' in file \"%s\", line %i", configfile, inputline);
	    if (backblue < 0.0 || backblue > 1.0)
		errit ("Blue value out of range for 'backrgb' in file \"%s\", line %i", configfile, inputline);
	} else if (! memcmp (buffer, "borderwidth:", 12)) {
	    ;
        } else if (! memcmp (buffer, "borderrgb:", 10)) {
            ;
	} else if (! memcmp (buffer, "symbolsize:", 11)) {
	    ;
	} else if (! memcmp (buffer, "symbollinewidth:", 16)) {
	    ;
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
            islandr = atof (buffer + 8);
            if (islandr <= 0.0)
                errit ("Illegal value for 'islandr' in file \"%s\", line %i", configfile, inputline);
        } else if (! memcmp (buffer, "islandlw:", 9)) {
            islandlw = atof (buffer + 9);
            if (islandlw < 0.0)
                errit ("Illegal value for 'islandlw' in file \"%s\", line %i", configfile, inputline);
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
        names [n_names].red = names [n_names].green = names [n_names].blue =
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

    rmin = gmin = bmin = FLT_MAX;
    rmax = gmax = bmax = -FLT_MAX;
    fp = r_fopen (vecfile);
    GetLine (TRUE, fp);
    if (atoi (buffer) != 3)
        errit ("Illegal vector size in file \"%s\", line %i", vecfile, inputline);
    while (GetLine (FALSE, fp)) {
        for (i = 0; i < n_names; i++)
            if (! strcmp (names [i].s, buffer))
                break;
        if (i == n_names)
            errit (
                "Label \"%s\" in file \"%s\" not found in file \"%s\"",
                buffer,
                vecfile,
                labelfile
            );
        for (j = 0; j < 3; j++) {
            GetLine (TRUE, fp);
            if (sscanf (buffer, "%lf", &f) < 1)
                errit ("Missing value in file \"%s\", line %i", vecfile, inputline);
            rgb [j] = f;
	    if (noscale) {
		if (f < 0.0 || f > 1.0) {
		    errit ("Value %g out of range in file \"%s\", line %i", f, vecfile, inputline);
		}
	    }
        }
        names [i].data = TRUE;
        names [i].red   = rgb [0];
        names [i].green = rgb [1];
        names [i].blue  = rgb [2];
        if (rgb [0] < rmin)
            rmin = rgb [0];
        if (rgb [0] > rmax)
            rmax = rgb [0];
        if (rgb [1] < gmin)
            gmin = rgb [1];
        if (rgb [1] > gmax)
            gmax = rgb [1];
        if (rgb [2] < bmin)
            bmin = rgb [2];
        if (rgb [2] > bmax)
            bmax = rgb [2];
    }
    fclose (fp);

    if (equiscale) {
	f = rmax - rmin;
	if (gmax - gmin > f)
	    f = gmax - gmin;
	if (bmax - bmin > f)
	    f = bmax - bmin;
	rmin = (rmin + rmax) / 2.0 - f / 2.0;	rmax = rmin + f;
	gmin = (gmin + gmax) / 2.0 - f / 2.0;	gmax = gmin + f;
	bmin = (bmin + bmax) / 2.0 - f / 2.0;	bmax = bmin + f;
    }

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

    for (i = 0; i < n_names; i++) {
        if (names [i].v < -1) {
            if (names [i].data)
                names [i].usage = USE;
            else
                names [i].usage = default_absent;
        } else {
	    if (names [i].v & 64)
		names [i].usage = SKIP;
            else if (names [i].v >= 0)
                names [i].usage = names [i].data ? USE : PRESERVE;
            else
                names [i].usage = SKIP;
        }
    }

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

        ptmp = (POINT *) s_malloc (sizeof (POINT));
        ptmp->x = xmax + (xmax - xmin) * fillmargin;
        ptmp->y = ymin - (ymax - ymin) * fillmargin;
        ptmp->next = point [i];
        point [i] = ptmp;

        ptmp = (POINT *) s_malloc (sizeof (POINT));
        ptmp->x = xmax + (xmax - xmin) * fillmargin;
        ptmp->y = ymax + (ymax - ymin) * fillmargin;
        ptmp->next = point [i];
        point [i] = ptmp;

        ptmp = (POINT *) s_malloc (sizeof (POINT));
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
    fprintf (fp_out, "%s, (c) P. Kleiweg 1998 - 2010\n", programname);
    fputs ("%%CreationDate: ", fp_out);
    time (&tp);
    fputs (asctime (localtime (&tp)), fp_out);
    fputs ("%%Title: ", fp_out);
    fprintf (fp_out, "%s\n", vecfile);
    if (pslevel > 1) {
        fputs ("%%LanguageLevel: ", fp_out);
        fprintf (fp_out, "%i\n", pslevel);
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
    fputs (
        "/SETCOLOR { setrgbcolor } bind def     % red / green / blue\n"
        "%/SETCOLOR { sethsbcolor } bind def     % hue / saturation / brightness\n"
        "%/SETCOLOR { 0 setcmykcolor } bind def  % cyan / magenta / yellow / black (=0)\n"
        "                                       % NOTE: the last one requires PS Level 2\n"
        "\n"
        "% In what order should the rgb values be used\n"
        "% 1: First input column\n"
        "% 2: Second input column\n"
        "% 3: Third input column\n"
        "% 4: This component always 0\n"
        "% 5: This component always 1\n"
        "/Order [ 1 2 3 ] def\n"
        "\n"
        "% Colour components that should be reversed\n"
        "/C1reverse false def\n"
        "/C2reverse false def\n"
        "/C3reverse false def\n"
        "\n",
        fp_out
    );
    fprintf (
        fp_out,
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
	"\n"
	"/Islandr %g def\n"
	"/Islandlw %g def \n"
        "\n",
	backred,
	backgreen,
	backblue,
        fontname,
        fontmatrix,
        radius, '%',
        white, '%',
        xgap, '%',
        ygap, '%',
        linewidth,
	islandr,
	islandlw
    );
    if (uselimit || ! clipfile)
        fprintf (
            fp_out,
            "/Limit %i def\n"
            "\n",
            limit
        );
    fputs (
        "%%%% End of User Options %%%%\n"
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
        "/Ord {\n"
        "    0 1\n"
        "    5 array astore\n"
        "    dup Order 0 get 1 sub get C1reverse { 1 exch sub } if exch\n"
        "    dup Order 1 get 1 sub get C2reverse { 1 exch sub } if exch\n"
        "    Order 2 get 1 sub get C3reverse { 1 exch sub } if\n"
        "} bind def\n"
        "\n"
        "% [ X Y Map (name) dx dy p r g b ]\n"
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
        "%    64: draw data island\n"
        "%   128: draw data island as diamond (requires 64)\n"
        "% r, g, b: Red, green, blue colour values\n",
        fp_out
    );
    fprintf (
        fp_out,
        "/default %i def\n"
        "/PP [\n",
        itemdefault
    );
    for (i = 0; i < n_names; i++) {
        if (names [i].usage == USE || (names [i].v > -1 && names [i].v & 64)) {
            if (names [i].v < -1)
                s = "default";
            else
                s = tostr (names [i].v);
            fprintf (
                fp_out,
                "  [ %g %g Map (%s) %g %g %s %g %g %g Ord ]\n",
                names [i].x,
                names [i].y,
                psstring ((unsigned char *)names [i].s),
                names [i].dx,
                names [i].dy,
                s,
                noscale ? names [i].red   : (names [i].red   - rmin) / (rmax - rmin),
                noscale ? names [i].green : (names [i].green - gmin) / (gmax - gmin),
                noscale ? names [i].blue  : (names [i].blue  - bmin) / (bmax - bmin)
            );
        } else
            fputs ("  [ 0 0 () 0 0 0 0 0 0 ]\n", fp_out);
    }

    fputs (
        "] def\n"
        "\n"
        "/NR PP length 1 sub def\n"
        "\n"
        "/Str 60 string def\n"
        "/dot1 {\n"
        "    dup /nmbr exch def\n"
        "    PP exch get aload pop\n"
        "    /b exch def\n"
        "    /g exch def\n"
        "    /r exch def\n"
        "    /p exch def\n"
        "    /dy exch def\n"
        "    /dx exch def\n"
        "    /n exch def\n"
        "    /y exch def\n"
        "    /x exch def\n"
        "    p 64 and 0 ne {\n"
        "        p 128 and 0 ne {\n"
        "            x Islandr add y moveto\n"
        "            x y Islandr add lineto\n"
        "            x Islandr sub y lineto\n"
        "            x y Islandr sub lineto\n"
        "        } {\n"
        "            x Islandr add y moveto\n"
        "            x y Islandr 0 360 arc\n"
        "        } ifelse\n"
        "        closepath\n"
        "        r g b SETCOLOR\n"
        "        Islandlw 0 gt {\n"
        "            gsave\n"
        "                fill\n"
        "            grestore\n"
        "            currentgray Graylimit gt { 0 } { 1 } ifelse setgray\n"
        "            Islandlw setlinewidth\n"
        "            stroke\n"
        "        } {\n"
        "            fill\n"
        "        } ifelse\n"
        "    } if\n"
        "} bind def\n"
	"\n"
        "/dot2 {\n"
        "    dup /nmbr exch def\n"
        "    PP exch get aload pop\n"
        "    /b exch def\n"
        "    /g exch def\n"
        "    /r exch def\n"
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
        "        r g b SETCOLOR\n"
        "        currentgray Graylimit gt { 0 } { 1 } ifelse setgray\n"
        "        show\n"
        "    } if\n"
        "\n"
        "    0 setgray\n"
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

    if (clipfile && ! uselimit)
        fputs (
            "/poly {\n"
            "    newpath\n"
            "    Map moveto\n"
            "    counttomark 2 idiv { Map lineto } repeat\n"
            "    closepath\n"
            "    pop\n"
            "    PP exch get\n"
            "    aload pop SETCOLOR\n"
            "    8 and 0 eq {\n"
            "        fill\n"
            "    } {\n"
            "        gsave fill grestore\n"
            "        currentgray Graylimit gt { 0 } { 1 } ifelse\n"
            "        setgray\n"
            "        stroke\n"
            "    } ifelse\n"
            "    5 { pop } repeat\n"
            "} bind def\n",
            fp_out
        );
    else
        fputs (
            "/poly {\n"
            "    gsave\n"
            "        counttomark\n"
            "        1 add index\n"
            "        PP exch get\n"
            "        dup 0 get\n"
            "        exch 1 get\n"
            "        2 copy\n"
            "        exch Limit add exch moveto\n"
            "        Limit 0 360 arc\n"
            "        closepath clip\n"
            "        newpath\n"
            "        Map moveto\n"
            "        counttomark 2 idiv { Map lineto } repeat\n"
            "        closepath\n"
            "        pop\n"
            "        PP exch get\n"
            "        aload pop SETCOLOR\n"
            "        8 and 0 eq {\n"
            "            fill\n"
            "        } {\n"
            "            gsave fill grestore\n"
            "            currentgray Graylimit gt { 0 } { 1 } ifelse\n"
            "            setgray\n"
            "            stroke\n"
            "        } ifelse\n"
            "        5 { pop } repeat\n"
            "    grestore\n"
            "} bind def\n",
            fp_out
        );

    fputs (
        "Linewidth setlinewidth\n",
        fp_out
    );
    for (i = 0; i < n_names; i++)
        if (names [i].usage == USE) {
            fprintf (fp_out, "%i mark\n", i);
            for (ptmp = point [i]; ptmp != NULL; ptmp = ptmp->next)
                fprintf (fp_out, "%g %g\n", ptmp->x, ptmp->y);
            fputs ("poly\n", fp_out);
        }
    fputs ("\n", fp_out);

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
	"theFont setfont\n"
        "0 setlinejoin\n"
        "0 1 NR { dot1 } for\n"
        "0 1 NR { dot2 } for\n"
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
            if (EQUAL (xp2, p4->x) && EQUAL (yp2, p4->y))
                p1->next = p4;
            else {
                p1->next = p3;
                p3->x = xp2;
                p3->y = yp2;
            }
        } else {
            /* p2 at inside */
            if (EQUAL (xp2, p3->x) && EQUAL (yp2, p3->y))
                p3->next = NULL;
            else if (p3->next) {
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
            p4->x = xp2;
            p4->y = yp2;
            p4->next = NULL;
        } else {
            /* p1 at inside */
            if (EQUAL (xp2, p3->x) && EQUAL (yp2, p3->y))
		/* this shouldn't happen */
                return;
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
		}
		p2->next = p3;
		p3->x = xp2;
		p3->y = yp2;
	    }
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
        if (buffer [i] == '#')
            continue;
        if (buffer [i]) {
            memmove (buffer, buffer + i, strlen (buffer) + 1);
            return TRUE;
        }
    }
}

void process_args ()
{
    while (arg_c > 1 && arg_v [1][0] == '-') {
        switch (arg_v [1][1]) {
	    case 'e':
		equiscale = TRUE;
		noscale = FALSE;
		break;
	    case 'r':
		equiscale = FALSE;
		noscale = TRUE;
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

void syntax ()
{
    fprintf (
        stderr,
        "\n"
        "Colour Map Drawing, Version " my_VERSION "\n"
        "(c) P. Kleiweg 1998 - 2010\n"
        "\n"
        "Usage: %s [-e] [-r] [-o filename] configfile vectorfile\n"
        "\n"
	"\t-e : equivalent scaling of colour axes\n"
	"\t-r : raw input, no scaling of colour axes\n"
        "\t-o : output file\n"
        "\n",
        programname
    );
    exit (1);
}
