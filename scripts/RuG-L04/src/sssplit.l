/*
 * File: sssplit.l
 *
 * (c) Peter Kleiweg
 *     Mon Nov 14 14:16:57 2005
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2,
 * or (at your option) any later version.
 *
 * Compile:
 *
 *    flex sssplit.l
 *    gcc -s -Wall -o sssplit lex.yy.c -lfl
 *    rm lex.yy.c
 *
 * Goal: Split up a spreadsheet in into a set of files that
 * can be processed by 'leven' or 'giw'.
 *
 * Spreadsheet, general format:
 *
 *         PLACE1, PLACE2, PLACE3
 *   WRD1, item11, item12, item13
 *   WRD2, item21, item22, item23
 *   WRD3, item31, item32, item33
 *
 *
 * Column labels are required.
 * Row labels are optional. If present, the line with column labels can
 * have an extra empty label at the beginning of the line.
 *
 * You can use a TAB instead of a comma to separate labels and/or items.
 * You can NOT mix TABs and comma's.
 *  
 * Comment lines and empty lines are ignored
 * A comments starts with a #
 *
 * A data cell can be empty, it can have one item, or several items
 * separated by a semicolon.
 * Items can be put in double quotes.
 * Leading and trailing white space is removed
 * 
 * Example data cells:
 * item       # one item: "item"
 * " item  "  # one item: "item"
 * it; em     # two items: "it" and "em"
 * "it; em"   # one item: "it; em"
 * "it, em"   # one item: "it, em"
 * it\; em    # one item: "it; em"
 * it\, em    # one item: "it, em"
 * it\\em     # one item: "it\em"
 * ""         # no items
 *            # (empty) no items
 */

%{

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
#  define F_OK 0
#else
#  include <unistd.h>
#endif  /* __MSDOS__  */
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFSIZE 4096

typedef enum { FALSE = 0, TRUE } BOOL_;

typedef struct {
    char
	**word;
    int
	n_word;
} FIELD_;

FIELD_
    *field = NULL;

BOOL_
    useTab = FALSE,
    useComma = FALSE,
    useRowlabels = FALSE,
    fSafe = FALSE,
    testRowlabels = TRUE;

int
    body_columns = 0,
    dataline = -1,
    n_column,
    n_field = 0,
    max_field = 0,
    n_word = 0,
    max_word = 0,
    buflen = 0,
    lineno = 1,
    arg_c;

char
    **column = NULL,
    **word = NULL,
    buffer [BUFSIZE + 1],
    bufferq [BUFSIZE + 1],
    bufferfn [3 * BUFSIZE + 1],
    bufferfs [3 * BUFSIZE + 1],
    bufrowlab [BUFSIZE + 1],
    buffer2 [10],
    *dest = "out",
    **arg_v,
    *programname,
    *no_mem_buffer,
    out_of_memory [] = "Out of memory";

FILE
    *fpout = NULL;

void
    setTab (void),
    setComma (void),
    endline (void),
    endfield (void),
    endword (void),
    chr (char c),
    chrs (char const *),
    parse_error (char const *s),
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
    *fsafe( char const *s),
    *quote (char const *s);

%}

%option batch noyywrap nounput 8bit caseful reject

NL	(\r|\n|\r\n|\n\r)

%Start _quoted _unquoted _sep

%%

<INITIAL>{

\#.*		;

[ ]+            ;

{NL}		{ endline ();
		  lineno++;
		}

\t		{ if (! useComma) {
		      setTab ();
		      endfield ();
		  }
		}

","		{ if (! useTab) {
		      setComma ();
		      endfield ();
		  } else {
		      yyless (0);
		      BEGIN _unquoted;
		  }
		}

\"		{ BEGIN _quoted;
		}

.		{ yyless (0);
		  BEGIN _unquoted;
		}
}

<_sep>{

\#.*		;

[ ]+		;

{NL}		{ endline ();
		  lineno++;
		  BEGIN INITIAL;
		}

\t		{ if (useComma) {
		      REJECT;
		  } else {
		      setTab ();
		      endfield ();
		      BEGIN INITIAL;
		  }
		}

","		{ if (useTab) {
		      REJECT;
		  } else {
		      setComma ();
		      endfield ();
		      BEGIN INITIAL;
		  }
		}

";"		{ endword ();
		  BEGIN INITIAL;
		}

.|\n		{ parse_error ("_sep"); }

}

<_quoted>{

\"		{
		  BEGIN _sep;
		}

\\.		{ chr (yytext [1]); }

[^\"\\\n\r\t]+	{ chrs (yytext); }

.		{ parse_error ("_quoted"); }

}

<_unquoted>{

[ ]\#.*		;

\\.		{ chr (yytext [1]); }

\t		{ if (useComma) {
		      parse_error ("_unquoted");
		  } else {
		      yyless (0);
		      BEGIN _sep;
		  }
		}

","		{ if (useTab)
		      chr (yytext [0]);
		  else {
		      yyless (0);
		      BEGIN _sep;
		  }
		}

";"		{ yyless (0);
		  BEGIN _sep;
		}

{NL}		{ yyless (0);
		  BEGIN _sep;
		}

.               { chr (yytext [0]); }

}

