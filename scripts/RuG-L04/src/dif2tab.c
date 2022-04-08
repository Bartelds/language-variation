/*
 * File: dif2tab.c
 *
 * (c) Peter Kleiweg
 *     Wed May  1 12:25:24 2002
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2,
 * or (at your option) any later version.
 *
 */

#define my_VERSION "0.03"

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
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUFSIZE 2048

typedef enum { FALSE = 0, TRUE } BOOL;

int
    size,
    arg_c;

long int
    input_line = 0;

char
    delim = '\t',
    *na = "NA",
    *infile = NULL,
    *outfile = NULL,
    **names,
    ***d,
    buffer [BUFSIZE + 1],
    ebuf [2 * BUFSIZE + 1],
    **arg_v,
    *programname,
    *no_mem_buffer,
    out_of_memory [] = "Out of memory";

BOOL
    labels = TRUE,
    diagonal = TRUE,
    bottom = TRUE,
    top = TRUE;

FILE
    *fp_in,
    *fp_out;

void
    *s_malloc (size_t size),
    get_programname (char const *argv0),
    process_args (void),
    errit (char const *format, ...),
    syntax (void);
char
    *get_arg (void),
    *s_strdup (char const *s);
char const
    *escape (char const *s),
    *unquote (char *s);
BOOL
    GetLine ();

int main (int argc, char *argv [])
{
    int
	i,
	j,
	n;
    double
	f;
    BOOL
	sep;

    no_mem_buffer = (char *) malloc (1024);

    get_programname (argv [0]);

    arg_c = argc;
    arg_v = argv;
    process_args ();

    switch (arg_c) {
	case 1:
	    if (isatty (fileno (stdin)))
		syntax ();
	    break;
	case 2:
	    infile = arg_v [1];
	    break;
	default:
	    syntax ();
    }

    if (infile) {
	fp_in = fopen (infile, "r");
	if (! fp_in)
	    errit ("Reading file \"%s\": %s", infile, strerror (errno));
    } else {
	fp_in = stdin;
	infile = "<stdin>";
    }

    GetLine ();
    size = atoi (buffer);
    if (size < 1)
	errit ("Illegal table size in file \"%s\", line %li", infile, input_line);

    if (labels)
	names = (char **) s_malloc (size * sizeof (char *));

    for (i = 0; i < size; i++) {
	GetLine ();
	if (labels)
	    names [i] = s_strdup (escape (unquote (buffer)));
    }

    d = (char ***) s_malloc (size * sizeof (char **));
    for (i = 1; i < size; i++)
	d [i] = (char **) s_malloc (i * sizeof (char *));
    
    for (i = 1; i < size; i++)
	for (j = 0; j < i; j++) {
	    GetLine ();
	    if (sscanf (buffer, "%lf%n", &f, &n) >= 1) {
		buffer [n] = '\0';
		d [i][j] = s_strdup (buffer);
	    } else
		d [i][j] = na;
	}

    if (fp_in != stdin)
	fclose (fp_in);
    
    if (outfile != NULL) {
	fp_out = fopen (outfile, "w");
	if (! fp_out)
	    errit ("Creating file \"%s\": %s", outfile, strerror (errno));
    } else
	fp_out = stdout;

    for (i = 0; i < size; i++) {
	if (i == 0 && !labels && !diagonal && !top)
	    continue;

	sep = FALSE;

	if (labels) {
	    fprintf (fp_out, "\"%s\"", names [i]);
	    sep = TRUE;
	}

	if (bottom)
	    for (j = 0; j < i; j++) {
		if (sep)
		    fputc (delim, fp_out);
		fputs (d [i][j], fp_out);
		sep = TRUE;
	    }

	if (diagonal) {
	    if (sep)
		fputc (delim, fp_out);
	    fputc ('0', fp_out);
	    sep = TRUE;
	}

	if (top)
	    for (j = i + 1; j < size; j++) {
		if (sep)
		    fputc (delim, fp_out);
		fputs (d [j][i], fp_out);
		sep = TRUE;
	    }

	fputc ('\n', fp_out);
    }

    if (fp_out != stdout)
        fclose (fp_out);

    return 0;
}

BOOL GetLine (BOOL required)
/* Lees een regel in
 * Plaats in buffer
 * Negeer lege regels en regels die beginnen met #
 * Verwijder leading/trailing white space
 */
{
    int
        i;

    for (;;) {
        if (fgets (buffer, BUFSIZE, fp_in) == NULL) {
            if (required)
                errit ("Unexpected end of file in \"%s\"", infile);
            else
                return FALSE;
        }
        input_line++;
        i = strlen (buffer);
        while (i && isspace ((unsigned char) buffer [i - 1]))
            buffer [--i] = '\0';
        i = 0;
        while (buffer [i] && isspace ((unsigned char) buffer [i]))
            i++;
        if (buffer [i] == '#')
            continue;
        if (buffer [i]) {
            memmove (buffer, buffer + i, strlen (buffer + i) + 1);
            return TRUE;
        }
    }
}

/*
 * remove quotes from string, overwrite with result
 */
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

/*
 * add escapes for '\' and '"'
 */
char const *escape (char const *s)
{
    int
        i,
        j;

    j = 0;
    for (i = 0; s [i]; i++) {
        if (s [i] == '"' || s [i] == '\\')
            ebuf [j++] = '\\';
        ebuf [j++] = s [i];
    }
    ebuf [j] = '\0';
    return ebuf;
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

void process_args ()
{
    while (arg_c > 1 && arg_v [1][0] == '-') {
        switch (arg_v [1][1]) {
	    case 'b':
		bottom = TRUE;
		diagonal = top = FALSE;
		break;
	    case 'B':
		bottom = diagonal = TRUE;
		top = FALSE;
		break;
            case 'c':
		delim = ',';
                break;
            case 'n':
                labels = FALSE;
                break;
            case 'o':
                outfile = get_arg ();
                break;
	    case 't':
		top = TRUE;
		diagonal = bottom = FALSE;
		break;
	    case 'T':
		top = diagonal = TRUE;
		bottom = FALSE;
		break;
            case 'u':
                na = get_arg ();
                break;
            case 'U':
                na = "";
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
	"Convert difference table file into tab-delimited file, "
	"Version " my_VERSION "\n"
	"(c) P. Kleiweg 2002\n"
	"\n"
	"Usage: %s [-b] [-B] [-c] [-n] [-o filename] [-t] [-T] [-u string] [-U] [difference table file]\n"
	"\n"
	"\t-b : bottom half only, without diagonal\n"
	"\t-B : bottom half only, with diagonal\n"
	"\t-c : use comma instead of tab as delimiter\n"
	"\t-n : no labels\n"
	"\t-o : output file\n"
	"\t-t : top half only, without diagonal\n"
	"\t-T : top half only, with diagonal\n"
	"\t-u : string to use for missing values (default: NA)\n"
	"\t-U : use empty string for missing values\n"
	"\n",
	programname
    );
    exit (1);
}
