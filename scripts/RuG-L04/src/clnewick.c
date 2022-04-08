/*
 * File: clnewick.c
 *
 * (c) Peter Kleiweg
 *     Fri Apr 25 14:26:15 2008
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

#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUFSIZE 1024

typedef enum { CLS, LBL, NONE } NODETYPE;
typedef enum { FALSE = 0, TRUE = 1} BOOL;

typedef struct {
    int
        index;
    float
        value [3];
    char
        *text;
    NODETYPE
        node [3];
    union {
        int
            cluster;
        char
            *label;
    } n [3];
} CLUSTER;


CLUSTER
    *cl = NULL;

char
    *outfile = NULL,
    buffer [BUFSIZE + 1],
    **arg_v,
    *programname,
    *no_mem_buffer,
    out_of_memory [] = "Out of memory";

int
    useVALUES = 0,
    inputline = 0,
    outlen = 0,
    max = 0,
    used = 0,
    top,
    arg_c;

FILE
    *fp,
    *fp_out;

void
    trim (void),
    validstring (void),
    output (char const *),
    foutput (float),
    get_programname (char const *argv0),
    process_args (void),
    errit (char const *format, ...),
    syntax (void),
    *s_malloc (size_t size),
    *s_realloc (void *block, size_t size);
char
    *get_arg (void),
    *s_strdup (char const *s);

float
    process (int n);

BOOL
    GetLine (BOOL required);

int main (int argc, char *argv [])
{
    int
	i,
	j = 0,
	k,
	n;
    BOOL
        found;

    no_mem_buffer = (char *) malloc (1024);

    get_programname (argv [0]);

    arg_c = argc;
    arg_v = argv;
    process_args ();

    if (outfile) {
        fp_out = fopen (outfile, "w");
        if (! fp_out)
            errit ("Creating file \"%s\": %s", outfile, strerror (errno));
    } else
        fp_out = stdout;

    switch (arg_c) {
    case 1:
	if (isatty (fileno (stdin)))
	    syntax ();
	fp = stdin;
	break;
    case 2:
	fp = fopen (arg_v [1], "r");
	if (! fp)
	    errit ("Opening file \"%s\": %s", arg_v [1], strerror (errno));
	break;
    default:
	syntax ();
    }
    
    while (GetLine (FALSE)) {
        if (used == max) {
            max += 256;
            cl = (CLUSTER *) s_realloc (cl, max * sizeof (CLUSTER));
        }
	cl [used].node [2] = NONE;
	if (! useVALUES) {
	    if (buffer [0] == ':')
		useVALUES = 2;
	    else
		useVALUES = 1;
	}
	if (useVALUES > 1) {
	    if (buffer [1] == ':')
		useVALUES = 3;
	    else
		useVALUES = 2;
	}
	switch (useVALUES) {
	case 1:
	    j = sscanf (buffer, "%i %g%n",
			&(cl [used].index),
			&(cl [used].value [0]),
			&i);
	    break;
	case 2:
	    j = sscanf (buffer, ": %i %g %g%n",
			&(cl [used].index),
			&(cl [used].value [0]),
			&(cl [used].value [1]),
			&i);
	    break;
	case 3:
	    j = sscanf (buffer, ":: %i %g %g %g%n", 
			&(cl [used].index),
			&(cl [used].value [0]),
			&(cl [used].value [1]),
			&(cl [used].value [2]), 
			&i);
	    break;
	}
	if (j < useVALUES + 1)
	    errit ("Syntax error at line %i: \"%s\"", inputline, buffer);

        memmove (buffer, buffer + i, strlen (buffer + i) + 1);
        trim ();
        if (buffer [0] && buffer [0] != '#') {
            validstring ();
            cl [used].text = s_strdup (buffer);
        } else
            cl [used].text = NULL;
        for (n = 0; n < ((useVALUES == 3) ? 3 : 2); n++) {
            GetLine (TRUE);
            switch (buffer [0]) {
	    case 'l':
	    case 'L':
		cl [used].node [n] = LBL;
		buffer [0] = ' ';
		trim ();
		validstring ();
		cl [used].n [n].label = s_strdup (buffer);
		break;
	    case 'c':
	    case 'C':
		cl [used].node [n] = CLS;
		if (sscanf (buffer + 1, "%i", &(cl [used].n [n].cluster)) != 1)
		    errit ("Missing cluster number at line %i", inputline);
		break;
	    default:
		errit ("Syntax error at line %i: \"%s\"", inputline, buffer);
            }
        }
        used++;
    }
    

    if (argc == 2)
        fclose (fp);

    if (!used)
        errit ("No data");

    /* replace indexes */
    for (i = 0; i < used; i++)
        for (j = 0; j < 3; j++)
            if (cl [i].node [j] == CLS)
                for (k = 0; k < used; k++)
                    if (cl [i].n [j].cluster == cl [k].index) {
                        cl [i].n [j].cluster = k;
                        break;
                    }

    /* locate top node */
    top = 0;
    do {
        found = FALSE;
        for (i = 1; i < used; i++)
            if ((cl [i].node [0] == CLS && cl [i].n [0].cluster == top) ||
                (cl [i].node [1] == CLS && cl [i].n [1].cluster == top) ||
                (cl [i].node [2] == CLS && cl [i].n [2].cluster == top)
	    ) {
                top = i;
                found = TRUE;
                break;
            }
    } while (found);

    process (top);

    output (";");

    if (outlen > 0)
	fputc ('\n', fp_out);

    if (outfile)
        fclose (fp_out);

    return 0;
}

