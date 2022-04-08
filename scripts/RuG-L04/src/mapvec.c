/*
 * File: mapvec.c
 *
 * (c) Peter Kleiweg
 *     2005 - 2008
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

typedef struct {
    char
        *s;
    double
        x,
        y,
        xv,
        yv;
    BOOL
        coo,
        useIt;
} NAME;

typedef struct {
    double
        lnk,
        geo;
    BOOL
        useIt;
} LNK;

NAME
    *names;

LNK
    **diff = NULL;

#define BUFSIZE 2048

BOOL
    commentsave = FALSE,
    useeofill = FALSE;

char
    **comments = NULL,
    **arg_v,
    *diffile,
    *configfile,
    *outfile = NULL,
    *transformfile = NULL,
    *coordfile = NULL,
    *mapfile = NULL,
    *clipfile = NULL,
    buffer [BUFSIZE + 1],
    buf    [BUFSIZE + 1],
    *programname,
    *no_mem_buffer,
    out_of_memory [] = "Out of memory!";

#define MAXLNK .125

int
    n_comments = 0,
    max_comments = 0,
    llx = 0,
    lly = 0,
    urx = 595,
    ury = 842,
    arg_c,
    n_names = 0,
    max_names = 0,
    inputline;

#define MINSIZE 0.1
#define MARGIN 0.01

double
    maxlnk = MAXLNK,
    minsize = MINSIZE,
    margin = MARGIN,
    radius = 2.5,
    white = 1,
    backred = .9,
    backgreen = .9,
    backblue = .9,
    aspect = 1.0;

time_t
    tp;

FILE
    *fp_out;

void
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
    *s_strdup (char const *s);

char const
    *unquote (char *s);

int
    geocmp (void const *p1, void const *p2);

double
    diffgeo (int i, int j);

BOOL
    has_argument (void),
    GetLine (BOOL required, FILE *fp);

FILE
    *r_fopen (char const *filename);

int main (int argc, char *argv [])
{
    int
        i,
        j,
        k,
        n;
    double
        x,
        y,
        dx,
        dy,
        dfx,
        dfy,
        f,
	max,
	sum,
	dmin,
	dmax,
	diffgeomax = 0.0;
    FILE
        *fp;

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
        } else if (! memcmp (buffer, "markers:", 6))
            ;
        else if (! memcmp (buffer, "missing:", 8))
            ;
        else if (! memcmp (buffer, "limit:", 6))
            ;
	else if (! memcmp (buffer, "uselimit:", 9))
	    ;
        else if (! memcmp (buffer, "boundingbox:", 12)) {
            if (sscanf (buffer + 12, " %i %i %i %i", &llx, &lly, &urx, &ury) != 4)
                errit ("Missing value(s) for 'boundingbox' in file \"%s\", line %i", configfile, inputline);
        } else if (! memcmp (buffer, "fontname:", 9))
            ;
        else if (! memcmp (buffer, "fontmatrix:", 11))
            ;
        else if (! memcmp (buffer, "pslevel:", 8))
            ;
	else if (! memcmp (buffer, "fillstyle:", 10)) {
            if (strstr (buffer + 10, "eofill") != NULL)
		useeofill = TRUE;
	} else if (! memcmp (buffer, "fillmargin:", 11))
	    ;
        else if (! memcmp (buffer, "radius:", 7)) {
            radius = atof (buffer + 7);
            if (radius <= 0.0)
                errit ("Illegal value for 'radius' in file \"%s\", line %i", configfile, inputline);
        } else if (! memcmp (buffer, "white:", 6)) {
            white = atof (buffer + 6);
            if (white <= 0.0)
                errit ("Illegal value for 'white' in file \"%s\", line %i", configfile, inputline);
        } else if (! memcmp (buffer, "xgap:", 5))
            ;
        else if (! memcmp (buffer, "ygap:", 5))
            ;
        else if (! memcmp (buffer, "linewidth:", 10)) 
            ;
        else if (! memcmp (buffer, "borderwidth:", 12))
            ;
        else if (! memcmp (buffer, "backrgb:", 8)) {
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
        } else if (! memcmp (buffer, "symbolsize:", 11))
            ;
        else if (! memcmp (buffer, "symbollinewidth:", 16))
            ;
        else if (! memcmp (buffer, "transform:", 10)) {
            transformfile = getstring (buffer + 10);
        } else if (! memcmp (buffer, "clipping:", 9)) {
            clipfile = getstring (buffer + 9);
        } else if (! memcmp (buffer, "map:", 4)) {
            mapfile = getstring (buffer + 4);
        } else if (! memcmp (buffer, "labels:", 7))
            ;
        else if (! memcmp (buffer, "coordinates:", 12)) {
            coordfile = getstring (buffer + 12);
        } else if (! memcmp (buffer, "othermarkers:", 13))
            ;
        else if (! memcmp (buffer, "islandr:", 8)) {
            ;
        } else if (! memcmp (buffer, "islandlw:", 9)) {
            ;
        } else
            errit ("Unknown option in file \"%s\", line %i:\n%s\n", configfile, inputline, buffer);
    }
    fclose (fp);

    if (coordfile == NULL)
        errit ("Missing 'coordinates' in file \"%s\"\n", configfile);

    if (transformfile == NULL)
        errit ("Missing 'transform' in file \"%s\"\n", configfile);

    fp = r_fopen (diffile);
    GetLine (TRUE, fp);
    n_names = atoi (buffer);
    if (n_names < 2)
        errit ("Illegal number of items in \"%s\": %i", diffile, n_names);
    names = (NAME *) s_malloc (n_names * sizeof (NAME));
    for (i = 0; i < n_names; i++) {
        GetLine (TRUE, fp);
        names [i].s = s_strdup (buffer);
        names [i].coo = FALSE;
    }

    dmin = FLT_MAX;
    dmax = -FLT_MAX;
    diff = (LNK **) s_malloc (n_names * sizeof (LNK *));
    for (i = 0; i < n_names; i++)
        diff [i] = (LNK *) s_malloc (n_names * sizeof (LNK));
    for (i = 0; i < n_names; i++) {
        diff [i][i].useIt = FALSE;
        for (j = 0; j < i; j++) {
            GetLine (TRUE, fp);
            if (sscanf (buffer, "%lf", &f) != 1) {
                warn ("Missing value [%i,%i], file \"%s\", line %i", j + 1, i + 1, diffile, inputline);
                diff [i][j].useIt = diff [j][i].useIt = FALSE;
            } else {
		diff [i][j].lnk = diff [j][i].lnk = f;
		if (f > dmax)
		    dmax = f;
		if (f < dmin)
		    dmin = f;
                diff [i][j].useIt = diff [j][i].useIt = TRUE;
	    }
        }
    }
    fclose (fp);
    if (dmin / dmax < margin) {
	f = (margin * dmax - dmin) / ( 1.0 - margin);
	warn ("Adjusting input differences: [%g, %g] => [%g, %g]", dmin, dmax, dmin + f, dmax + f);
	for (i = 0; i < n_names; i++)
	    for (j = 0; j < i; j++)
		if (diff [i][j].useIt) {
		    diff [i][j].lnk += f;
		    diff [j][i].lnk += f;
		}
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
            continue;
        names [i].x = x;
        names [i].y = y;
        names [i].coo = TRUE;
    }
    fclose (fp);

    for (i = 0; i < n_names; i++)
        if (! names [i].coo)
            errit ("No position defined for \"%s\"", names [i].s);

    diffgeomax = 0.0;
    for (i = 0; i < n_names; i++) {
        diff [i][i].geo = 0.0;
        for (j = 0; j < i; j++) {
	    f = diffgeo (i, j);
            diff [i][j].geo = diff [j][i].geo = f;
	    if (diff [i][j].useIt && f > diffgeomax)
		diffgeomax = f;
	}
    }

    for (i = 0; i < n_names; i++) {
	k = 0;
        dx = dy = 0.0;
        for (j = 0; j < n_names; j++)
	    if (diff [i][j].useIt && diff [i][j].geo <= diffgeomax * maxlnk) {
		dx += names [j].x - names [i].x;
		dy += names [j].y - names [i].y;
		k++;
	    }

	if (k) {
	    dx /= k;
	    dy /= k;
	    dfx = dfy = 0.0;
	    for (j = 0; j < n_names; j++)
		if (diff [i][j].useIt && diff [i][j].geo <= diffgeomax * maxlnk) {
		    f = diff [i][j].lnk;
		    dfx += (names [j].x - names [i].x - dx) / f;
		    dfy += (names [j].y - names [i].y - dy) / f;
		}
	    names [i].xv = dfx / aspect;
	    names [i].yv = dfy;
	} else {
	    names [i].xv = 0;
	    names [i].yv = 0;
	}
    }

    max = 0.0;
    for (i = 0; i < n_names; i++) {
	f = sqrt (names [i].xv * names [i].xv + names [i].yv * names [i].yv);
	if (f > max)
	    max = f;
    }
    max *= minsize;
    n = 0;
    sum = 0.0;
    for (i = 0; i < n_names; i++) {
	f = sqrt (names [i].xv * names [i].xv + names [i].yv * names [i].yv);
	if (f >= max) {
	    n++;
	    sum += f;
	    names [i].useIt = TRUE;
	} else
	    names [i].useIt = FALSE;
    }
    sum /= n;
    for (i = 0; i < n_names; i++) {
        names [i].xv /= sum;
        names [i].yv /= sum;
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
    fprintf (fp_out, "%s, (c) P. Kleiweg 2005 - 2008\n", programname);
    fputs ("%%CreationDate: ", fp_out);
    time (&tp);
    fputs (asctime (localtime (&tp)), fp_out);
    fputs ("%%Title: ", fp_out);
    fprintf (fp_out, "%s | f = %g | m = %g | n = %g\n", diffile, minsize, margin, maxlnk);
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
        "/Backrgb [ %g %g %g ] def\n"
        "\n"
        "/Radius %g def  %c Radius of dot\n"
        "/White %g def   %c Linewidth around dot\n"
        "\n"
        "/VectorWidth 1 def\n"
        "/DotColour [ 0 0 1 ] def\n"
        "/VScale 10 def\n"
        "\n",
        backred,
        backgreen,
        backblue,
        radius, '%',
        white, '%'
    );

    fputs (
        "%%%% End of User Options %%%%\n"
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
        "% [ X Y Map xv yv true ]\n"
        "% [ X Y Map false ]\n"
        "%\n"
        "% X, Y: Map coordinates\n"
        "% Map: This translates map coordinates into screen coordinates\n"
        "% xv, yv: vector\n"
        "/PP [\n",
        fp_out
    );
    for (i = 0; i < n_names; i++)
	if (names [i].useIt)
 	    fprintf (
	        fp_out,
		"  [ %g %g Map %g %g true ]\n",
		names [i].x,
		names [i].y,
		names [i].xv,
		names [i].yv
	    );
	else
 	    fprintf (
	        fp_out,
		"  [ %g %g Map false ]\n",
		names [i].x,
		names [i].y
	    );

    fputs (
        "] def\n"
        "\n"
        "/NR PP length 1 sub def\n"
        "\n"
        "/dot {\n"
        "    dup /nmbr exch def\n"
        "    PP exch get aload pop\n"
	"    /u exch def\n"
	"    u {\n"
        "        /yv exch def\n"
        "        /xv exch def\n"
	"    } if\n"
        "    /y exch def\n"
        "    /x exch def\n"
        "    x Radius add y moveto\n"
        "    x y Radius 0 360 arc\n"
        "    closepath\n"
        "    gsave\n"
        "        White 2 mul setlinewidth\n"
        "        1 setgray\n"
        "        stroke\n"
	"        u {\n"
        "            0 setgray\n"
        "            VectorWidth setlinewidth\n"
        "            x y moveto\n"
        "            xv VScale mul\n"
        "            yv VScale mul rlineto\n"
        "            stroke\n"
	"        } if\n"
        "    grestore\n"
        "    fill\n"
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
            "Backrgb aload pop setrgbcolor %sfill\n"
            "0 setgray\n"
            "\n",
            useeofill ? "eo" : ""
        );
    }

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
        "DotColour aload pop setrgbcolor\n"
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
	    case 'f':
		minsize = atof (get_arg ());
		if (minsize <= 0.0 || minsize >= 1.0)
		    errit ("Value for -f must be between 0.0 and 1.0, exclusive");
		break;
	    case 'm':
		margin = atof (get_arg ());
		if (margin <= 0.0 || margin >= 1.0)
		    errit ("Value for -m must be between 0.0 and 1.0, exclusive");
		break;
            case 'n':
		maxlnk = atof (get_arg ());
		if (maxlnk <= 0.0 || maxlnk > 1.0)
		    errit ("Value for -m must be between 0.0 (exclusive) and 1.0 (inclusive)");
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

int geocmp (void const *p1, void const *p2)
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

double diffgeo (int i, int j)
{
    double
        dx,
        dy;

    dx = (names [i].x - names [j].x) / aspect;
    dy = names [i].y - names [j].y;
    return sqrt (dx * dx + dy * dy);

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
        "Vector Map Drawing, Version " my_VERSION "\n"
        "(c) P. Kleiweg 2005 - 2008\n"
        "\n"
        "Usage: %s [-f float] [-m float] [-n int] [-o filename] configfile differencefile\n"
        "\n"
	"\t-f : limit fraction (default: %g)\n"
	"\t-m : required minimal difference as fraction of maximum (default: %g)\n"
        "\t-n : neighbourhood size (default: %g)\n"
        "\t-o : output file\n"
        "\n",
        programname,
	(float) MINSIZE,
	(float) MARGIN,
        (float) MAXLNK
    );
    exit (1);
}
