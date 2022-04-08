/*
 * File: mapclust.c
 *
 * (c) Peter Kleiweg
 *     1997 - 2014
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2,
 * or (at your option) any later version.
 *
 */

#define MAXGROUPS 19

#define my_VERSION "0.51"

#define __NO_MATH_INLINES

#ifdef __WIN32__
#  define my_PATH_SEP '\\'
#else
#  define my_PATH_SEP '/'
#endif

#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

typedef enum { FALSE = 0, TRUE = 1} BOOL;

typedef enum { CLS, LBL } NODETYPE;

typedef struct {
    int
        index,
        group [2];
    double
        value;
    char
        *text;
    NODETYPE
        node [2];
    union {
        int
            cluster;
        int
            name;
    } n [2];
} CLUSTER;

CLUSTER
    *cl = NULL;

typedef enum { SKIP, PRESERVE, USE } USAGE_;

typedef struct {
    BOOL
        data;
    USAGE_
        usage;
    int
        i,
        v,
        group;
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
    *dendrofile,
    *indexfile,
    *outfile = NULL,
    *colorfile = NULL,
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
    out_of_memory [] = "Out of memory!",
    *pat [] = {
        "/c0  4 4 {< 7f7fdfdf         >} defpattern\n",
        "/c1  2 2 {< 7fbf             >} defpattern\n",
        "/c2  6 6 {< bfa3bff717f7     >} defpattern\n",
        "/c3  4 4 {< 7fffdfff         >} defpattern\n",
        "/c4  4 4 {< 1fff4fff         >} defpattern\n",
        "/c5  3 4 {< 7f7fbfbf         >} defpattern\n",
        "/c6  3 3 {< 7fbfdf           >} defpattern\n",
        "/c7  4 4 {< 9fff0fff         >} defpattern\n",
        "/c8  8 6 {< 017d55d710ff     >} defpattern\n",
        "/c9  4 6 {< bf5fbfef57ef     >} defpattern\n",
        "/c10 6 4 {< ff17ffa3         >} defpattern\n",
        "/c11 2 4 {< 5fffbfff         >} defpattern\n",
        "/c12 8 8 {< 515f51ff15f515ff >} defpattern\n",
        "/c13 3 4 {< 7f7f7f9f         >} defpattern\n",
        "/c14 6 6 {< 27bf93fb4bef     >} defpattern\n",
        "/c15 4 8 {< bf5f5fbfef5f5fef >} defpattern\n",
        "/c16 5 4 {< 9f6f6fff         >} defpattern\n",
        "/c17 6 6 {< af27ff27afff     >} defpattern\n",
        "/c18 8 8 {< df99fdf7dfccfd7f >} defpattern\n"
    },
    *sym [] = {
        "    Symsize -2 div dup rmoveto\n"
        "    Symsize 0 rlineto\n"
        "    0 Symsize rlineto\n"
        "    Symsize neg 0 rlineto\n"
        "    closepath\n"
        "    gsave 1 setgray fill grestore\n"
        "    stroke\n",

        "    currentpoint\n"
        "    Symsize 2 div 0 rmoveto\n"
        "    Symsize 2 div 0 360 arc\n"
        "    closepath\n"
        "    gsave 1 setgray fill grestore\n"
        "    stroke\n",

        "    Symsize -2 div dup rmoveto\n"
        "    Symsize 0 rlineto\n"
        "    Symsize -2 div Symsize rlineto\n"
        "    closepath\n"
        "    gsave 1 setgray fill grestore\n"
        "    stroke\n",

        "    currentpoint\n"
        "    Symsize -2 div 0 rmoveto\n"
        "    Symsize 0 rlineto\n"
        "    Symsize 2 div sub moveto\n"
        "    0 Symsize rlineto\n"
        "    stroke\n",

        "    Symsize -2 div dup rmoveto\n"
        "    Symsize dup rlineto\n"
        "    0 Symsize neg rmoveto\n"
        "    Symsize neg Symsize rlineto\n"
        "    stroke\n",

        "    Symsize 2 div\n"
        "    dup neg 0 exch rmoveto\n"
        "    dup dup rlineto\n"
        "    dup dup neg exch rlineto\n"
        "    neg dup rlineto\n"
        "    closepath\n"
        "    gsave 1 setgray fill grestore\n"
        "    stroke\n",

        "    Symsize 2 div dup rmoveto\n"
        "    Symsize neg 0 rlineto\n"
        "    Symsize 2 div Symsize neg rlineto\n"
        "    closepath\n"
        "    gsave 1 setgray fill grestore\n"
        "    stroke\n",

        "    currentpoint\n"
        "    sym0\n"
        "    moveto\n"
        "    sym4\n",

        "    currentpoint\n"
        "    sym3\n"
        "    moveto\n"
        "    sym4\n",

        "    currentpoint\n"
        "    sym5\n"
        "    moveto\n"
        "    sym3\n",

        "    currentpoint\n"
        "    sym1\n"
        "    moveto\n"
        "    sym3\n",

        "    gsave\n"
        "        1 setgray\n"
        "        currentpoint\n"
        "        0 Symsize -2 div rmoveto\n"
        "        Symsize 2 div Symsize rlineto\n"
        "        Symsize neg 0 rlineto\n"
        "        closepath\n"
        "        fill\n"
        "        moveto\n"
        "        0 Symsize 2 div rmoveto\n"
        "        Symsize -2 div Symsize neg rlineto\n"
        "        Symsize 0 rlineto\n"
        "        closepath\n"
        "        fill\n"
        "    grestore\n"
        "    0 Symsize -2 div rmoveto\n"
        "    Symsize 2 div Symsize rlineto\n"
        "    Symsize neg 0 rlineto\n"
        "    Symsize 2 div Symsize neg rlineto\n"
        "    0 Symsize rmoveto\n"
        "    Symsize -2 div Symsize neg rlineto\n"
        "    Symsize 0 rlineto\n"
        "    Symsize -2 div Symsize rlineto\n"
        "    stroke\n",

        "    currentpoint\n"
        "    sym0\n"
        "    moveto\n"
        "    sym3\n",

        "    currentpoint\n"
        "    Symsize 2 div 0 rmoveto\n"
        "    2 copy Symsize 2 div 0 360 arc\n"
        "    closepath\n"
        "    gsave 1 setgray fill grestore\n"
        "    moveto\n"
        "    gsave\n"
        "        clip\n"
        "        sym4\n"
        "    grestore\n"
        "    stroke\n",

        "    currentpoint\n"
        "    sym0\n"
        "    moveto\n"
        "    2 setlinejoin\n"
        "    sym2\n"
        "    0 setlinejoin\n",

        "    Symsize -2 div dup rmoveto\n"
        "    Symsize 0 rlineto\n"
        "    0 Symsize rlineto\n"
        "    Symsize neg 0 rlineto\n"
        "    closepath\n"
        "    fill\n",

        "    currentpoint\n"
        "    Symsize 2 div 0 rlineto\n"
        "    Symsize 2 div 0 360 arc\n"
        "    closepath\n"
        "    fill\n",

        "    Symsize -2 div dup rmoveto\n"
        "    Symsize 0 rlineto\n"
        "    Symsize -2 div Symsize rlineto\n"
        "    closepath\n"
        "    fill\n",

        "    Symsize 2 div\n"
        "    dup neg 0 exch rmoveto\n"
        "    dup dup rlineto\n"
        "    dup dup neg exch rlineto\n"
        "    neg dup rlineto\n"
        "    closepath\n"
        "    fill\n"
    };

float
    colors [MAXGROUPS][3] = {
        { 0.0, 0.0, 1.0 },
        { 0.0, 1.0, 0.0 },
        { 0.0, 1.0, 1.0 },
        { 1.0, 0.0, 0.0 },
        { 1.0, 0.0, 1.0 },
        { 1.0, 1.0, 0.0 },
        { 0.0, 0.0, 0.5 },
        { 0.0, 0.5, 0.0 },
        { 0.0, 0.5, 0.5 },
        { 0.5, 0.0, 0.0 },
        { 0.5, 0.0, 0.5 },
        { 0.5, 0.5, 0.0 },
        { 0.3, 0.3, 0.7 },
        { 0.3, 0.7, 0.3 },
        { 0.3, 0.7, 0.7 },
        { 0.7, 0.3, 0.3 },
        { 0.7, 0.3, 0.7 },
        { 0.7, 0.7, 0.3 },
        { 0.3, 0.3, 0.3 } };

float
    **usercolors = NULL;


BOOL
    use_colours = TRUE,
    use_usercolours = FALSE,
    use_patterns = FALSE,
    use_borders = FALSE,
    use_symbols = FALSE,
    use_clnums = FALSE,
    use_rainbow = FALSE,
    use_bright,
    indexed = FALSE,
    preserve_borders,
    uselimit = FALSE,
    useeofill = FALSE,
    userenum = FALSE;


int
    *groups,
    *groupnums = NULL,
    currentgroup = -1,
    currentrenum = 1,
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
    max_colors = 0,
    n_colors = 0,
    high_names = 0,
    n_clusters = 0,
    max_clusters = 0,
    aantal,
    volgende = 1,
    top,
    inputline;

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
    islandr = 10,
    islandlw = 1,
    borderred = 0,
    bordergreen = 0,
    borderblue = 1,
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
    setclgroups (int cluster, int group),
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

int
    renumber (int cluster);

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
        k,
        n,
        v;
    double
        f,
        x,
        y,
        dx,
        dy,
        xmin,
        xmax,
        ymin,
        ymax;
    float
	r,
	g,
	b;
    FILE
        *fp;
    POINT
        *ptmp;
    BOOL
	int2float,
        err;

    no_mem_buffer = (char *) malloc (1024);

    get_programname (argv [0]);

    arg_c = argc;
    arg_v = argv;
    process_args ();

    if ((arg_c != 4 && ! indexed) || (arg_c != 3 && indexed))
        syntax ();

    if (use_usercolours) {
	int2float = FALSE;
	fp = r_fopen (colorfile);
	while (GetLine (FALSE, fp)) {
	    if (n_colors == max_colors) {
		max_colors += 16;
		usercolors = (float **) s_realloc (usercolors, max_colors * sizeof (float**));
	    }
	    if (sscanf (buffer, "%f %f %f", &r, &g, &b) != 3)
                errit ("Missing value(s) for in file \"%s\", line %i", colorfile, inputline);
	    if (r < 0 || r > 255)
                errit ("Red component out of range in file \"%s\", line %i", colorfile, inputline);
	    if (g < 0 || g > 255)
                errit ("Green component out of range in file \"%s\", line %i", colorfile, inputline);
	    if (b < 0 || b > 255)
                errit ("Blue component out of range in file \"%s\", line %i", colorfile, inputline);
	    if (r > 1 || g > 1 || b > 1)
		int2float = TRUE;
	    usercolors [n_colors] = s_malloc (3 * sizeof (float));
	    usercolors [n_colors][0] = r;
	    usercolors [n_colors][1] = g;
	    usercolors [n_colors][2] = b;
	    n_colors++;
	}
	fclose (fp);
	if (int2float)
	    for (i = 0; i < n_colors; i++)
		for (j = 0; j < 3; j++)
		    usercolors [i][j] /= 255;
    } else
	n_colors = MAXGROUPS;

    configfile = arg_v [1];
    if (indexed)
        indexfile = arg_v [2];
    else {
        dendrofile = arg_v [2];
        aantal = atoi (arg_v [3]);
        if (aantal < 1)
            errit ("Too few groups");
        if (aantal > n_colors && ((use_colours && ! use_rainbow) || use_patterns || use_symbols))
            errit ("Too many groups. You should use cluster borders, numbers, or rainbow colours.");
    }

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
            if (sscanf (buffer + 10, " %lf %lf %lf", &borderred, &bordergreen, &borderblue) != 3)
                errit ("Missing value(s) for 'borderrgb' in file \"%s\", line %i", configfile, inputline);
            if (borderred < 0.0 || borderred > 1.0)
                errit ("Red value out of range for 'borderrgb' in file \"%s\", line %i", configfile, inputline);
            if (bordergreen < 0.0 || bordergreen > 1.0)
                errit ("Green value out of range for 'borderrgb' in file \"%s\", line %i", configfile, inputline);
            if (borderblue < 0.0 || borderblue > 1.0)
                errit ("Blue value out of range for 'borderrgb' in file \"%s\", line %i", configfile, inputline);
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

    if (use_patterns && pslevel < 2) {
        warn ("Producing PostScript Level 2");
        pslevel = 2;
    }

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
        names [n_names].v = -2;
        names [n_names++].group = 1;
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

    if (indexed) {

        aantal = 0;

        fp = r_fopen (indexfile);
        while (GetLine (FALSE, fp)) {
            if (sscanf (buffer, "%i%n", &i, &n) < 1)
                errit ("Missing index number in file \"%s\", line %i: \"%s\"", indexfile, inputline, buffer);
            if (i < 1)
                errit ("Illegal index number in file \"%s\", line %i: \"%s\"", indexfile, inputline, buffer);
            if (i > aantal)
                aantal = i;
            while (buffer [n] && isspace ((unsigned char) buffer [n]))
                n++;
            unquote (buffer + n);
            for (j = 0; j < n_names; j++)
                if (! strcmp (buffer + n, names [j].s))
                    break;
            if (j == n_names)
                errit (
                    "Label \"%s\" in file \"%s\" not in file \"%s\"",
                    buffer + n,
                    indexfile,
                    labelfile
                );
            names [j].data = TRUE;
            names [j].group = i;
        }
        fclose (fp);

        if (aantal < 1)
            errit ("Too few groups");
        if (aantal > n_colors && ((use_colours && ! use_rainbow) || use_patterns || use_symbols))
            errit ("Too many groups. You should use cluster borders, numbers, or rainbow colours.");

    } else {

        fp = r_fopen (dendrofile);
        while (GetLine (FALSE, fp)) {
            if (n_clusters == max_clusters) {
                max_clusters += 256;
                cl = (CLUSTER *) s_realloc (cl, max_clusters * sizeof (CLUSTER));
            }
            if (sscanf (
                    buffer,
                    "%i %lf%n",
                    &(cl [n_clusters].index),
                    &(cl [n_clusters].value),
                    &i
                ) < 2
            )
                errit ("Syntax error in file \"%s\", line %i: \"%s\"", dendrofile, inputline, buffer);
            while (buffer [i] && isspace ((unsigned char) buffer [i]))
                i++;
            if (buffer [i] && buffer [i] != '#')
                cl [n_clusters].text = s_strdup (unquote (buffer + i));
            else
                cl [n_clusters].text = NULL;
            for (n = 0; n < 2; n++) {
                GetLine (TRUE, fp);
                switch (buffer [0]) {
                    case 'l':
                    case 'L':
                        cl [n_clusters].node [n] = LBL;
                        for (i = 1; buffer [i]; i++)
                            if (! isspace ((unsigned char) buffer [i]))
                                break;
                        unquote (buffer + i);
                        for (j = 0; j < n_names; j++)
                            if (! strcmp (buffer + i, names [j].s))
                                break;
                        if (j == n_names)
                            errit (
                                "Label %s in \"%s\" not in \"%s\"",
                                buffer + i,
                                dendrofile,
                                labelfile
                            );
                        cl [n_clusters].n [n].name = j;
                        names [j].data = TRUE;
                        break;
                    case 'c':
                    case 'C':
                        cl [n_clusters].node [n] = CLS;
                        if (sscanf (buffer + 1, "%i", &(cl [n_clusters].n [n].cluster)) != 1)
                            errit ("Missing cluster number at line %i", inputline);
                        break;
                    default:
                        errit ("Syntax error at line %i: \"%s\"", inputline, buffer);
                }
            }
            n_clusters++;
        }
        fclose (fp);


        /* new */

        /* replace indexes */
        for (i = 0; i < n_clusters; i++)
            for (j = 0; j < 2; j++)
                if (cl [i].node [j] == CLS)
                    for (k = 0; k < n_clusters; k++)
                        if (cl [i].n [j].cluster == cl [k].index) {
                            cl [i].n [j].cluster = k;
                            break;
                        }

        /* locate top node */
        for (i = 0; i < n_clusters; i++)
            cl [i].index = 1;
        for (i = 0; i < n_clusters; i++)
            for (j = 0; j < 2; j++)
                if (cl [i].node [j] == CLS)
                    cl [cl [i].n [j].cluster].index = 0;
        for (i = 0; i < n_clusters; i++)
            if (cl [i].index) {
                top = i;
                break;
            }

        /* divide into groups */
        j = 0;
        for (i = 0; i < n_clusters; i++) {
            cl [i].group [0] = cl [i].group [1] = 1;
            for (k = 0; k < 2; k++)
                if (cl [i].node [k] == LBL)
                    j++;
        }
        if (aantal > j)
            errit ("Too many groups");
        groups = (int *) s_malloc (aantal * sizeof (int));
        groups [0] = top;
        for (n = 1; n < aantal; n++) {
            f = - FLT_MAX;
            for (i = 0; i < n; i++)
                if (groups [i] < n_clusters && cl [groups [i]].value > f) {
                    j = i;
                    f = cl [groups [i]].value;
                }
            cl [groups [j]].group [0] = n + 1;
            cl [groups [j]].group [1] = j + 1;
            if (cl [groups [j]].node [0] == CLS)
                groups [n] = cl [groups [j]].n [0].cluster;
            else {
                names [cl [groups [j]].n [0].name].group = n + 1;
                groups [n] = INT_MAX;
            }
            if (cl [groups [j]].node [1] == CLS)
                groups [j] = cl [groups [j]].n [1].cluster;
            else {
                names [cl [groups [j]].n [1].name].group = j + 1;
                groups [j] = INT_MAX;
            }
            setclgroups (groups [n], n + 1);
        }

        /* end new */

	if (userenum) {
	    /* renumber */
	    groupnums = (int *) s_malloc ((aantal + 1) * sizeof (int));
	    currentgroup = -1;
	    currentrenum = 1;
	    renumber (top);
	    for (i = 0; i < n_names; i++) {
		names [i].group = groupnums [names [i].group];
	    }
	}
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

    if (use_colours || use_patterns || use_borders) {
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
    fprintf (fp_out, "%s, (c) P. Kleiweg 1997 - 2014\n", programname);
    fputs ("%%CreationDate: ", fp_out);
    time (&tp);
    fputs (asctime (localtime (&tp)), fp_out);
    fputs ("%%Title: ", fp_out);
    fprintf (fp_out, "%s %i\n", indexed ? indexfile : dendrofile, aantal);
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

    if (use_patterns) {
	fputs (
	    "<<\n"
	    "    /PatternType 1\n"
	    "    /PaintType 1\n"
	    "    /TilingType 1\n"
	    "    /PaintProc {\n"
	    "        begin\n"
	    "            XStep\n"
	    "            YStep\n"
	    "            1\n"
	    "            [ 1 0 0 1 0 0 ]\n"
	    "            data\n"
	    "            image\n"
	    "        end\n"
	    "    }\n"
	    ">>\n"
	    "/pdict exch def\n"
	    "\n"
	    "% stack in:  /label width height patterndata\n"
	    "% stack out: -\n"
	    "/defpattern {\n"
	    "    /pat exch def\n"
	    "    /y exch def\n"
	    "    /x exch def\n"
	    "    pdict /BBox [ 0 0 x y ] put\n"
	    "    pdict /XStep x put\n"
	    "    pdict /YStep y put\n"
	    "    pdict /data pat put\n"
	    "    pdict [ 72 60 div 0 0\n"
	    "            72 60 div 0 0 ] makepattern\n"
	    "    def\n"
	    "} bind def\n"
	    "\n",
	    fp_out
        );
	for (i = 0; i < aantal; i++)
	    fputs (pat [i], fp_out);
    }

    if (use_colours) {
	if (use_rainbow) {
	    fputs ("/SETCOLOR { sethsbcolor } bind def\n", fp_out);
	    for (i = 0; i < aantal; i++)
		fprintf (
		    fp_out,
		    "/c%i [ %.4f 1 %s ] def\n",
		    i,
		    ((float) i) / aantal,
		    ((i % 2) && ! use_bright) ? ".6" : "1"
                );

	} else {
	    fputs ("/SETCOLOR { setrgbcolor } bind def\n", fp_out);
	    for (i = 0; i < aantal; i++)
		fprintf (
		    fp_out,
		    "/c%i [ %.2f %.2f %.2f ] def\n",
		    i,
		    use_usercolours ? usercolors [i][0] : colors [i][0],
		    use_usercolours ? usercolors [i][1] : colors [i][1],
		    use_usercolours ? usercolors [i][2] : colors [i][2]
                );
	}
    }

    if (use_symbols) {
	fprintf (
	    fp_out,
	    "/Symsize %g def\n"
	    "/Symlw %g def\n"
	    "\n",
	    symbolsize,
	    symbollinewidth
        );
	for (i = 0; i < aantal; i++)
	    fprintf (fp_out, "/c%i /sym%i def\n", i, i);
    }

    if (use_borders)
	fprintf (
	    fp_out,
	    "/Borderwidth %g def\n"
	    "/Borderrgb [ %g %g %g ] def\n"
	    "\n",
	    borderwidth,
	    borderred,
	    bordergreen,
	    borderblue
        );

    fputs (
	"\n"
	"%%%% End of User Options %%%%\n"
	"\n",
	fp_out
    );
    if (use_symbols)
	for (i = 0; i < aantal; i++) {
	    fprintf (fp_out, "/sym%i {\n", i);
	    fputs (sym [i], fp_out);
	    fputs ("} bind def\n\n", fp_out);
	}

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
	"% [ X Y Map (name) dx dy p c ]\n"
	"%\n"
	"% X, Y: Map coordinates\n"
	"% Map: This translates map coordinates into screen coordinates\n"
	"% (name): Label\n"
	"% dx, dy: Position of label\n"
	"%     One should be either -1 (dx: left, dy: bottom) or 1 (dx: right, dy: top)\n"
	"%     The other should be between -1 and 1\n"
	"% p: What to do with it, the sum of these values:\n"
	"%     1: put dot\n"
	"%     2: put location number\n"
	"%     4: put label\n"
	"%     8: draw polygram\n"
	"%    16: put cluster symbol\n"
	"%    32: put cluster number\n"
        "%    64: draw data island\n"
        "%   128: draw data island as diamond (requires 64 as well)\n"
	"% c: Cluster index\n",
	fp_out
    );
    if (use_symbols)
	itemdefault |= 16;
    if (use_clnums)
	itemdefault |= 32;
    fprintf (
	fp_out,
	"/default %i def\n"
	"/PP [\n",
	itemdefault
    );
    for (i = 0; i < n_names; i++) {
	if (names [i].usage == USE || ((names [i].v > 0) && (names [i].v & 64))) {
	    if (names [i].v < -1)
		s = "default";
	    else
		s = tostr (names [i].v);
	    fprintf (
		fp_out,
		"  [ %g %g Map (%s) %g %g %s %s%i ]\n",
		names [i].x,
		names [i].y,
		psstring ((unsigned char *)names [i].s),
		names [i].dx,
		names [i].dy,
		s,
		((! use_patterns) && (use_clnums || (use_borders && ! use_symbols))) ? "" : "c",
		names [i].group - 1
            );
	} else
	    fputs ("  [ 0 0 () 0 0 0 0 ]\n", fp_out);
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
	"    /c exch def\n"
	"    /p exch def\n"
	"    /dy exch def\n"
	"    /dx exch def\n"
	"    /n exch def\n"
	"    /y exch def\n"
	"    /x exch def\n",
	fp_out
    );
    if (use_colours)
	fputs (
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
	    "        c aload pop SETCOLOR\n"
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
	    "    } if\n",
	    fp_out
	);
    else if (use_patterns)
	fputs (
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
	    "        /Pattern setcolorspace\n"
	    "        c setcolor\n"
	    "        Islandlw 0 gt {\n"
	    "            gsave\n"
	    "                fill\n"
	    "            grestore\n"
	    "            0 setgray\n"
	    "            Islandlw setlinewidth\n"
	    "            stroke\n"
	    "        } {\n"
	    "            fill\n"
	    "        } ifelse\n"
	    "    } if\n",
	    fp_out
	);
    else
	fputs (
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
	    "        1 setgray\n"
	    "        Islandlw 0 gt {\n"
	    "            gsave\n"
	    "                fill\n"
	    "            grestore\n"
	    "            0 setgray\n"
	    "            Islandlw setlinewidth\n"
	    "            stroke\n"
	    "        } {\n"
	    "            fill\n"
	    "        } ifelse\n"
	    "    } if\n",
	    fp_out
	);

    fputs (
	"} bind def\n"
	"\n",
        fp_out
    );

    fputs (
	"/dot2 {\n"
	"    dup /nmbr exch def\n"
	"    PP exch get aload pop\n"
	"    /c exch def\n"
	"    /p exch def\n"
	"    /dy exch def\n"
	"    /dx exch def\n"
	"    /n exch def\n"
	"    /y exch def\n"
	"    /x exch def\n",
	fp_out
    );
    if (! use_symbols)
	fputs (
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
	    "\n",
	    fp_out
        );
    if (use_colours)
	fputs (
	    "    p 2 and 0 ne {\n"
	    "        x y moveto\n"
	    "        nmbr 1 add Str cvs\n"
	    "        dup stringwidth pop 2 div neg CHeight 2 div neg rmoveto\n"
	    "        c aload pop SETCOLOR\n"
	    "        currentgray Graylimit gt { 0 } { 1 } ifelse setgray\n"
	    "        show\n"
	    "    } if\n"
	    "\n",
	    fp_out
        );
    else
	fputs (
	    "    p 2 and 0 ne {\n"
	    "        x y moveto\n"
	    "        nmbr 1 add Str cvs\n"
	    "        dup stringwidth pop dup 2 div neg CHeight 2 div neg rmoveto\n"
	    "        gsave\n"
	    "            1 setgray\n"
	    "            currentpoint\n"
	    "            1 sub exch 1 sub exch\n"
	    "            3 -1 roll 2 add CHeight 2 add\n"
	    "            rectfill\n"
	    "        grestore\n"
	    "        show\n"
	    "    } if\n"
	    "\n",
	    fp_out
        );

    fputs (
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
	"\n",
	fp_out
    );

    if (use_symbols)
	fputs (
	    "    p 16 and 0 ne {\n"
	    "        Symlw setlinewidth\n"
	    "        x y moveto\n"
	    "        c cvx exec\n"
	    "    } if\n"
	    "\n",
	    fp_out
        );

    if (use_clnums) {
	fputs (
	    "    p 32 and 0 ne {\n"
	    "        x y moveto\n"
	    "        c 1 add Str cvs\n"
	    "        dup stringwidth pop dup 2 div neg CHeight 2 div neg rmoveto\n"
	    "        gsave\n"
	    "            1 setgray\n"
	    "            currentpoint\n"
	    "            1 sub exch 1 sub exch\n"
	    "            3 -1 roll 2 add CHeight 2 add\n"
	    "            rectfill\n"
	    "        grestore\n"
	    "        show\n"
	    "    } if\n"
	    "\n",
	    fp_out
        );
    }

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

    if (use_patterns) {
	if (clipfile && ! uselimit)
	    fputs (
		"/poly {\n"
		"    newpath\n"
		"    Map moveto\n"
		"    counttomark 2 idiv { Map lineto } repeat\n"
		"    closepath\n"
		"    pop\n"
		"    PP exch get\n"
		"    /Pattern setcolorspace\n"
		"    aload pop setcolor\n"
		"    8 and 0 eq {\n"
		"        fill\n"
		"    } {\n"
		"        gsave fill grestore\n"
		"        gsave Linewidth 3 mul setlinewidth 1 setgray stroke grestore\n"
		"        0 setgray\n"
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
		"        /Pattern setcolorspace\n"
		"        aload pop setcolor\n"
		"        8 and 0 eq {\n"
		"            fill\n"
		"        } {\n"
		"            gsave fill grestore\n"
		"            gsave Linewidth 3 mul setlinewidth 1 setgray stroke grestore\n"
		"            0 setgray\n"
		"            stroke\n"
		"        } ifelse\n"
		"        5 { pop } repeat\n"
		"    grestore\n"
		"} bind def\n",
		fp_out
	    );
    } else if (use_colours) {
	if (clipfile && ! uselimit)
	    fputs (
		"/poly {\n"
		"    newpath\n"
		"    Map moveto\n"
		"    counttomark 2 idiv { Map lineto } repeat\n"
		"    closepath\n"
		"    pop\n"
		"    PP exch get\n"
		"    aload pop aload pop SETCOLOR\n"
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
		"        aload pop aload pop SETCOLOR\n"
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
    }

    if (use_colours || use_patterns) {

	fputs ("\nLinewidth setlinewidth\n\n", fp_out);

	for (i = 0; i < n_names; i++)
	    if (names [i].usage == USE) {
		fprintf (fp_out, "%i mark\n", i);
		for (ptmp = point [i]; ptmp != NULL; ptmp = ptmp->next)
		    fprintf (fp_out, "%g %g\n", ptmp->x, ptmp->y);
		fputs ("poly\n", fp_out);
	    }
	fputs ("\n", fp_out);

    }

    if (use_borders) {
	fputs (
	    "\n"
	    "Borderwidth setlinewidth\n"
	    "Borderrgb aload pop setrgbcolor\n"
	    "1 setlinecap\n"
	    "/B { Map moveto Map lineto stroke } bind def\n",
	    fp_out
        );
	if (uselimit || ! clipfile)
	    fputs (
		"/C {\n"
		"    gsave\n"
		"    Map\n"
		"    2 copy\n"
		"    exch Limit add exch\n"
		"    moveto\n"
		"    Limit 0 360 arc\n"
		"    closepath\n"
		"    clip\n"
		"    newpath\n"
		"} bind def\n",
		fp_out
	    );


    }

    if (use_borders) {

        for (i = 0; i < n_names; i++) {
            j = (uselimit || ! clipfile) ? 0 : 2;
            if (names [i].usage == USE)
                for (ptmp = point [i]; ptmp != NULL; ptmp = ptmp->next)
                    if ((preserve_borders && ptmp->i > -1 && names [ptmp->i].usage == PRESERVE) ||
                        (ptmp->i > i && names [ptmp->i].usage == USE && names [i].group != names [ptmp->i].group))
		    {
                        if (j == 0) {
                            fprintf (
				fp_out,
                                "%g %g C\n",
                                names [i].x,
                                names [i].y
			    );
                            j = 1;
                        }
			fprintf (fp_out, "%g %g\n", ptmp->x, ptmp->y);
			if (ptmp->next)
			    fprintf (fp_out, "%g %g", ptmp->next->x, ptmp->next->y);
			else
			    fprintf (fp_out, "%g %g", (point [i])->x, (point [i])->y);
			fputs (" B\n", fp_out);
                    }
            if (j == 1)
                fputs ("grestore\n", fp_out);
        }

        fputs ("\n1 setlinewidth\n0 setlinecap\n0 setgray\n\n", fp_out);
    }

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

int renumber (int cluster) {
    int
	i,
	n;

    for (i = 0; i < 2; i++) {
	if (cl [cluster].node [i] == LBL) {
	    if (cl [cluster].group [i] != currentgroup) {
		currentgroup = cl [cluster].group [i];
		groupnums [currentgroup] = currentrenum++;
	    }
	    n = currentgroup;
	} else {
	    n = renumber (cl [cluster].n [i].cluster);
	}
    }
    return n;
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

void setclgroups (int cluster, int group)
{
    int
        i;

    if (cluster < n_clusters)
        for (i = 0; i < 2; i++) {
            cl [cluster].group [i] = group;
            if (cl [cluster].node [i] == CLS)
                setclgroups (cl [cluster].n [i].cluster, group);
            else
                names [cl [cluster].n [i].name].group = group;
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
            case 'b':
                use_borders = TRUE;
                use_colours = FALSE;
                preserve_borders = FALSE;
                break;
            case 'B':
                use_borders = TRUE;
                use_colours = FALSE;
                preserve_borders = TRUE;
                break;
            case 'i':
                indexed = TRUE;
                break;
            case 'I':
                use_clnums = TRUE;
                use_patterns = use_colours = use_symbols = FALSE;
                break;
            case 'o':
                outfile = get_arg ();
                break;
	    case 'O':
		userenum = TRUE;
		break;
            case 'p':
                use_patterns = TRUE;
                use_colours = use_symbols = use_clnums = FALSE;
                break;
            case 'r':
                use_rainbow = TRUE;
                use_bright = use_patterns = use_borders = use_symbols = use_clnums = FALSE;
                break;
            case 'R':
                use_bright = use_rainbow = TRUE;
                use_patterns = use_borders = use_symbols = use_clnums = FALSE;
                break;
            case 's':
                use_symbols = TRUE;
                use_colours = use_patterns = use_clnums = FALSE;
                break;
            case 'u':
                colorfile = get_arg ();
		use_usercolours = TRUE;
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
        "Cluster Map Drawing, Version " my_VERSION "\n"
        "(c) P. Kleiweg 1997 - 2014\n"
        "\n"
        "Usage: %s [-o filename] [-u filename] [-b] [-B] [-I] [-O] [-p] [-r] [-R] [-s]\n\tconfigfile clusterfile number\n"
        "\n"
        "Usage: %s -i [-o filename] [-u filename] [-b] [-B] [-I] [-p] [-r] [-R] [-s]\n\tconfigfile indexfile\n"
        "\n"
        "\t-i : use indexed data instead of clustering data\n"
        "\t-o : output file\n"
        "\t-b : don't use colours, use cluster borders (without preserved areas)\n"
        "\t-B : don't use colours, use cluster borders (with preserved areas)\n"
        "\t-I : don't use colours, use cluster numbers\n"
	"\t-O : reorder colours\n"
        "\t-p : don't use colours, use black/white patterns (PS level 2)\n"
        "\t-r : use rainbow colours, light and dark, instead of standard colours\n"
        "\t-R : use rainbow colours, light, instead of standard colours\n"
        "\t-s : don't use colours, use symbols\n"
	"\t-u : user-defined colours\n"
        "\n"
        "Options -b or -B can be combined with -I or -p or -s\n"
        "\n",
        programname,
        programname
    );
    exit (1);
}
