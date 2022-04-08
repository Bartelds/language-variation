/*
 * File: xstokens.c
 *
 * (c) Peter Kleiweg
 *     Fri Feb 21 17:42:24 2003
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
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum { FALSE = 0, TRUE } BOOL_;

typedef struct {
    BOOL_
        ignore;
    int
        lens,
	lenn,
	used;
    char
        t,
	*s;
    long int
        *n;
} TOKEN_;    

typedef struct _tokstr {
    char
        *s;
    long int
        n;
    struct _tokstr
        *left,
	*right;
} TOKSTR_;

#define BUFSIZE 4095

TOKEN_
    *tokens = NULL;

TOKSTR_
    *root = NULL;

char
    **arg_v,
    buffer [BUFSIZE + 1],
    tokbuf [BUFSIZE + 1],
    *infile,
    *savefile = NULL,
    *programname,
    *no_mem_buffer,
    out_of_memory [] = "Out of memory";

int
arg_c,
    offsets [256],
    n_tokens = 0,
    max_tokens = 0,
    outlength;

long int
    tokcount = 0,
    tokbuffer [BUFSIZE + 1],
    lineno;

FILE
    *fpin,
    *fpout;

void
    process_args (void),
    storeline (const char *s),
    tokflush (void),
    get_programname (char const *argv0),
    errit (char const *format, ...),
    syntax (void),
    *s_malloc (size_t size),
    *s_realloc (void *block, size_t size);

int
    cmpli (const void *p1, const void *p2),
    cmptok (const void *p1, const void *p2);

long int
    tokennum (void);

char
    *get_arg (void),
    *s_strdup (char const *s);

int main (int argc, char *argv [])
{
    BOOL_
	in_head,
	in_ignore;
    int
	found,
	i,
	j,
	k,
	l,
	p,
	pp;

    no_mem_buffer = (char *) malloc (1024);

    get_programname (argv [0]);

    arg_c = argc;
    arg_v = argv;
    process_args ();

    if (arg_c < 3)
	syntax ();

    infile = arg_v [1];
    fpin = fopen (infile, "r");
    if (! fpin)
	errit ("Opening file \"%s\": %s", infile, strerror (errno));

    lineno = 0;
    while (fgets (buffer, BUFSIZE, fpin) != NULL) {
	lineno++;
	for (i = 0; buffer [i] && isspace ((unsigned char) buffer [i]); i++)
	    ;
	switch (buffer [i]) {
	    case '\0':
	    case '#':
		break;
	    case 'S':
	    case 'U':
		storeline (buffer + i);
		break;
	    default:
		errit ("Illegal first character in file \"%s\", line %li", infile, lineno);
	}
    }
    fclose (fpin);

    qsort (tokens, n_tokens, sizeof (TOKEN_), cmptok);
    for (i = 0; i < 256; i++)
	offsets [i] = -1;
    for (i = 0; i < n_tokens; i++) {
	j = (unsigned char) (tokens [i].s [0]);
	if (offsets [j] < 0)
	    offsets [j] = i;
    }

    for (i = 2; i < arg_c; i++) {
	lineno = 0;
	infile = arg_v [i];
	fpin = fopen (infile, "r");
	if (! fpin)
	    errit ("Opening file \"%s\": %s", infile, strerror (errno));
	sprintf (buffer, "%s.tok", infile);
	fpout = fopen (buffer, "w");
	if (! fpout)
	    errit ("Creating file \"%s\": %s", buffer, strerror (errno));
	while (fgets (buffer, BUFSIZE, fpin) != NULL) {
	    lineno++;
	    p = 0;
	    pp = 0;
	    sscanf (buffer, " [ %*[0-9] ] %n", &pp);
	    if (pp) {
		p = buffer [pp];
		buffer [pp] = '\0';
		fputs (buffer, fpout);
		buffer [pp] = p;
		p = pp;
	    }
	    if (buffer [p] == '+')
		errit ("Illegal data in file \"%s\", line %lu", infile, lineno);
	    if (buffer [p] != '-') {
		fputs (buffer + p, fpout);
		continue;
	    }
	    fputs ("+", fpout);
	    j = strlen (buffer);
	    while (j > 0 && isspace ((unsigned char) buffer [j - 1]))
		   buffer [--j] = '\0';
	    j = p + 1;
	    while (buffer [j] && isspace ((unsigned char) buffer [j]))
		j++;
	    in_head = in_ignore = FALSE;
	    outlength = 0;
	    while (buffer [j]) {
		found = -1;
		l = 0;
		k = offsets [(unsigned char) buffer [j]];
		if (k >= 0)
		    for ( ; k < n_tokens && buffer [j] == tokens [k].s [0]; k++)
			if (tokens [k].lens > l && ! memcmp (buffer + j, tokens [k].s, tokens [k].lens * sizeof (char))) {
			    found = k;
			    l = tokens [k].lens;
			}
		if (found < 0)
		    errit ("Illegal token in file \"%s\", line %lu\n\t--> %s", infile, lineno, buffer + j);
		tokens [found].used++;
		if (tokens [found].t == 'U') {
		    tokflush ();
		    in_head = TRUE;
		    in_ignore = tokens [found].ignore;
		    if (! in_ignore)
			for (k = 0; k < tokens [found].lenn; k++)
			    fprintf (fpout, " %li", tokens [found].n [k]);
		} else {
		    if (! in_head)
			errit ("Sorted before unsorted tokens in file \"%s\", line %lu", infile, lineno);
		    if ((! in_ignore) && ! tokens [found].ignore)
			tokbuffer [outlength++] = found;
		}
		j += tokens [found].lens;
	    }
	    tokflush ();
	    fputs ("\n", fpout);
	    
	}
	fclose (fpout);
	fclose (fpin);
    }

    if (savefile) {
	fpout = fopen (savefile, "w");
	if (! fpout)
	    errit ("Creating file \"%s\": %s", savefile, strerror (errno));
	for (i = 0; i < n_tokens; i++) {
	    memcpy (buffer, tokens [i].s, tokens [i].lens * sizeof (char));
	    buffer [tokens [i].lens * sizeof (char)] = '\0';
	    fprintf (fpout, "%s\t%4i\n", buffer, tokens [i].used);
	}
	fclose (fpout);
	printf ("Token count saved to file \"%s\"\n", savefile);
    }

    return 0;
}

int cmpli (const void *p1, const void *p2)
{
    return (int) (*((long int *)p1) -  *((long int *)p2));
}

int cmptok (const void *p1, const void *p2)
{
    int
	i,
	l1,
	l2,
	l;

    l1 = ((TOKEN_ *)p1)->lens;
    l2 = ((TOKEN_ *)p2)->lens;
    l = (l1 > l2) ? l2 : l1;

    i = memcmp (((TOKEN_ *)p1)->s, ((TOKEN_ *)p2)->s, l);
    if (i)
	return i;
    return l1 - l2;
}

void tokflush ()
{
    int
	i,
	j;

    if (outlength == 0)
	return;

    qsort (tokbuffer, outlength, sizeof (long int), cmpli);
    for (i = 0; i < outlength; i++)
	for (j = 0; j < tokens [tokbuffer [i]].lenn; j++)
	    fprintf (fpout, " %li", tokens [tokbuffer [i]].n [j]);

    outlength = 0;
}

void storeline (char const *s)
{
    char
	*str,
	t;
    int
	i,
	lens,
	lenn;
    BOOL_
	ignore;

    t = s [0];
    s++;

    if (s [0] == 'I') {
	ignore = TRUE;
	s++;
    } else
	ignore = FALSE;
    
    if ((! s [0]) || ! isspace ((unsigned char) s [0]))
	errit ("Syntax error in file \"%s\", line %li", infile, lineno);

    while (s [0] && isspace ((unsigned char) s [0]))
	s++;

    if (! s [0])
	errit ("Missing token in file \"%s\", line %li", infile, lineno);

    for (lens = 0; s [lens] && ! isspace ((unsigned char) s [lens]); lens++)
	;
    str = (char *) s_malloc (lens * sizeof (char));
    memcpy (str, s, lens * sizeof (char));
    s += lens;

    lenn = 0;
    for (;;) {
	while (s [0] && isspace ((unsigned char) s [0]))
	    s++;
	if (! s [0])
	    break;
	if (sscanf (s, "%s%n", tokbuf, &i) < 1)
	    errit ("Not a token in file \"%s\", line %li", infile, lineno);
	tokbuffer [lenn++] = tokennum ();
	s += i;
    }
    if ((! lenn) && ! ignore)
	errit ("Missing tokens in file \"%s\", line %li", infile, lineno);

    if (n_tokens == max_tokens) {
	max_tokens += 256;
	tokens = (TOKEN_ *) s_realloc (tokens, max_tokens * sizeof (TOKEN_));
    }
    tokens [n_tokens].t = t;
    tokens [n_tokens].ignore = ignore;
    tokens [n_tokens].lens = lens;
    tokens [n_tokens].lenn = lenn;
    tokens [n_tokens].s = str;
    tokens [n_tokens].used = 0;

    if (! ignore) {
	tokens [n_tokens].n = (long int *) s_malloc (lenn * sizeof (long int));
	memcpy (tokens [n_tokens].n, tokbuffer, lenn * sizeof (long int));
    }
    n_tokens++;
}

TOKSTR_ *newtokstr ()
{
    TOKSTR_
	*t;

    t = (TOKSTR_ *) s_malloc (sizeof (TOKSTR_));
    t->s = s_strdup (tokbuf);
    t->n = ++tokcount;
    t->left = t->right = NULL;
    return t;
}

long int tokennum ()
{
    int
	i;
    TOKSTR_
	*tmp;

    if (! strcmp (tokbuf, "."))
	return ++tokcount;

    if (root == NULL) {
	root = newtokstr ();
	return root->n;
    }

    tmp = root;
    for (;;) {
	i = strcmp (tmp->s, tokbuf);
	if (i == 0)
	    return tmp->n;
	if (i < 0) {
	    if (tmp->left)
		tmp = tmp->left;
	    else {
		tmp->left = newtokstr ();
		return tmp->left->n;
	    }
	} else {
	    if (tmp->right)
		tmp = tmp->right;
	    else {
		tmp->right = newtokstr ();
		return tmp->right->n;
	    }
	}
    }

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


void process_args ()
{
    while (arg_c > 1 && arg_v [1][0] == '-') {
        switch (arg_v [1][1]) {
            case 's':
                savefile = get_arg ();
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

void syntax ()
{
    fprintf (
	stderr,
	"\n"
	"Version " my_VERSION "\n"
	"\n"
	"Usage: %s [-s filename] xstable *.fon\n"
	"\n"
	"\t-s : save token count to file\n"
	"\n",
	programname
    );
    exit (1);
}
