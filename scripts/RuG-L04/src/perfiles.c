/*
 * File: perfiles.c
 *
 * (c) Peter Kleiweg
 *     Mon Jul  7 20:16:58 2003,
 *         2007
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

typedef enum { FALSE = 0, TRUE } BOOL_;

typedef struct label_ {
    char
        *label,
	*filename;
    struct label_
        *left,
	*right;
} LABEL_;

LABEL_
    *root = NULL;

#define BUFSIZE 4090

char
    buffer [BUFSIZE + 1],
    scanbuf [BUFSIZE + 1],
    namebuf [BUFSIZE + 5];

char
    *infile,
    unterminated [] = "Unterminated string in file \"%s\", line %i",
    nper [] = "a-%03i.per",
    *programname,
    *no_mem_buffer,
    out_of_memory [] = "Out of memory";

int
    lineno,
    nlines = 0;

BOOL_
    raw = FALSE,
    fixed = FALSE;

FILE
    *fpin = NULL,
    *fpout = NULL;

void
    get_programname (char const *argv0),
    errit (char const *format, ...),
    syntax (void),
    *s_malloc (size_t size);
char
    *s_strdup (char const *s);
int
    scanword (int start);


int main (int argc, char *argv [])
{
    char
	c;
    int
	argi,
	i,
	p,
	q;
    BOOL_
	found;
    LABEL_
	*pLabel;

    no_mem_buffer = (char *) malloc (1024);

    get_programname (argv [0]);

    while (argc > 1 && argv [1][0] == '-') {
	if (! strcmp (argv [1], "-r"))
	    raw = TRUE;
	else if (argv [1][1] == 'n') {
	    fixed = TRUE;
	    if (argv [1][2])
		nlines = atoi (argv [1] + 2);
	    else {
		if (argc < 3)
		    errit ("Missing argument for option -n");
		argv++;
		argc--;
		nlines = atoi (argv [1]);
	    }
	    if (nlines < 1)
		errit ("Missing or invalid value for option -n");
	    for (i = 1; i <= nlines; i++) {
		sprintf (namebuf, nper, i);
		if (! access (namebuf, F_OK))
		    errit ("File \"%s\" already exists", namebuf);
	    }
	} else if (! strcmp (argv [1], "--")) {
	    argc--;
	    argv++;
	    break;
	} else
	    errit ("Unvalid option: %s", argv [1]);
	argc--;
	argv++;
    }

    if (argc == 1)
	syntax ();

    for (argi = 1; argi < argc; argi++) {
	infile = argv [argi];
	fpin = fopen (infile, "r");
	if (! fpin)
	    errit ("Opening file \"%s\": %s", infile, strerror (errno));
	lineno = 0;
	while (fgets (buffer, BUFSIZE, fpin) != NULL) {
	    lineno++;

	    if (fixed) {

		if (lineno > nlines)
		    errit ("Too many lines in file \"%s\"", infile);
		sprintf (namebuf, nper, lineno);
		p = 0;

	    } else {

		/* scan for label */
		p = scanword (0);
		if (scanbuf [0] == '\0')
		    break;

		/* convert label to filename */
		q = 0;
		for (i = 0; (c = scanbuf [i]) != '\0'; i++) {
		    if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '+' || c == '_')
			namebuf [q++] = c;
		    else if (c >= 'A' && c <= 'Z')
			namebuf [q++] = c - 'A' + 'a';
		    else
			namebuf [q++] = '_';
		}
		strcpy (namebuf + q, ".per");

		/* locate it */
		found = FALSE;
		if (root) {
		    pLabel = root;
		    for (;;) {
			i = strcmp (pLabel->filename, namebuf);
			if (!i) {
			    if (strcmp (pLabel->label, scanbuf))
				errit ("Labels \"%s\" and \"%s\" both translate to filename \"%s\"", pLabel->label, scanbuf, namebuf);
			    found = TRUE;
			    break;
			} else if (i < 0) {
			    if (pLabel->left)
				pLabel = pLabel->left;
			    else {
				pLabel = pLabel->left = (LABEL_ *) s_malloc (sizeof (LABEL_));
				break;
			    }
			} else {
			    if (pLabel->right)
				pLabel = pLabel->right;
			    else {
				pLabel = pLabel->right = (LABEL_ *) s_malloc (sizeof (LABEL_));
				break;
			    }
			}
		    }
		} else
		    root = pLabel = (LABEL_ *) s_malloc (sizeof (LABEL_));
		if (! found) {
		    if (! access (namebuf, F_OK))
			errit ("File \"%s\" already exists", namebuf);
		    pLabel->label = s_strdup (scanbuf);
		    pLabel->filename = s_strdup (namebuf);
		    pLabel->left = pLabel->right = NULL;
		}
		
	    }


	    /* scan for strings */
	    for (;;) {
		p = scanword (p);
		if (scanbuf [0] == '\0')
		    break;
		if (! fpout) {
		    fpout = fopen (namebuf, "a");
		    if (! fpout)
			errit ("Creating/appending file \"%s\": %s", namebuf, strerror (errno));
		    fprintf (fpout, ": %s\n", argv [argi]);
		}
		fprintf (fpout, "- %s\n", scanbuf);
	    }

	    if (fpout != NULL) {
		fclose (fpout);
		fpout = NULL;
	    }
	}
	fclose (fpin);
	if (fixed && lineno <  nlines)
	    errit ("Too few lines in file \"%s\"", infile);
    }

    return 0;
}

int scanword (int offset)
{
    int
	p,
	state;
    BOOL_
	quotes;

    while (buffer [offset] && isspace ((unsigned char) buffer [offset]))
	offset++;

    if (buffer [offset] == '#') {
	scanbuf [0] = '\0';
	return offset;
    }

    p = 0;

    if (raw) {
	while (buffer [offset] != '\0' && ! isspace ((unsigned char) buffer [offset]))
	    scanbuf [p++] = buffer [offset++];
	scanbuf [p] = '\0';
	return offset;
    }

    quotes = (buffer [offset] == '"') ? TRUE : FALSE;
    if (quotes)
	offset++;

    state = 0;
    for (;;) {
	if (buffer [offset] == '\0') {
	    if (quotes)
		errit (unterminated, infile, lineno);
	    break;
	} else if (buffer [offset] == '\\') {
	    offset++;
	    if (buffer [offset] == '\0')
		errit (unterminated, infile, lineno);
	    if (buffer [offset] != ' ' && buffer [offset] != '"' && buffer [offset] != '\\' && buffer [offset] != '#')
		errit ("Invalid quoted character \\%c, in file \"%s\", line %i", buffer [offset], infile, lineno);
	    scanbuf [p++] = buffer [offset++];
	} else if (buffer [offset] == '"') {
	    offset++;
	    if ((buffer [offset] != '\0' && ! isspace ((unsigned char) buffer [offset])) || ! quotes)
		errit ("Unescaped quote in string, in file \"%s\", line %i", infile, lineno);
	    break;
	} else if (isspace ((unsigned char) buffer [offset])) {
	    if (quotes) {
		scanbuf [p++] = ' ';
		offset++;
	    } else
		break;
	} else
	    scanbuf [p++] = buffer [offset++];
    }

    scanbuf [p] = '\0';
    while (p && scanbuf [p - 1] == ' ')
	scanbuf [--p] = '\0';

    return offset;
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
	"Usage: %s [-n number] [-r] filenames\n"
	"\n"
	"\t-n : no labels. fixed number of lines in all input files\n"
	"\t-r : raw input (no quotes, no escape sequences)\n"
	"\n",
	programname
    );
    exit (1);
}
