/*
 * File: ll2grid.c
 *
 * (c) Peter Kleiweg
 *     Wed May  8 20:35:11 2002
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2,
 * or (at your option) any later version.
 *
 */

#define my_VERSION "0.06"

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
#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define BUFSIZE 4095

int
    arg_c,
    n;

char
    *infile,
    *outfile = NULL,
    buffer [BUFSIZE + 1],
    **arg_v,
    *programname;

double
    lin = 0.5,
    long0,
    lat0,
    d,
    r,
    fx,
    fy,
    x,
    xx0,
    xx,
    xy,
    y,
    yy0,
    yy,
    ya,
    yb,
    z,
    zz0,
    zz,
    za,
    zb,
    zc;

FILE
    *fp_in,
    *fp_out;

void
    get_programname (char const *argv0),
    process_args (void),
    errit (char const *format, ...),
    syntax (void);
char
    *get_arg (void);

int main (int argc, char *argv [])
{
    get_programname (argv [0]);

    arg_c = argc;
    arg_v = argv;
    process_args ();

    if (arg_c != 4)
	syntax ();

    long0 = atof (arg_v [1]);
    lat0 = atof (arg_v [2]);
    infile = arg_v [3];

    if (long0 < -180 || long0 > 180)
	errit ("longitude out of range, must be between -180 and 180");

    if (lat0 < -90 || lat0 > 90)
	errit ("latitude out of range, must be between -90 and 90");

    long0 = long0 / 180.0 * M_PI;
    lat0  = lat0  / 180.0 * M_PI;

    xx0 = cos (long0) * cos (lat0);
    yy0 = sin (long0) * cos (lat0);
    zz0 = sin (lat0);
    
    ya = sin(-long0);
    yb = cos(-long0);
    za = cos(-long0) * sin(-lat0);
    zb = -1.0 * sin(-long0) * sin(-lat0);
    zc = cos(-lat0);

    fp_in = fopen (infile, "r");
    if (! fp_in)
	errit ("Opening file \"%s\": %s", infile, strerror (errno));

    if (outfile) {
	fp_out = fopen (outfile, "w");
	if (! fp_out)
	    errit ("Creating file \"%s\": %s", outfile, strerror (errno));	
    } else
	fp_out = stdout;

    while (fgets (buffer, BUFSIZE, fp_in)) {
	if (sscanf (buffer, " %lf %lf%n", &x, &y, &n) < 2) {
	    fputs (buffer, fp_out);
	    continue;
	}

	x = x / 180.0 * M_PI;
	y = y / 180.0 * M_PI;

	xx = cos (x) * cos (y);
	yy = sin (x) * cos (y);
	zz = sin (y);
	fx = ya * xx + yb * yy;
	fy = za * xx + zb * yy + zc * zz;
      
	d = xx0 * xx + yy0 * yy + zz0 * zz;
	if (d < -1) d = -1;
	if (d >  1) d =  1;
	d = acos (d);

	d = lin * d + (1.0 - lin) * sin (d);

	if (fx > -.00001 && fx < .00001 && fy > -.00001 && fy < .00001) {
	    fx = fx / M_PI * 180.0;
	    fy = fy / M_PI * 180.0;
	} else {
	    r = atan2 (fy, fx);
	    fx = d * cos (r) / M_PI * 180.0;
	    fy = d * sin (r) / M_PI * 180.0;
	}

	fprintf (fp_out, "%g %g%s", fx, fy, buffer + n);
	
    }

    return 0;
}

void process_args ()
{
    while (arg_c > 1 && arg_v [1][0] == '-') {
        switch (arg_v [1][1]) {
	    case '0':
	    case '1':
	    case '2':
	    case '3':
	    case '4':
	    case '5':
	    case '6':
	    case '7':
	    case '8':
	    case '9':
	    case '.':
		return;
            case 'o':
                outfile = get_arg ();
                break;
            case 'p':
		lin = atof (get_arg ());
		if (lin < 0.0 || lin > 1.0)
		    errit ("-p out of range");
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
	"Longitude / latitude to rectangular grid convertor, Version " my_VERSION "\n"
	"(c) P. Kleiweg 2002\n"
	"\n"
	"Usage: %s [-o filename] [-p float] longitude latitude filename\n"
	"\n"
	"\t-o : output file\n"
	"\t-p : projection, a value between 0 and 1 (default: 0.5)\n"
	"\n",
	programname
    );
    exit (1);
}
