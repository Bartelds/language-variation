/*
 * File: legend.c
 *
 * (c) Peter Kleiweg 2002, 2004, 2005, 2007, 2008
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2,
 * or (at your option) any later version.
 *
 */

#define my_VERSION "0.07"

#define __NO_MATH_INLINES

#define MAXITEMS 19

#define defFONTSIZE 9

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
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef enum { FALSE = 0, TRUE } BOOL_;

typedef enum { legCOLORS, legPATTERNS, legSYMBOLS } legTYPE_;

typedef struct {
    int
        i;
    unsigned char
        *s;
} LINE_;

#define BUFSIZE 1047

LINE_
    *lines = NULL;

legTYPE_
    legtype = legCOLORS;

BOOL_
    eXtended = FALSE,
    use_rainbow = FALSE,
    use_bright,
    use_usercolours = FALSE;

int
    pslevel = 1,
    llx = 104,
    lly = 0,
    urx = 595,
    ury = 706,
    nlines = 0,
    maxlines = 0,
    max_colors = 0,
    n_colors = MAXITEMS,
    arg_c;
long int
    lineno = 0;

int
    helvetica [] = {
           0,  278,  278,  278,  278,  278,  278,  278,
         278,  278,  278,  278,  278,  278,  278,  278,
         278,  278,  278,  278,  278,  278,  278,  278,
         278,  278,  278,  278,  278,  278,  278,  278,
         278,  278,  355,  556,  556,  889,  667,  222,
         333,  333,  389,  584,  278,  584,  278,  278,
         556,  556,  556,  556,  556,  556,  556,  556,
         556,  556,  278,  278,  584,  584,  584,  556,
        1015,  667,  667,  722,  722,  667,  611,  778,
         722,  278,  500,  667,  556,  833,  722,  778,
         667,  778,  722,  667,  611,  722,  667,  944,
         667,  667,  611,  278,  278,  278,  469,  556,
         222,  556,  556,  500,  556,  556,  278,  556,
         556,  222,  222,  500,  222,  833,  556,  556,
         556,  556,  333,  500,  278,  556,  500,  722,
         500,  500,  500,  334,  260,  334,  584,  278,
         278,  278,  278,  278,  278,  278,  278,  278,
         278,  278,  278,  278,  278,  278,  278,  278,
         278,  333,  333,  333,  333,  333,  333,  333,
         333,  278,  333,  333,  278,  333,  333,  333,
         278,  333,  556,  556,  556,  556,  260,  556,
         333,  737,  370,  556,  584,  333,  737,  333,
         400,  584,  333,  333,  333,  556,  537,  278,
         333,  333,  365,  556,  834,  834,  834,  611,
         667,  667,  667,  667,  667,  667, 1000,  722,
         667,  667,  667,  667,  278,  278,  278,  278,
         722,  722,  778,  778,  778,  778,  778,  584,
         778,  722,  722,  722,  722,  667,  667,  611,
         556,  556,  556,  556,  556,  556,  889,  500,
         556,  556,  556,  556,  278,  278,  278,  278,
         556,  556,  556,  556,  556,  556,  556,  584,
         611,  556,  556,  556,  556,  500,  556,  500   };

