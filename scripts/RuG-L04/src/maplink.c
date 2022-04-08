/*
 * File: maplink.c
 *
 * (c) Peter Kleiweg
 *     2008, 2010
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2,
 * or (at your option) any later version.
 *
 */

#define my_VERSION "0.13"

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
    float
        x,
        y,
        dx,
        dy;
    int
        v;
    BOOL
        coo;
} NAME_;

typedef struct {
    int
        i,
	j;
    double
        dif,
	geo;
} LINK_;

NAME_
    *names = NULL;

LINK_
    *links = NULL;

#define BUFSIZE 2048

BOOL
    commentsave = FALSE,
    greyscale = FALSE,
    reverse = FALSE,
    hasNegative = FALSE,
    negEqual = FALSE;

char
    **comments = NULL,
    **arg_v,
    *diffile,
    *configfile,
    *outfile = NULL,
    *transformfile = NULL,
    *coordfile = NULL,
    *mapfile = NULL,
    *usefile = NULL,
    *fontname = "Helvetica",
    *fontmatrix = "[ 8 0 0 8 0 0 ]",
    buffer [BUFSIZE + 1],
    buf    [BUFSIZE + 1],
    *programname,
    *no_mem_buffer,
    out_of_memory [] = "Out of memory!";

int
    difsize,
    n_links,
    n_comments = 0,
    max_comments = 0,
    pslevel = 1,
    llx = 0,
    lly = 0,
    urx = 595,
    ury = 842,
    arg_c,
    itemdefault = 0,
    inputline;

float
    expF  = 1.0,
    baseF = 0.0,
    maxDif = .9,
    maxGeo = .3;

double
    radius = 2.5,
    white = 1,
    xgap = 4.0,
    ygap = 4.0,
    borderwidth = 2,
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
    *s_strdup (char const *s),
    *tostr (int i);

char const
    *unquote (char *s),
    *psstring (unsigned char const *s);

int
    difcmp (void *, void *),
    difabscmp (void *, void *),
    geocmp (void *, void *);

BOOL
    has_argument (void),
    GetLine (BOOL required, FILE *fp);

FILE
    *r_fopen (char const *filename);