float process (int i)
{
    int
        j,
	n;
    float
	f;

    output ("(");

    n = (cl [i].node [2] == NONE) ? 2 : 3;
    for (j = 0; j < n; j++) {
	if (j > 0)
	    output (",");
        if (cl [i].node [j] == CLS) {
            f = process (cl [i].n [j].cluster);
	    output (":");
	    if (useVALUES == 1)
		foutput (cl [i].value [0] - f);
	    else
		foutput (cl [i].value [j]);
	} else {
	    output (cl [i].n [j].label);
	    output (":");
	    if (useVALUES == 1)
		foutput (cl [i].value [0]);
	    else
		foutput (cl [i].value [j]);
        }
    }

    output (")");

    if (cl [i].text)
        output (cl [i].text);

    return cl [i].value [0];
}

void foutput (float f)
{
    sprintf (buffer, "%g", f);
    output (buffer);
}

void output (char const *s)
{
    outlen += strlen (s);
    if (outlen > 72) {
	outlen = 0;
	fputc ('\n', fp_out);
    }
    fputs (s, fp_out);
}


BOOL GetLine (BOOL required)
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
                errit ("Unexpected end of file");
            else
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

void trim ()
{
    int
        i;

    i = 0;
    while (buffer [i] && isspace ((unsigned char) buffer [i]))
        i++;
    if (i)
        memmove (buffer, buffer + i, strlen (buffer + i) + 1);

    i = strlen (buffer);
    while (i && isspace ((unsigned char) buffer [i - 1]))
        buffer [--i] = '\0';
}

void process_args ()
{
    while (arg_c > 1 && arg_v [1][0] == '-') {
        switch (arg_v [1][1]) {
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

void validstring ()
{
    int
	i;

    for (i = 0; buffer [i]; i++)
	if (isspace ((unsigned char) buffer [i]) ||
	    buffer [i] == '('  ||
	    buffer [i] == ')'  ||
	    buffer [i] == '['  ||
	    buffer [i] == ']'  ||
	    buffer [i] == '\'' ||
	    buffer [i] == ':'  ||
	    buffer [i] == ';'  ||
	    buffer [i] == ',' 
	)
	    buffer [i] = '_';
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
	"Translate a cluster file to the Newick tree format, version " my_VERSION "\n"
	"(c) P. Kleiweg 2008\n"
	"\n"
	"Usage: %s [-o filename] [cluster_file] > newick_file\n"
	"\n"
        "\t-o  : output file\n"
	"\n",
	programname
    );
    exit (1);
}