char
    buffer [BUFSIZE + 1],
    *infile,
    *outfile = NULL,
    *colorfile = NULL,
    **arg_v,
    *programname,
    *no_mem_buffer,
    out_of_memory [] = "Out of memory",
    *pat [] = {
        "/c1  4 4 {< 7f7fdfdf         >} defpattern\n",
        "/c2  2 2 {< 7fbf             >} defpattern\n",
        "/c3  6 6 {< bfa3bff717f7     >} defpattern\n",
        "/c4  4 4 {< 7fffdfff         >} defpattern\n",
        "/c5  4 4 {< 1fff4fff         >} defpattern\n",
        "/c6  3 4 {< 7f7fbfbf         >} defpattern\n",
        "/c7  3 3 {< 7fbfdf           >} defpattern\n",
        "/c8  4 4 {< 9fff0fff         >} defpattern\n",
        "/c9  8 6 {< 017d55d710ff     >} defpattern\n",
        "/c10 4 6 {< bf5fbfef57ef     >} defpattern\n",
        "/c11 6 4 {< ff17ffa3         >} defpattern\n",
        "/c12 2 4 {< 5fffbfff         >} defpattern\n",
        "/c13 8 8 {< 515f51ff15f515ff >} defpattern\n",
        "/c14 3 4 {< 7f7f7f9f         >} defpattern\n",
        "/c15 6 6 {< 27bf93fb4bef     >} defpattern\n",
        "/c16 4 8 {< bf5f5fbfef5f5fef >} defpattern\n",
        "/c17 5 4 {< 9f6f6fff         >} defpattern\n",
        "/c18 6 6 {< af27ff27afff     >} defpattern\n",
        "/c19 8 8 {< df99fdf7dfccfd7f >} defpattern\n"
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
    colors [MAXITEMS][3] = {
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

FILE
    *fp_in,
    *fp_out;

void
    get_programname (char const *argv0),
    process_args (void),
    errit (char const *format, ...),
    syntax (void),
    *s_malloc (size_t size),
    *s_realloc (void *block, size_t size);
char
    *get_arg (void),
    *s_strdup (char const *s);
char const
    *toPS (unsigned char const *s);
float
    psstringwidth (unsigned char const *s);
BOOL_
    GetLine (void);

int main (int argc, char *argv [])
{
    int
	i,
	j,
	n,
	max;
    float
	f,
	fmax,
	r,
	g,
	b;
    time_t
	tp;
    BOOL_
	int2float;
	
    no_mem_buffer = (char *) malloc (1024);

    get_programname (argv [0]);

    arg_c = argc;
    arg_v = argv;
    process_args ();

    if (legtype == legPATTERNS)
	pslevel = 2;

    if (arg_c != 2)
	syntax ();


    if (use_usercolours) {
       int2float = FALSE;
       n_colors = 0;
       lineno = 0;
       fp_in = fopen (colorfile, "r");
       if (! fp_in)
	   errit ("Opening file \"%s\": %s", colorfile, strerror (errno));
       while (GetLine ()) {
           if (n_colors == max_colors) {
               max_colors += 16;
               usercolors = (float **) s_realloc (usercolors, max_colors * sizeof (float**));
           }
           if (sscanf (buffer, "%f %f %f", &r, &g, &b) != 3)
                errit ("Missing value(s) for in file \"%s\", line %i", colorfile, lineno);
           if (r < 0 || r > 255)
                errit ("Red component out of range in file \"%s\", line %i", colorfile, lineno);
           if (g < 0 || g > 255)
                errit ("Green component out of range in file \"%s\", line %i", colorfile, lineno);
           if (b < 0 || b > 255)
                errit ("Blue component out of range in file \"%s\", line %i", colorfile, lineno);
           if (r > 1 || g > 1 || b > 1)
               int2float = TRUE;
           usercolors [n_colors] = s_malloc (3 * sizeof (float));
           usercolors [n_colors][0] = r;
           usercolors [n_colors][1] = g;
           usercolors [n_colors][2] = b;
           n_colors++;
       }
       fclose (fp_in);
       if (int2float)
           for (i = 0; i < n_colors; i++)
               for (j = 0; j < 3; j++)
                   usercolors [i][j] /= 255;
       lineno = 0;
    }

    infile = arg_v [1];

    fp_in = fopen (infile, "r");
    if (! fp_in)
	errit ("Opening file \"%s\": %s", infile, strerror (errno));
    while (GetLine ()) {
	if (sscanf (buffer, "%i%n", &i, &n) < 1)
	    errit ("Missing value in file \"%s\", line %li", infile, lineno);
	if (i < 0 || (i > n_colors && ! use_rainbow))
	    errit ("Value out of range in file \"%s\", line %li", infile, lineno);
	while (buffer [n] && isspace ((unsigned char) buffer [n]))
	    n++;
	if (nlines == maxlines) {
	    maxlines += 32;
	    lines = (LINE_ *) s_realloc (lines, maxlines * sizeof (LINE_));
	}
	lines [nlines].i = i;
	lines [nlines++].s = (unsigned char *) s_strdup (buffer + n);
    }
    fclose (fp_in);

    if (outfile) {
	fp_out = fopen (outfile, "w");
	if (! fp_out)
	    errit ("Creating file \"%s\": %s", outfile, strerror (errno));
    } else
	fp_out = stdout;

    lly = 707 - (legtype == legPATTERNS ? 16 : 12) * nlines;

    fmax = 0;
    for (i = 0; i < nlines; i++)
	if (lines [i].i > 0) {
	    f = psstringwidth (lines [i].s);
	    if (f > fmax)
		fmax = f;
	}
    urx = 121 + fmax;

    if (legtype == legPATTERNS) {
	llx -= 4;
	ury += 4;
    }

    for (i = 0; i < nlines && ! eXtended; i++)
	for (j = 0; lines [i].s [j]; j++)
	    if ((int) lines [i].s [j] < 32 || (int) lines [i].s [j] > 127) {
		eXtended = TRUE;
		break;
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
    fprintf (fp_out, "%s, (c) P. Kleiweg 2002, 2004, 2005, 2007\n", programname);
    fputs ("%%CreationDate: ", fp_out);
    time (&tp);
    fputs (asctime (localtime (&tp)), fp_out);

    if ((legtype == legPATTERNS) || (eXtended && pslevel == 2))
	fputs ("%%LanguageLevel: 2\n", fp_out);

    fputs (
        "%%EndComments\n"
        "\n"
        "64 dict begin\n"
	"\n",
	fp_out
    );
    fputs (legtype == legPATTERNS ? "/dy 16 def\n" : "/dy 12 def\n", fp_out);
    fputs (
	"\n"
	"/Fontname /Helvetica def\n"
	"/Fontsize ",
	fp_out
    );
    fprintf (fp_out, "%g def\n\n", (float) defFONTSIZE);

    max = 0;
    for (i = 0; i < nlines; i++)
	if (lines [i].i > max)
	    max = lines [i].i;

    switch (legtype) {
	case legCOLORS:
	    fputs (
		"/Rectsize 9 def\n"
		".5 setlinewidth\n"
		"\n",
		fp_out
	    );
	    if (use_rainbow) {
		fputs ("/SETCOLOR { sethsbcolor } bind def\n", fp_out);
		for (i = 0; i < max; i++)
		    fprintf (
		        fp_out,
			"/c%i [ %.4f 1 %s ] def\n",
			i + 1,
			((float) i) / max,
			((i % 2) && ! use_bright) ? ".6" : "1"
		    );
			 
	    } else {
		fputs ("/SETCOLOR { setrgbcolor } bind def\n", fp_out);
		for (i = 0; i < max; i++)
		    fprintf (
		        fp_out,
			"/c%i [ %.2f %.2f %.2f ] def\n",
			i + 1,
			use_usercolours ? usercolors [i][0] : colors [i][0],
			use_usercolours ? usercolors [i][1] : colors [i][1],
			use_usercolours ? usercolors [i][2] : colors [i][2]
		    );
	    }
	    fputs (
		"\n"
		"/nl {\n"
		"    /Y Y dy sub def\n"
		"} bind def\n"
		"\n"
		"/l {\n"
		"    120 Y shift sub moveto\n"
		"    show\n"
		"    110 Y moveto\n"
		"    Rectsize -2 div dup rmoveto\n"
		"    Rectsize 0 rlineto\n"
		"    0 Rectsize rlineto\n"
		"    Rectsize neg 0 rlineto\n"
		"    closepath\n"
		"    gsave\n"
		"        aload pop SETCOLOR fill\n"
		"    grestore\n"
		"    stroke\n"
		"    nl\n"
		"} bind def\n"
		"\n",
		fp_out
	    );
	    break;
	case legPATTERNS:
	    fputs (
		"/Rectsize 14 def\n"
		".3 setlinewidth\n"
		"\n"
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
	    for (i = 0; i < max; i++)
		fputs (pat [i], fp_out);
	    fputs (
		"\n"
		"/nl {\n"
		"    /Y Y dy sub def\n"
		"} bind def\n"
		"\n"
		"/l {\n"
		"    120 Y shift sub moveto\n"
		"    show\n"
		"    110 Y moveto\n"
		"    Rectsize -2 div dup rmoveto\n"
		"    Rectsize 0 rlineto\n"
		"    0 Rectsize rlineto\n"
		"    Rectsize neg 0 rlineto\n"
		"    closepath\n"
		"    gsave\n"
		"        /Pattern setcolorspace setcolor fill\n"
		"    grestore\n"
		"    stroke\n"
		"    nl\n"
		"} bind def\n"
		"\n",
		fp_out
	    );
	    break;
	case legSYMBOLS:
	    fputs (
		"/Symsize 8 def\n"
		"/Symlw .7 def\n"
		"\n",
		fp_out
	    );
	    for (i = 0; i < max; i++)
		fprintf (fp_out, "/c%i /sym%i def\n", i + 1, i);
	    for (i = 0; i < max; i++)
		fprintf (
		    fp_out,
		    "/sym%i {\n%s} bind def\n",
		    i,
		    sym [i]
		);
	    fputs (
		"/nl {\n"
		"    /Y Y dy sub def\n"
		"} bind def\n"
		"\n"
		"/l {\n"
		"    120 Y shift sub moveto\n"
		"    show\n"
		"    110 Y moveto\n"
		"    cvx exec\n"
		"    nl\n"
		"} bind def\n"
		"\n",
		fp_out
	    );
	    break;
    }

    fputs (
	"/Y 700 def\n"
	"\n",
	fp_out
    );

    if (eXtended && pslevel == 1)
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
        eXtended
          ? "/RE {\n"
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
            "/Font-ISOlat1 findfont Fontsize scalefont setfont\n"
            "\n"
          : "Fontname findfont Fontsize scalefont setfont\n"
            "\n",
            fp_out
    );

    fputs (
        "gsave\n"
        "    newpath\n"
        "    0 0 moveto\n"
        "    (x) false charpath\n"
        "    pathbbox\n"
        "grestore\n"
        "2 div /shift exch def\n"
        "pop\n"
        "pop\n"
        "pop\n"
	"\n",
	fp_out
    );

    for (i = 0; i < nlines; i++)
	if (lines [i].i == 0)
	    fputs ("nl\n", fp_out);
	else
	    fprintf (fp_out, "c%i (%s) l\n", lines [i].i, toPS (lines [i].s));

    fputs (
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

float psstringwidth (unsigned char const *s)
{
    float
        f;
    int
        i;

    f = 0.0;
    for (i = 0; s [i]; i++)
        f += helvetica [s [i]];

    return f / 1000.0 * (float) defFONTSIZE;
}

char const *toPS (unsigned char const *s)
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

BOOL_ GetLine ()
{
    int
        i;

    for (;;) {
        lineno++;
        if (fgets (buffer, BUFSIZE, fp_in) == NULL)
	    return FALSE;
        i = strlen (buffer);
        while (i > 0 && isspace ((unsigned char) buffer [i - 1]))
            buffer [--i] = '\0';
        i = 0;
        while (buffer [i] && isspace ((unsigned char) buffer [i]))
            i++;
        if (i)
            memmove (buffer, buffer + i, strlen (buffer + i) + 1);
        if (buffer [0] == '\0' || buffer [0] == '#')
            continue;
        return TRUE;
    }
}

void process_args ()
{
    while (arg_c > 1 && arg_v [1][0] == '-') {
        switch (arg_v [1][1]) {
	    case '2':
		pslevel = 2;
		break;
            case 'o':
                outfile = get_arg ();
                break;
            case 'p':
                legtype = legPATTERNS;
                break;
            case 'r':
                legtype = legCOLORS;
		use_rainbow = TRUE;
		use_bright = FALSE;
                break;
            case 'R':
                legtype = legCOLORS;
		use_rainbow = TRUE;
		use_bright = TRUE;
                break;
            case 's':
                legtype = legSYMBOLS;
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
	"Version " my_VERSION "\n"
	"\n"
	"Usage: %s [-2] [-o filename] [-u filename] [-p] [-r] [-R] [-s] legendfile\n"
	"\n"
	"\t-2 : PostScript level 2\n"
	"\t-o : output file\n"
	"\t-p : patterns\n"
	"\t-r : rainbow colours, light and dark\n"
	"\t-R : rainbow colours, light\n"
	"\t-s : symbols\n"
	"\t-u : user-defined colours\n"
	"\n",
	programname
    );
    exit (1);
}