%%

int main (int argc, char *argv [])
{
    no_mem_buffer = (char *) malloc (1024);

    get_programname (argv [0]);

    arg_c = argc;
    arg_v = argv;
    process_args ();

    switch (arg_c) {
        case 1:
            if (isatty (fileno (stdin)))
                syntax ();
            yyin = stdin;
            break;
        case 2:
            yyin = fopen (arg_v [1], "r");
            if (! yyin)
                errit ("Opening file \"%s\": %s", arg_v [1], strerror (errno));
            break;
        default:
            syntax ();
    }

    yylex ();
    endline();

    if (yyin != stdin)
        fclose (yyin);


    return 0;
}

void setTab ()
{
    if (! useTab) {
	useTab = TRUE;
	fprintf (stderr, "Fields delimited by tab\n");
    }
}

void setComma ()
{
    if (! useComma) {
	useComma = TRUE;
	fprintf (stderr, "Fields delimited by comma\n");
    }
}

void closefile ()
{
    if (fpout) {
	fclose (fpout);
	fpout = NULL;
    }
}

char const *fsafe (char const *s)
{
    int
	i, j;
    unsigned char
	c;
    if (! fSafe)
	return s;
    j = 0;
    for (i = 0; s [i]; i++) {
	c = (unsigned char) s [i];
	if (isalnum (c) || c == '-') {
	    bufferfs [j++] = c;
	} else {
	    sprintf (bufferfs + j, "_%02X", (int) c);
	    j += 3;
	}
    }
    bufferfs [j] = '\0';
    return bufferfs;
}

void openfile (char const *filename, BOOL_ lblfile)
{
    if (fpout)
	closefile ();

    if (lblfile)
	sprintf (bufferfn, "%s/_LABELS_.LBL", dest);
    else
	sprintf (bufferfn, "%s/%s.txt", dest, fsafe (filename));
    if (access (bufferfn, F_OK) == 0)
	errit ("File exists: %s", bufferfn);

    fpout = fopen (bufferfn, "w");
    if (! fpout)
	errit ("Creating file \"%s\": %s", bufferfn, strerror (errno)); 
}

void out (int i, int j, char const *s)
{
    if (fpout == NULL)
	errit ("This is a bug (out)");
    if (! j)
	fprintf (fpout, ": %s\n", quote(column [i]));
    fprintf (fpout, "- %s\n", s);
}

/*
 * Used for place names only
 *
 * If there are specials, then use double quotes and escapes, otherwise, use it raw
 *
 * specials are: " \
 */
char const *quote (char const *s)
{
    int
	i,
	j;

    for (i = 0; s [i]; i++)
	if (s [i] == '"' || s [i] == '\\')
	    break;
    if (! s [i])
	return s;

    j = 0;
    bufferq [j++] = '"';
    for (i = 0; s [i]; s++) {
	if (s [i] == '"' || s [i] == '\\')
	    bufferq [j++] = '\\';
	bufferq [j++] = s [i];
    }
    bufferq [j++] = '"';
    bufferq [j] = '\0';
    return bufferq;
}

void endline ()
{
    int
	i,
	j,
	skip;

    /* line with zero fields, or perhaps one empty field */
    if (! n_field)
	return;

    endfield ();

    dataline++;   /* lines not counting empty lines and comment lines */

    if (dataline == 0) {
	/* column labels in line 0 */
	skip = 0;
	if (field [0].n_word == 0) {
	    /* first column label empty -> there must be row labels also */
	    useRowlabels = TRUE;
	    testRowlabels = FALSE;  /* no need to further test for row labels */
	    body_columns = n_field;

	    skip = 1;
	    n_column = n_field - 1;
	} else {
	    skip = 0;
	    n_column = n_field;
	}
	/* save column labels */
	openfile ("", TRUE);
	column = (char **) s_malloc (n_column * sizeof (char *));
	for (i = 0; i < n_column; i++) {
	    if (field [i + skip].n_word != 1)
		errit ("Label error in column %i", i + 1);
	    column [i] = field [i + skip].word [0];
	    fprintf (fpout, "%4i\t%s\n", i + 1, quote(column [i]));
	}
	closefile ();
    } else if (testRowlabels) {
	testRowlabels = FALSE;
	/* first data row with test for row labels*/
	skip = 0;
	if (n_field == n_column) {
	    /* no extra data column -> there are no row labels */
	    useRowlabels = FALSE;
	    skip = 0;
	    body_columns = n_column;
	    if (field [0].n_word != 1)
		errit ("Label error in row 1");
	    openfile ("001", FALSE);
	} else if (n_field == n_column + 1) {
	    /* one extra data column -> first column is row label */
	    useRowlabels = TRUE;
	    skip = 1;
	    body_columns = n_column + 1;   /* 1 row label, plus data columns */
	    openfile (field [0].word [0], FALSE);
	} else
	    errit ("Invalid number of columns in row 1");

	for (i = 0; i < n_column; i++)
	    for (j = 0; j < field [i + skip].n_word; j++)
		out (i, j, field [i + skip].word [j]);
    } else {
	/* more rows -> already done */
	if (n_field < body_columns)
	    errit ("Missing field(s) in row %i", dataline);
    }

    /* close file for this line */
    closefile ();

    /* reset number of fields for next line */
    n_field = 0;

    /* clear row label for next line */
    bufrowlab [0] = '\0';

#ifdef DEBUG
    printf ("LINE\n");
#endif
}