int main (int argc, char *argv [])
{
    char
        *s = NULL;
    int
        i,
        j,
        n,
	v;
    double
        x,
        y,
	d1,
	d2,
	dd,
        dx,
        dy,
	f;
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
            ;
        } else if (! memcmp (buffer, "limit:", 6)) {
	    ;
        } else if (! memcmp (buffer, "uselimit:", 9))
	    ;
	else if (! memcmp (buffer, "boundingbox:", 12)) {
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
	    ;
        } else if (! memcmp (buffer, "fillmargin:", 11)) {
	    ;
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
	    ;
	} else if (! memcmp (buffer, "borderwidth:", 12)) {
	    borderwidth = atof (buffer + 12);
            if (borderwidth <= 0.0)
                errit ("Illegal value for 'borderwidth' in file \"%s\", line %i", configfile, inputline);
        } else if (! memcmp (buffer, "backrgb:", 8)) {
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
	    ;
        } else if (! memcmp (buffer, "map:", 4)) {
            mapfile = getstring (buffer + 4);
        } else if (! memcmp (buffer, "labels:", 7)) {
	    ;
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

    if (coordfile == NULL)
        errit ("Missing 'coordinates' in file \"%s\"\n", configfile);

    if (transformfile == NULL)
        errit ("Missing 'transform' in file \"%s\"\n", configfile);

    fp = r_fopen (diffile);
    commentsave = TRUE;
    GetLine (TRUE, fp);
    difsize = atoi (buffer);
    if (difsize < 2)
        errit ("Illegal table size in file \"%s\", line %lu\n", diffile, inputline);
    names = (NAME_ *) s_malloc (difsize * sizeof (NAME_));
    for (i = 0; i < difsize; i++) {
	GetLine (TRUE, fp);
	names [i].s = s_strdup (buffer);
	names [i].coo = FALSE;
	names [i].v = -2;
    }
    n_links = 0;
    links = (LINK_*) s_malloc ((difsize * difsize - difsize) / 2 * sizeof(LINK_));
    for (i = 1; i < difsize; i++) {
	for (j = 0; j < i; j++) {
	    GetLine (TRUE, fp);
	    if (! strcmp (buffer, "NA"))
		continue;
	    if (sscanf (buffer, "%lf", &f) < 1)
		errit ("Missing vale in file \"%s\", line %i", diffile, inputline);
	    links [n_links].i = i;
	    links [n_links].j = j;
	    links [n_links].dif = f;
	    n_links++;
	    if (f < 0) {
		hasNegative = TRUE;
		greyscale = FALSE;
	    }
	}
    }
    commentsave = FALSE;
    fclose (fp);

    if (reverse) {
	if (hasNegative) {
	    for (i = 0; i < n_links; i++)
		links [i].dif = -links [i].dif;
	} else {
	    d1 = d2 = links [0].dif;
	    for (i = 1; i < n_links; i++) {
		if (links [i].dif < d1)
		    d1 = links [i].dif;
		if (links [i].dif > d2)
		    d2 = links [i].dif;
	    }
	    for (i = 0; i < n_links; i++)
		links [i].dif = d1 + d2 - links [i].dif;
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
        for (i = 0; i < difsize; i++)
            if (! strcmp (names [i].s, buffer + n))
                break;
        if (i == difsize)
	    continue;
	names [i].x = x;
	names [i].y = y;
	names [i].dx = dx;
	names [i].dy = dy;
	names [i].coo = TRUE;
    }
    fclose (fp);

    for (i = 0; i < difsize; i++)
	if (! names [i].coo)
	    errit ("Missing coordinates for \"%s\" in file \"%s\"", names [i].s, coordfile);

    if (usefile) {
        fp = r_fopen (usefile);
        while (GetLine (FALSE, fp)) {
            if (sscanf (buffer, "%i%n", &v, &n) < 1)
                errit ("Missing values in file \"%s\", line %i", usefile, inputline);
            if (v < -1 || v > 255)
                errit ("Illegal value in file \"%s\", line %i", usefile, inputline);
	    if (v < 0)
		v = 0;
            while (buffer [n] && isspace ((unsigned char) buffer [n]))
                n++;
            if (! buffer [n])
                errit ("Missing name in file \"%s\", line %i", usefile, inputline);
            unquote (buffer + n);
            for (i = 0; i < difsize; i++)
                if (! strcmp (names [i].s, buffer + n))
                    break;
            if (i < difsize)
		names [i].v = v;
        }
        fclose (fp);
    }

    if (maxGeo < 1.0) {
	for (i = 0; i < n_links; i++) {
	    dx = (names [links [i].i].x - names [links [i].j].x) / aspect;
	    dy = names [links [i].i].y - names [links [i].j].y;
	    links [i].geo = sqrt (dx * dx + dy * dy);
	}
	Heapsort (links, n_links, sizeof (LINK_), geocmp);
	f = maxGeo * links [n_links - 1].geo;
	while (n_links > 0 && links [n_links - 1].geo > f)
	    n_links--;
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
    fprintf (fp_out, "%s, (c) P. Kleiweg 2008, 2010\n", programname);
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
        "/Fontname /%s def\n"
        "/Fontmtrx %s def\n"
        "\n"
        "/Radius %g def  %c Radius of dot\n"
        "/White %g def   %c Linewidth around dot\n"
        "/XGap %g def    %c Horizontal distance between label and center of dot\n"
        "/YGap %g def    %c Vertical   distance between label and center of dot\n"
        "\n"
        "\n",
	expF,
	baseF,
        fontname,
        fontmatrix,
        radius, '%',
        white, '%',
        xgap, '%',
        ygap, '%'
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

    if (greyscale)
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
    else if (hasNegative)
	fputs (
	    "/RdBu56a [\n"
            "  [ 249 246 245 ]\n"
            "  [ 251 244 239 ]\n"
            "  [ 253 240 231 ]\n"
            "  [ 253 234 222 ]\n"
            "  [ 253 227 211 ]\n"
            "  [ 253 219 199 ]\n"
            "  [ 253 210 187 ]\n"
            "  [ 252 201 174 ]\n"
            "  [ 250 192 161 ]\n"
            "  [ 249 181 148 ]\n"
            "  [ 246 171 136 ]\n"
            "  [ 242 159 124 ]\n"
            "  [ 237 148 114 ]\n"
            "  [ 232 136 103 ]\n"
            "  [ 226 123  94 ]\n"
            "  [ 220 110  85 ]\n"
            "  [ 214  96  77 ]\n"
            "  [ 209  82  69 ]\n"
            "  [ 203  67  62 ]\n"
            "  [ 197  53  56 ]\n"
            "  [ 191  40  50 ]\n"
            "  [ 183  29  45 ]\n"
            "  [ 173  20  41 ]\n"
            "  [ 161  13  38 ]\n"
            "  [ 148   8  36 ]\n"
            "  [ 134   5  34 ]\n"
            "  [ 118   2  32 ]\n"
            "  [ 103   0  31 ]\n"
	    "] def\n"
	    "\n"
	    "/RdBu56b [\n"
            "  [ 245 247 248 ]\n"
            "  [ 240 246 249 ]\n"
            "  [ 233 243 248 ]\n"
            "  [ 226 239 246 ]\n"
            "  [ 218 234 243 ]\n"
            "  [ 209 229 240 ]\n"
            "  [ 199 224 237 ]\n"
            "  [ 189 219 234 ]\n"
            "  [ 178 213 231 ]\n"
            "  [ 166 207 228 ]\n"
            "  [ 153 201 224 ]\n"
            "  [ 139 193 220 ]\n"
            "  [ 123 184 215 ]\n"
            "  [ 108 175 209 ]\n"
            "  [  93 166 204 ]\n"
            "  [  79 156 199 ]\n"
            "  [  67 147 195 ]\n"
            "  [  58 138 192 ]\n"
            "  [  50 130 189 ]\n"
            "  [  44 122 186 ]\n"
            "  [  40 114 182 ]\n"
            "  [  35 106 176 ]\n"
            "  [  31  98 167 ]\n"
            "  [  26  88 156 ]\n"
            "  [  21  79 143 ]\n"
            "  [  16  69 129 ]\n"
            "  [  10  58 113 ]\n"
            "  [   5  48  97 ]\n"
	    "] def\n"
	    "\n"
	    "/setmycolorplus {\n"
	    "    28 mul cvi\n"
	    "    dup  0 lt { pop  0 } if\n"
	    "    dup 27 gt { pop 27 } if\n"
	    "    RdBu56a exch get\n"
	    "    aload pop\n"
	    "    3 { 255 div 3 1 roll } repeat\n"
	    "    setrgbcolor\n"
	    "} bind def\n"
	    "\n"
	    "/setmycolorneg {\n"
	    "    28 mul cvi\n"
	    "    dup  0 lt { pop  0 } if\n"
	    "    dup 27 gt { pop 27 } if\n"
	    "    RdBu56b exch get\n"
	    "    aload pop\n"
	    "    3 { 255 div 3 1 roll } repeat\n"
	    "    setrgbcolor\n"
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
        "%     4: put label\n",
        fp_out
    );
    fprintf (
        fp_out,
        "/default %i def\n"
        "/PP [\n",
        itemdefault
    );
    for (i = 0; i < difsize; i++) {
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
        "        gsave\n"
        "            1 setgray\n"
        "            currentpoint Lower sub\n"
        "            2 index stringwidth pop Height 1 add\n"
        "            rectfill\n"
        "        grestore\n"
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
	"\n"
        "} bind def\n"
        "\n"
	"Borderwidth setlinewidth\n"
	"1 setlinecap\n"
	"\n",
	fp_out
    );

    if (hasNegative) {

	fputs (
	    "/B {\n"
	    "    EXP exp\n"
	    "    1 BASE sub mul BASE add\n"
	    "    setmycolorplus\n"
	    "} bind def\n"
	    "\n"
	    "/Bn {\n"
	    "    neg\n"
	    "    EXP exp\n"
	    "    1 BASE sub mul BASE add\n"
	    "    setmycolorneg\n"
	    "} bind def\n"
	    "\n"
	    "/l {\n"
	    "    dup 0 lt {\n"
	    "        Bn\n"
	    "    } {\n"
	    "        B\n"
	    "    } ifelse\n"
	    "    PP exch get\n"
	    "    aload pop\n"
	    "    pop pop pop pop\n"
	    "    moveto\n"
	    "    PP exch get\n"
	    "    aload pop\n"
	    "    pop pop pop pop\n"
	    "    lineto\n"
	    "    stroke\n"
	    "} bind def\n"
	    "\n",
	    fp_out
	);

	d1 = d2 = 0.0;
	for (i = 0; i < n_links; i++) {
	    if (links [i].dif > d2)
		d2 = links [i].dif;
	    if (links [i].dif < d1)
		d1 = links [i].dif;
	}
	d1 = fabs (d1);
	if (negEqual) {
	    if (d2 > d1)
		d1 = d2;
	    else
		d2 = d1;
	}

	for (i = 0; i < n_links; i++) {
	    if (links [i].dif < 0.0)
		links [i].dif /= d1;
	    else
		links [i].dif /= d2;
	}
	Heapsort (links, n_links, sizeof (LINK_), difabscmp);
	for (i = 0; i < n_links; i++) {
	    f = links [i].dif;
	    if (fabs (f) >= 1.0 - maxDif)
		fprintf (fp_out, "%i %i %g l\n", links [i].i, links [i].j, f);
	}

    } else {

	fputs (
	    "/B {\n"
	    "    1 exch sub\n"
	    "    EXP exp\n"
	    "    1 BASE sub mul BASE add\n"
	    "    setmycolor\n"
	    "} bind def\n"
	    "\n"
	    "/l {\n"
	    "    B\n"
	    "    PP exch get\n"
	    "    aload pop\n"
	    "    pop pop pop pop\n"
	    "    moveto\n"
	    "    PP exch get\n"
	    "    aload pop\n"
	    "    pop pop pop pop\n"
	    "    lineto\n"
	    "    stroke\n"
	    "} bind def\n"
	    "\n",
	    fp_out
	);

	Heapsort (links, n_links, sizeof (LINK_), difcmp);
	d1 = links [0].dif;
	d2 = links [n_links - 1].dif;
	dd = d2 - d1;
	for (i = n_links - 1; i >= 0; i--) {
	    f = (links [i].dif - d1) / dd;
	    if (f <= maxDif)
		fprintf (fp_out, "%i %i %g l\n", links [i].i, links [i].j, f);
	}
    }
    fputs (
        "\n"
	"1 setlinewidth\n"
	"0 setlinecap\n"
	"0 setgray\n"
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
            case 'e':
                negEqual = TRUE;
                break;
            case 'g':
                greyscale = TRUE;
                break;
            case 'o':
                outfile = get_arg ();
                break;
            case 'r':
                reverse = TRUE;
                break;
	    case 'D':
		maxDif = atof (get_arg ());
		if (maxDif < 0.0 || maxDif > 1.0)
		    errit ("Value for -D must be between 0 and 1");
		break;
	    case 'G':
		maxGeo = atof (get_arg ());
		if (maxGeo < 0.0 || maxGeo > 1.0)
		    errit ("Value for -G must be between 0 and 1");
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


int difcmp (void *p1, void *p2)
{
    double
	f;
    f = ((LINK_ *)p1)->dif - ((LINK_ *)p2)->dif;
    if (f < 0.0)
	return -1;
    if (f > 0.0)
	return 1;
    return 0;
}

int difabscmp (void *p1, void *p2)
{
    double
	f;
    f = fabs( ((LINK_ *)p1)->dif) - fabs (((LINK_ *)p2)->dif);
    if (f < 0.0)
	return -1;
    if (f > 0.0)
	return 1;
    return 0;
}

int geocmp (void *p1, void *p2)
{
    double
	f;
    f = ((LINK_ *)p1)->geo - ((LINK_ *)p2)->geo;
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
        "Link Map Drawing, Version " my_VERSION "\n"
        "(c) P. Kleiweg 2008, 2010\n"
        "\n"
        "Usage: %s [-c float] [-C float] [-D] [-G] [-e] [-g] [-o filename]\n\tconfigfile differencefile\n"
	"\n"
	"\t-c : contrast parameter 1 (now: %g)\n"
	"\t-C : contrast parameter 2 (now: %g)\n"
	"\t-D : maximum difference (fraction, now: %g)\n"
	"\t-G : maximum geographic distance (fraction, now: %g)\n"
	"\t-e : equal scaling of positive and negative values\n"
	"\t-g : use greyscale\n"
        "\t-o : output file\n"
	"\t-r : reverse values\n"
        "\n",
        programname,
	expF,
	baseF,
	maxDif,
	maxGeo
    );
    exit (1);
}
