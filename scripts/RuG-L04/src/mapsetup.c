/*
 * File: mapsetup.c
 *
 * (c) Peter Kleiweg 2002 - 2004
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2,
 * or (at your option) any later version.
 *
 */

#define my_VERSION "0.07"

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
#include <errno.h>
#include <float.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define BUFSIZE 4095

typedef enum { FALSE = 0, TRUE } BOOL;

typedef enum { cooLL, cooXY } cooTYPE;

cooTYPE
    cooType = cooXY;

int
    width = 595,
    height = 842,
    margin = 0,
    arg_c;

double
    aspect = 1,
    xmin = FLT_MAX,
    xmax = -FLT_MAX,
    ymin = FLT_MAX,
    ymax = -FLT_MAX;

char
    buffer [BUFSIZE + 1],
    buf1 [BUFSIZE + 1],
    buf2 [BUFSIZE + 1],
    *clipping = NULL,
    *map = NULL,
    *locations = NULL,
    **arg_v,
    *programname;

FILE
    *fp_in,
    *fp_out;

void
    scanfile (char const *filename),
    openin (char const *filename),
    openout (char const *filename),
    processmap (BOOL draw),
    printcomment (char *s, BOOL always_newline),
    get_programname (char const *argv0),
    process_args (void),
    errit (char const *format, ...),
    syntax (void);
char
    *get_arg (void);

int main (int argc, char *argv [])
{
    double
        y1,
        y2;

    get_programname (argv [0]);

    if (argc == 1)
        syntax ();

    arg_c = argc;
    arg_v = argv;
    process_args ();

    if (arg_c > 1)
        syntax ();

    if (clipping == NULL && map == NULL && locations == NULL)
        syntax ();

    if (clipping)
        scanfile (clipping);

    if (map)
        scanfile (map);

    if (locations)
        scanfile (locations);

    if (cooType == cooLL) {
        y1 = ymin / 180.0 * M_PI;
        y2 = ymax / 180.0 * M_PI;
	if (y2 - y1 < .001)
	    aspect = 1 / cos ((y1 + y2) / 2);
	else
	    aspect = (y2 - y1) / (sin (y2) - sin (y1));
    }

    if (margin <= 0) {
        if (clipping != NULL || map != NULL)
            margin = 100;
        else
            margin = 150;
    }

    openout ("out.trn");
    fputs (
        "% Min/max x and y coordinates\n",
        fp_out
    );
    fprintf (
        fp_out,
        "/X1 %g def\n"
        "/X2 %g def\n"
        "/Y1 %g def\n"
        "/Y2 %g def\n",
        xmin,
        xmax,
        ymin,
        ymax
    );
    fputs (
        "\n"
        "% Papersize and margin\n",
        fp_out
    );
    fprintf (
        fp_out,
        "/HSize %i def\n"
        "/VSize %i def\n"
        "/Margin %i def\n",
        width,
        height,
        margin
    );
    fputs (
        "\n"
        "% Center of x and y coordinates becomes origin\n"
        "X1 X2 add 2 div neg\n"
        "Y1 Y2 add 2 div neg\n"
        "matrix translate\n"
        "\n"
        "% Adjust aspect ratio between x and y coordinates\n"
        "1 Aspect\n"
        "matrix scale\n"
        "matrix concatmatrix\n"
        "\n"
        "% Adjust scaling for map to fit on paper\n"
        "% Limit by X2 - X1 or by Y2 - Y1 ?\n"
        "HSize Margin 2 mul sub            X2 X1 sub div\n"
        "VSize Margin 2 mul sub Aspect div Y2 Y1 sub div\n"
        "2 copy gt { exch } if pop\n"
        "dup\n"
        "matrix scale\n"
        "matrix concatmatrix\n"
        "\n"
        "% Reposition to center of paper\n"
        "HSize 2 div\n"
        "VSize 2 div\n"
        "matrix translate\n"
        "matrix concatmatrix\n"
        "\n"
        "/MapMatrix exch def\n"
        "\n"
        "/Map {\n"
        "    MapMatrix transform\n"
        "} bind def\n"
        "\n"
        "% % Testing\n"
        "% X1 Y1 Map moveto\n"
        "% X2 Y1 Map lineto\n"
        "% X2 Y2 Map lineto\n"
        "% X1 Y2 Map lineto\n"
        "% closepath\n"
        "% stroke\n"
        "% showpage\n",
        fp_out
    );
    fclose (fp_out);
    printf ("transform saved to: out.trn\n");

    if (clipping) {
        openin (clipping);
        openout ("out.clp");
        processmap (FALSE);
        fclose (fp_in);
        fclose (fp_out);
        printf ("clipping saved to: out.clp\n");
    }

    if (map) {
        openin (map);
        openout ("out.map");
        processmap (TRUE);
        fclose (fp_in);
        fclose (fp_out);
        printf ("map saved to: out.map\n");
    }

    if (aspect > 1.0)
	printf ("\nDon't forget to put the following line in your configuration file:\n\naspect: %g\n\n", aspect);

    return 0;
}

void scanfile (char const *filename)
{
    double
        x,
        y;
    FILE
        *fp;

    fp = fopen (filename, "r");
    if (! fp)
        errit ("Opening file \"%s\": %s", filename, strerror (errno));

    while (fgets (buffer, BUFSIZE, fp) != NULL) {
        if (sscanf (buffer, " %lf %lf", &x, &y) == 2) {
            if (x > xmax)
                xmax = x;
            if (x < xmin)
                xmin = x;
            if (y > ymax)
                ymax = y;
            if (y < ymin)
                ymin = y;
        }
    }
    fclose (fp);
}