void endfield ()
{
    int
	i;

    endword ();

    if (testRowlabels) {
	/* buffering until column labels are done and the existence of row labels is known */
	if (n_field == max_field) {
	    max_field += 256;
	    field = (FIELD_*) s_realloc (field, max_field * sizeof (FIELD_));
	}
	field [n_field].word = (char**) s_malloc (n_word * sizeof (char *));
	for (i = 0; i < n_word; i++)
	    field [n_field].word [i] = word [i];
	field [n_field].n_word = n_word;

    } else {
	/* no buffering */
	if (n_field == body_columns)
	    errit ("Too many fields in row %i", dataline + 1);
    }

    /* increase field counter */
    n_field++;

    /* reset word counter */
    n_word = 0;

#ifdef DEBUG
    printf ("   FIELD\n");
#endif
}

void endword ()
{
    int
	i;

    /* strip leading and trailing white space */
    while (buflen && isspace ((unsigned char) buffer [buflen - 1]))
	buflen--;
    buffer [buflen] = '\0';
    i = 0;
    while (buffer [i] && isspace ((unsigned char) buffer [i]))
	i++;

    /* reset word */
    buflen = 0;

    if (! buffer [i])
	return;

#ifdef DEBUG
    printf ("      ITEM\n");
#endif

    if (testRowlabels) {
	/* buffering until column labels are done and the existence of row labels is known */

	if (n_word == max_word) {
	    max_word += 16;
	    word = (char **) s_realloc (word, max_word * sizeof (char *));
	}
	word [n_word] = s_strdup (buffer + i);

    } else {
	/* no buffering */

	if (n_field == body_columns)
	    errit ("Too many fields in row %i", dataline + 1);

	if (n_field == 0 && useRowlabels) {
	    /* this is a row label */

	    /* a row label can have only one word */
	    if (n_word)
		errit ("Label error in row %i", dataline + 1);

	    /* save row label in case we need it (i.e. the row is not empty) */
	    strcpy (bufrowlab, buffer + i);

	} else {
	    /* this is a data field */

	    if (! fpout) {
		/* first non-empty data field in row -> open new file */
		if (useRowlabels) {
		    if (! bufrowlab [0])
			errit ("Missing label in row %i\n", dataline + 1);
		    openfile (bufrowlab, FALSE);
		} else {
		    sprintf (buffer2, "%03i", dataline + 1);
		    openfile (buffer2, FALSE);
		}
	    }

	    out (useRowlabels ? n_field - 1 : n_field, n_word, buffer + i);
	}

    }

    /* keep count of how many words there are in this data field */
    n_word++;

#ifdef DEBUG
    printf ("\t_%s_\n", buffer + i);
#endif

}

void chr (char c)
{
    if (buflen == BUFSIZE)
	errit ("Buffer overflow");

    buffer [buflen++] = c;
}

void chrs (char const *s)
{
    int
	i;

    i = strlen (s);
    if (buflen + i >= BUFSIZE)
	errit ("Buffer overflow");
    strcpy (buffer + buflen, s);
    buflen += i;
}

void parse_error (char const *s)
{
    int
	i,
	j;
    j = 0;
    buffer [0] = '\0';
    for (i = 0; yytext [i]; i++) {
	switch (yytext [i]) {
	    case '\t':
		strcat (buffer, "[TAB]");
		break;
	    case ' ':
		strcat (buffer, "[SAPCE]");
		break;
	    case '\r':
		strcat (buffer, "[CR]");
		break;
	    case '\n':
		strcat (buffer, "[NL]");
		break;
	    case '\f':
		strcat (buffer, "[FF]");
		break;
	    default:
		j = strlen (buffer);
	        buffer [j++] = yytext [i];
	        buffer [j] = '\0';
	}
    }
    errit ("Parse error in line %i, state <%s> : %s", lineno, s, buffer);
}

void process_args ()
{
    while (arg_c > 1 && arg_v [1][0] == '-') {
        switch (arg_v [1][1]) {
	    case 'c':
		if (useTab)
		    errit ("Can't use both -c and -t");
		setComma ();
		break;
            case 'd':
                dest = get_arg ();
                break;
	    case 's':
		fSafe = TRUE;
		break;
	    case 't':
		if (useComma)
		    errit ("Can't use both -c and -t");
		setTab ();
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
	"Split spreadsheet for processing by RuG/L04\n"
	"\n"
	"Version " my_VERSION "\n"
	"\n"
	"Usage: %s [-c|-t] [-d directory] [-s]\n"
	"\n"
	"\t-c : set input delimiter to comma (default: autodetect)\n"
	"\t-t : set input delimiter to tab (default: autodetect)\n"
	"\t-d : destination\n"
        "\t-s : safe output filenames\n"
	"\n",
	programname
    );
    exit (1);
}