void openout (char const *filename)
{
    if (access (filename, F_OK) == 0)
        errit ("File exists: \"%s\"", filename);
    fp_out = fopen (filename, "w");
    if (! fp_out)
        errit ("Creating file \"%s\": %s", filename, strerror (errno));
}

void openin (char const *filename)
{
    fp_in = fopen (filename, "r");
    if (! fp_in)
        errit ("Openinb file \"%s\": %s", filename, strerror (errno));    
}

void processmap (BOOL ismap)
{
    double
        x0 = 0,
        y0 = 0,
        x1 = 0,
        y1 = 0,
        x,
        y;
    int
        i,
	n,
        state;
    char
        *s;

    fputs (
        "/M {\n"
        "    Map moveto\n"
        "} bind def\n"
        "\n"
        "/L {\n"
        "    Map lineto\n"
        "} bind def\n"
        "\n"
        "/C {\n"
        "    6 2 roll Map\n"
        "    6 2 roll Map\n"
        "    6 2 roll Map\n"
        "    curveto\n"
        "} bind def\n"
        "\n",
        fp_out
    );

    if (ismap)
        fputs (
            "/Stroke {\n"
            "    gsave\n"
            "        1.5 setlinewidth\n"
            "        1 setgray\n"
            "        stroke\n"
            "    grestore\n"
            "    stroke\n"
            "} bind def\n"
            "\n"
            "1 setlinecap\n"
            "1 setlinejoin\n"
            ".5 setlinewidth\n"
            "0 setgray\n"
            "\n",
            fp_out
        );

    state = 0;
    for (;;) {
        s = fgets (buffer, BUFSIZE, fp_in);
	if (s != NULL) {
	    i = strlen (buffer);
	    while (i && isspace ((unsigned char) buffer [i - 1]))
		buffer [--i] = '\0';
	}
        if (s != NULL && sscanf (buffer, " %lf %lf%n", &x, &y, &n) >= 2) {
            switch (state) {
                case 0:
                    fprintf (fp_out, "%g %g M", x, y);
		    printcomment (buffer + n, TRUE);
                    x0 = x;
                    y0 = y;
                    state = 1;
                    break;
                case 1:
		    strcpy (buf1, buffer + n);
                    x1 = x;
                    y1 = y;
                    state = 2;
                    break;
                case 2:
                    fprintf (fp_out, "%g %g L", x1, y1);
		    printcomment (buf1, TRUE);
		    strcpy (buf1, buffer + n);
                    x1 = x;
                    y1 = y;
                    break;
            }
        } else {
            if (state == 2) {
                if (ismap) {
                    if (x1 != x0 || y1 != y0) {
                        fprintf (fp_out, "%g %g L", x1, y1);
			printcomment (buf1, TRUE);
                    } else {
			printcomment (buf1, FALSE);
                        fprintf (fp_out, "closepath\n");
		    }
                    fprintf (fp_out, "Stroke\n");
                } else {
                    if (x1 != x0 || y1 != y0) {
                        fprintf (fp_out, "%g %g L", x1, y1);
			printcomment (buf1, TRUE);
		    } else
			printcomment (buf1, FALSE);
                    fprintf (fp_out, "closepath\n");
                }
            }
            state = 0;
            if (s != NULL) {
		if (sscanf (buffer, " %s %s%n", buf1, buf2, &n) >= 2) {
		    if (strcmp (buf1, "NA") || strcmp (buf2, "NA"))
			n = 0;
		} else
		    n = 0;
		printcomment (buffer + n, FALSE);
		fputc ('\n', fp_out);
            }
        }
        if (s == NULL)
            break;
    }
}

void printcomment (char *s, BOOL always_newline)
{
    int
	i,
	j;

    i = 0;
    while (s [i] && isspace ((unsigned char) s [i]))
	i++;

    j = i;
    while (s [j] == '#')
	s [j++] = '%';

    if (! s [i]) {
	if (always_newline)
	    fputc ('\n', fp_out);
    } else if (s [i] == '%')
	fprintf (fp_out, "\t%s\n", s + i);
    else
	fprintf (fp_out, "\t%c %s\n", '%', s + i);
}

void process_args ()
{
    while (arg_c > 1 && arg_v [1][0] == '-') {
        switch (arg_v [1][1]) {
            case 'L':
                cooType = cooLL;
                break;
            case 'p':
                width = 612;
                height = 792;
                break;
            case 'b':
                margin = atoi (get_arg ());
                break;
            case 'c':
                clipping = get_arg ();
                break;
            case 'm':
                map = get_arg ();
                break;
            case 'l':
                locations = get_arg ();
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

void syntax ()
{
    fprintf (
        stderr,
        "\n"
        "Setup for map drawing, Version " my_VERSION "\n"
	"(c) P. Kleiweg 2002 - 2004\n"
        "\n"
        "Usage: %s [-b int] [-L] [-p] -c clipping [-m map]\n"
        "Usage: %s [-b int] [-L] [-p] -l locations\n"
        "\n"
        "Use clipping if you can, use map if you can.\n"
        "\n"
	"   clipping : the name of a file with path(s) for clipping\n"
	"        map : the name of a file with path(s) for drawing\n"
	"  locations : the name of a file with coordinates of locations\n"
	"\n"
        "  -b : margin, default 100 for clipping, 150 for locations\n"
        "  -L : coordinates are in longitude/latitude\n"
        "       a suitable aspect ratio will be calculated\n"
        "  -p : papersize 'letter', default is 'a4'\n"
        "\n",
        programname,
        programname
    );
    exit (1);
}
