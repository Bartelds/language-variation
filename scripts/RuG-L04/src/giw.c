/*
 * file: giw.c
 *
 * (c) Peter Kleiweg 2000 - 2011
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2,
 * or (at your option) any later version.
 *
 */

#define __NO_MATH_INLINES
#include <math.h>

#define giwVERSION "0.27"

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
#  include <io.h>
#  define F_OK 0
#else
#  include <unistd.h>
#endif
#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define BUFSIZE 4095

#define VARSPACE 21

typedef enum { FALSE = 0, TRUE } BOOL_;

typedef struct item_struct {
    int
        *s,
	len,
	n,
	used_n;
    double
        f;
    struct item_struct
        *left,
	*right,
	*next;
} ITEM_;

ITEM_
    *root = NULL,
    *list = NULL,
    ***loclist,
    **row1 = NULL,
    **row2 = NULL;

typedef struct {
    char
        *s;
    int
        i;
} LABEL_;

LABEL_
    *labels = NULL;

typedef struct {
    double
        f,
        n;
} DIFF_;

DIFF_
    **diff;

int
    minf = 1,
    *loclist_n,
    *loclist_max,
    ilen,
    rowlen = 0,
    n_str [2] = { 0, 0 },
    max_str [2] = { 0, 0 },
    arg_c,
    n_locations = 0,
    max_code = INT_MAX,
    fileitems,
    filelocs,
    *fileItems,
    *fileLocs,
    used_n = 0,
    used_max = 0,
    *used = NULL,
    dynbuf_max,
    bootn,
    *booti;

float
    bootperc;

long
    lineno,
    itemsum = 0;

int
    ibuffer [BUFSIZE + 1],
    caWords = 0;

char
    buffer [BUFSIZE + 1],
    buf1 [2 * BUFSIZE + 3],
    buf2 [2 * BUFSIZE + 3],
    *dynbuf,
    **fileStrings,
    **arg_v,
    *label_file = NULL,
    *output_file = "giw.out",
    *programname,
    *no_mem_buffer,
    out_of_memory [] = "Out of memory";

char const
    *filename;

FILE
    *fp_in = NULL,
    *fp_out = NULL;

BOOL_
    test = FALSE,
    skip_missing = FALSE,
    min2 = FALSE,
    cAlpha = FALSE,
    cAlphaOld = FALSE,
    verbose = TRUE,
    bootstrap = FALSE,
    utf8;

float
    ***caDiff,
    caUndef = -0.99 * FLT_MAX;

void
    putstring (void),
    putlist (void),
    insertstring (int loc),
    process_file (char const *s),
    open_read (char const *s),
    read_labelfile (void),
    get_programname (char const *argv0),
    process_args (void),
    errit (char const *format, ...),
    warn (char const *format, ...),
    syntax (int err),
    *s_malloc (size_t size),
    *s_realloc (void *block, size_t size),
    checkCancel(void);
char
    *GetLine (BOOL_ required),
    *get_arg (void),
    *s_strdup (char const *s);
char const
    *quote (char const *s, char *buf),
    *unquote (char *s);

int
    cmpi (void const *, void const *),
    cmpint (void const *, void const *),
    cmps (void const *, void const *),
    cmpd (void const *, void const *),
    searchcmp (void const *key, void const *p),
    getlocation (void);

ITEM_
    *insertitem (void);

int main (int argc, char *argv [])
{
    time_t
        tp;
    int
	i,
	ii,
	j,
	n,
	skip,
	varN,
	covN,
        caN,
	caWordsUsed,
	wrd,
	wrd1,
	wrd2;
    float
	sum,
	ssum,
	xsum,
	ysum,
        av = 0.0,
        sd = 0.0,
	varAv,
	varSum,
	covAv,
	covSum,
	caValue = 0.0,
        caValueOld = 0.0,
        caRav,
        caRsum;
    BOOL_
	*caUse,
	caValueUsed = FALSE,
	caValueUsedOld = FALSE;

    no_mem_buffer = (char *) malloc (1024);

    get_programname (argv [0]);

    if (arg_c == 1)
	syntax (0);

    arg_c = argc;
    arg_v = argv;
    process_args ();

    if (cAlpha) {
	caDiff = (float ***) s_malloc (arg_c * sizeof (float **));
    }

    if (arg_c == 1)
	syntax (1);

    if (n_locations < 2)
	errit ("Illegal number of locations: %i", n_locations);

    if (label_file)
	read_labelfile ();

    if (access (output_file, F_OK) == 0)
	errit ("File exists: \"%s\"", output_file);

    if (! test) {
	fp_out = fopen (output_file, "w");
	if (! fp_out)
	    errit ("Creating file \"%s\": %s", output_file, strerror (errno));
    }

    diff = (DIFF_ **) s_malloc ((n_locations + 1) * sizeof (DIFF_ *));
    for (i = 2; i <= n_locations; i++) {
	diff [i] = (DIFF_ *) s_malloc (i * sizeof (DIFF_));
	for (j = 1; j < i; j++) {
	    diff [i][j].n = diff [i][j].f = 0.0;
	}
    }

    loclist = (ITEM_ ***) s_malloc ((n_locations + 1) * sizeof (ITEM_ **));
    loclist_n = (int *) s_malloc ((n_locations + 1) * sizeof (int));
    loclist_max = (int *) s_malloc ((n_locations + 1) * sizeof (int));
    for (i = 1; i <= n_locations; i++) {
        loclist_max [i] = 16;
	loclist [i] = (ITEM_ **) s_malloc (loclist_max [i] * sizeof (ITEM_ *));
    }

    used_max = 100;
    used = (int *) s_malloc (used_max * sizeof (int));

    dynbuf_max = BUFSIZE;
    dynbuf = (char *) s_malloc (dynbuf_max * sizeof (char));

    booti = (int *) s_malloc ((arg_c + 1) * sizeof (int));
    if (test || ! bootstrap) {
	for (i = 1; i < arg_c; i++)
	    booti [i] = 1;
    } else {
	for (i = 1; i < arg_c; i++)
	    booti [i] = 0;
	srand ((unsigned int) time (NULL));
	bootn = (int) (((float) (arg_c - 1)) / 100.0 * bootperc + 0.5);
	if (bootn < 1)
	    errit ("Percentage for bootstrapping too small");
	for (i = 0; i < bootn; i++) {
	    j = rand() % (arg_c - 1);
	    if (j >= arg_c - 1)
		j = arg_c - 2;
	    booti [j + 1]++;
	}
    }

    fileItems = (int *) s_malloc ((arg_c + 1) * sizeof (int));
    fileLocs = (int *) s_malloc ((arg_c + 1) * sizeof (int));
    fileStrings = (char **) s_malloc ((arg_c + 1) * sizeof (char *));
    for (i = 1; i < arg_c; i++)
	for (ii = 0; ii < booti [i]; ii++) {
	    checkCancel ();
	    fileitems = filelocs = used_n = 0;
	    process_file (arg_v [i]);
	    if (test)
		continue;
	    if (ii > 1)
		continue;
	    fileItems [i] = fileitems;
	    fileLocs [i] = filelocs;
	    sprintf (dynbuf, "#%*s%i:", VARSPACE, "", used_n);
	    skip = 0;
	    qsort (used, used_n, sizeof (int), cmpint);
	    for (j = 0; j < used_n; j++) {
		if (used [j] == 0)
		    continue;
		n = 1;
		while (j < used_n - 1 && used [j] == used [j + 1]) {
		    n++;
		    j++;
		}
		if (strlen (dynbuf) + 80 > dynbuf_max) {
		    dynbuf_max += 1024;
		    dynbuf = (char *) s_realloc (dynbuf, dynbuf_max * sizeof (char));
		}
		if (strlen (dynbuf + skip) > 70) {
		    sprintf (dynbuf + strlen (dynbuf), "\n");
		    skip = strlen (dynbuf);
		    sprintf (dynbuf + strlen (dynbuf), "#%*s ", VARSPACE, "");
		}
		if (n == 1)
		    sprintf (dynbuf + strlen (dynbuf), " %i", used [j]);
		else
		    sprintf (dynbuf + strlen (dynbuf), " %i(%i)", used [j], n);
	    }

	    fileStrings [i] = s_strdup (dynbuf);
	}

    if (test)
	return 0;

    if (! cAlpha)
	cAlphaOld = FALSE;
    else {
	if (verbose) {
	    fprintf (stderr, "\nCronbach Alpha... ");
	    fflush (stderr);
	}
	caUse = (BOOL_ *) s_malloc (caWords * sizeof (BOOL_));
	
	caWordsUsed = caWords;

	/* variances */

	varSum = 0.0;
	varN = 0;
	for (wrd = 0; wrd < caWords; wrd++) {
	    checkCancel ();
	    sum = ssum = 0.0;
	    n = 0;
	    for (i = 1; i < n_locations; i++)
		for (j = 0; j < i; j++)
		    if (caDiff [wrd][i][j] != caUndef) {
			sum += caDiff [wrd][i][j];
			ssum += caDiff [wrd][i][j] * caDiff [wrd][i][j];
			n++;
		    }

	    if (n < 2) {
		caUse [wrd] = FALSE;
		caWordsUsed--;
	    } else {
		caUse [wrd] = TRUE;
		varSum += (ssum - sum * sum / (float) n) / (n - 1);
		varN++;
	    }

	}


	/* covariances */

	covSum = 0.0;
	covN = 0;
	for (wrd1 = 1; wrd1 < caWords; wrd1++) {
	    checkCancel ();
	    if (! caUse [wrd1])
		continue;
	    for (wrd2 = 0; wrd2 < wrd1; wrd2++) {
		if (! caUse [wrd2])
		    continue;
		n = 0;
		xsum = ysum = ssum = 0.0;
		for (i = 1; i < n_locations; i++)
		    for (j = 0; j < i; j++)
			if (caDiff [wrd1][i][j] != caUndef && caDiff [wrd2][i][j] != caUndef) {
			    xsum += caDiff [wrd1][i][j];
			    ysum += caDiff [wrd2][i][j];
			    ssum += caDiff [wrd1][i][j] * caDiff [wrd2][i][j];
			    n++;
			}
		if (n > 1) {
		    covSum += (ssum - xsum * ysum / ((float) n)) / (float) (n - 1);
		    covN++;
		}
	    }
	}

	if (varN < 1 || covN < 1) {
	    caValueUsed = FALSE;
	    if (verbose)
		fprintf (stderr, "Undefined\n");
	} else {
	    caValueUsed = TRUE;
	    varAv = varSum / (float) varN;
	    covAv = covSum / (float) covN;
	    caValue = caWordsUsed * covAv / (varAv + (caWordsUsed - 1) * covAv);
	    if (verbose)
		fprintf (stderr, "%g\n", caValue);
	}

	/*
	 *   old method: this was wrong
	 */ 

	if (! caValueUsed)
	    cAlphaOld = FALSE;

	if (cAlphaOld) {
	    if (verbose) {
		fprintf (stderr, "\nCronbach Alpha... ");
		fflush (stderr);
	    }

	    caWordsUsed = caWords;

	    /* normalisation */
	    for (wrd = 0; wrd < caWords; wrd++) {
		checkCancel ();
		sum = ssum = 0.0;
		n = 0;
		for (i = 1; i < n_locations; i++)
		    for (j = 0; j < i; j++)
			if (caDiff [wrd][i][j] != caUndef) {
			    sum += caDiff [wrd][i][j];
			    ssum += caDiff [wrd][i][j] * caDiff [wrd][i][j];
			    n++;
			}

		if (n < 2)
		    caUse [wrd] = FALSE;
		else {
		    caUse [wrd] = TRUE;
		    av = sum / (float) n;
		    sd = sqrt ((ssum - sum * sum / (float) n) / (n - 1));
		    if (sd == 0.0)
			caUse [wrd] = FALSE;
		}

		if (! caUse [wrd])
		    caWordsUsed--;
		else
		    for (i = 1; i < n_locations; i++)
			for (j = 0; j < i; j++)
			    if (caDiff [wrd][i][j] != caUndef)
				caDiff [wrd][i][j] = (caDiff [wrd][i][j] - av) / sd;
	    }
	    caRsum = 0.0;
	    caN = 0;
	    for (wrd1 = 1; wrd1 < caWords; wrd1++) {
		checkCancel ();
		if (! caUse [wrd1])
		    continue;
		for (wrd2 = 0; wrd2 < wrd1; wrd2++) {
		    if (! caUse [wrd2])
			continue;
		    n = 0;
		    sum = 0;
		    for (i = 1; i < n_locations; i++)
			for (j = 0; j < i; j++)
			    if (caDiff [wrd1][i][j] != caUndef && caDiff [wrd2][i][j] != caUndef) {
				sum += caDiff [wrd1][i][j] * caDiff [wrd2][i][j];
				n++;
			    }
		    if (n > 1) {
			caRsum += sum / (float) (n - 1);
			caN++;
		    }
		}
	    }

	    if (caN == 0) {
		caValueUsedOld = FALSE;
		if (verbose)
		    fprintf (stderr, "Undefined  (old, imprecise method)\n");
	    } else {
		caValueUsedOld = TRUE;
		caRav = caRsum / (float) caN;
		caValueOld = caWordsUsed * caRav / (1 + (caWordsUsed - 1) * caRav);
		if (verbose)
		    fprintf (stderr, "%g  (old, imprecise method)\n", caValueOld);
	    }
		
	}

    }


    time (&tp);
    fprintf (
        fp_out,
	"# Created by: %s, Version " giwVERSION "\n",
	programname
    );

    if (bootstrap)
	fprintf (fp_out, "# Using bootstrapping: %g%% (%i draws)\n", bootperc, bootn);

    if (minf > 1)
	fprintf (fp_out, "# Minimum number of occurrences per variant: %i\n", minf);
    if (min2)
	fprintf (fp_out, "# Items with less than two variants removed\n");
    fprintf (fp_out, "# Items used: %li\n", itemsum);

    if (cAlpha) {
	fprintf (fp_out, "# Cronbach Alpha: ");
	if (caValueUsed)
	    fprintf (fp_out, "%g\n", caValue);
	else
	    fprintf (fp_out, "Undefined\n");
    }
    if (cAlphaOld) {
	fprintf (fp_out, "# Cronbach Alpha: ");
	if (caValueUsedOld)
	    fprintf (fp_out, "%g", caValueOld);
	else
	    fprintf (fp_out, "Undefined");
	fprintf (fp_out, "  (old, imprecise method)\n");
    }

    fprintf (
        fp_out,
	"# Date: %s\n",
	asctime (localtime (&tp))
    );

    if (bootstrap)
	fprintf (fp_out, "#   Items  Locations  [draws] File\n");
    else
	fprintf (fp_out, "#   Items  Locations  File\n");
    fprintf (fp_out, "#                     Variants: frequency(repeats)\n");
    for (i = 1; i < arg_c; i++) {
	if (bootstrap) {
	    if (booti [i])
		fprintf (fp_out, "#%8i %8i    [%i] %s\n", fileItems [i], fileLocs [i], booti [i], arg_v [i]);
	    else
		fprintf (fp_out, "#                     [%i] %s\n", booti [i], arg_v [i]);
	} else
	    fprintf (fp_out, "#%8i %8i    %s\n", fileItems [i], fileLocs [i], arg_v [i]);
	if (fileItems [i])
	    fprintf (fp_out, "%s\n", fileStrings [i]);
    }
    fprintf (fp_out, "\n");

    if (label_file)
	qsort (labels, n_locations, sizeof (LABEL_), cmpi);

    fprintf (fp_out, "%i\n", n_locations);
    if (label_file)
	for (i = 0; i < n_locations; i++)
	    fprintf (fp_out, "%s\n", labels [i].s);
    else
	for (i = 1; i <= n_locations; i++)
	    fprintf (fp_out, "%i\n", i);

    for (i = 2; i <= n_locations; i++)
	for (j = 1; j < i; j++)
	    if (diff [i][j].n > 0.0)
		fprintf (fp_out, "%g\n", diff [i][j].f / diff [i][j].n);
	    else {
		fprintf (fp_out, "NA\n");
		if (verbose)
		    fprintf (stderr, "No value for: %i \\ %i\n", j, i);
	    }

    fclose (fp_out);
    if (verbose) {
	fprintf (stderr, "\a\nResults written to %s\n", output_file);
	fprintf (stderr, "Items used: %li\n\n", itemsum);
    }

    return 0;
}

void checkCancel ()
{
    if (test)
	return;
    if (access ("_CANCEL_.L04", F_OK) == 0) {
	if (fp_out)
	    fclose (fp_out);
	unlink (output_file);
	errit ("CANCELLED");
    }
}

/*
 * kgv = kleinst gemeemschappelijk veelvoud
 */
int kgv (int l1, int l2)
{
    int
	ll1,
	ll2;

    ll1 = l1;
    ll2 = l2;
    for (;;) {
        if (ll1 == ll2)
	    return ll1;
	while (ll1 < ll2)
	    ll1 += l1;
	while (ll2 < ll1)
	    ll2 += l2;
    }
}

void process_file (char const *s)
{
    int
        i,
	i1,
	i2,
	j,
	l,
	l1,
	l2,
	n,
	loc,
	count;

    double
	sum,
	weight;
    ITEM_
        *tmp;
    BOOL_
	doIt;

    if (verbose) {
	fprintf (stderr, "%s\n", s);
	fflush (stderr);
    }

    loc = 0;

    count = 0;

    for (i = 1; i <= n_locations; i++)
	loclist_n [i] = 0;

    root = list = NULL;

    weight = 1.0;

    utf8 = FALSE;
    open_read (s);
    while (GetLine (FALSE)) {
        if (buffer [0] == '*') {
	    weight = atof (buffer + 1);
	    if (weight <= 0)
		errit ("Invalid weight, %s, line %li", filename, lineno);
	} else if (buffer [0] == '%') {
	    if (memcmp (buffer, "%utf8", 5) && memcmp (buffer, "%UTF8", 5))
		errit ("Syntax error, in file %s, line %li", s, lineno);
	    utf8 = TRUE;
	} else if (buffer [0] == ':') {
	    if (! label_file)
		errit ("No labels defined, %s, line %li", filename, lineno);
	    loc = getlocation ();
	} else if (isdigit ((unsigned char) buffer [0])) {
	    loc = atoi (buffer);
	    if (loc < 1 || loc > n_locations)
		errit ("Location out of range, in file %s, line %li", s, lineno);
	} else if (buffer [0] == '-') {
	    if (loc == 0)
		errit ("No location before string, in file %s, line %li", s, lineno);
	    if (loc != -1) {
		putstring ();
		insertstring (loc);
	    }
	} else if (buffer [0] == '+') {
	    if (loc == 0)
		errit ("No location before string, in file %s, line %li", s, lineno);
	    if (loc != -1) {
		putlist ();
		insertstring (loc);
	    }
	} else
	    errit ("Syntax error, in file %s, line %li", s, lineno);
    }
    fclose (fp_in);

    if (! test) {

	if (minf > 1)
	    /* skip infrequent variants */
	    for (i = 1; i <= n_locations; i++)
		for (j = 0; j < loclist_n [i]; j++)
		    if (loclist [i][j]->n < minf)
			loclist [i][j--] = loclist [i][--loclist_n [i]];

	doIt = TRUE;
	if (min2) {
	    i = 0;
	    for (tmp = list; tmp; tmp = tmp->next)
		if (tmp->n >= minf)
		    i++;
	    if (i < 2)
		doIt = FALSE;
	}

	if (doIt) {

	    if (cAlpha) {
		caDiff [caWords] = (float **) s_malloc (n_locations * sizeof (float *));
		for (i = 1; i < n_locations; i++) {
		    caDiff [caWords][i] = (float *) s_malloc (i * sizeof (float));
		    for (j = 0; j < i; j++)
			caDiff [caWords][i][j] = caUndef;
		}
	    }

	    for (i = 1; i <= n_locations; i++) {
		count += loclist_n [i];
		itemsum += loclist_n [i];
		if (loclist_n [i])
		    filelocs++;
	    }
	    fileitems = count;

	    for (tmp = list; tmp; tmp = tmp->next)
		tmp->f = ((double) tmp->n) / (double) count;

	    for (i = 1; i <= n_locations; i++) {
		l1 = loclist_n [i];
		for (j = 0; j < l1; j++)
		    loclist [i][j]->used_n++;
	    }

	    for (i = 2; i <= n_locations; i++) {
		checkCancel ();
		if ((l1 = loclist_n [i]) == 0)
		    continue;
		for (j = 1; j < i; j++) {
		    if ((l2 = loclist_n [j]) == 0)
			continue;
		    l = kgv (l1, l2);
		    if (l > rowlen) {
			rowlen = l;
			row1 = (ITEM_ **) s_realloc (row1, rowlen * sizeof (ITEM_ *));
			row2 = (ITEM_ **) s_realloc (row2, rowlen * sizeof (ITEM_ *));
		    }
		    for (n = 0; n < l; n++) {
			row1 [n] = loclist [i][n % loclist_n [i]];
			row2 [n] = loclist [j][n % loclist_n [j]];
		    }
		    for (i1 = 0; i1 < l; i1++)
			if (row1 [i1] != row2 [i1])
			    for (i2 = l - 1; i2 >= 0; i2--)
				if (i1 != i2 && row1 [i1] == row2 [i2] && row1 [i2] != row2 [i2]) {
				    tmp = row2 [i1];
				    row2 [i1] = row2 [i2];
				    row2 [i2] = tmp;
				    break;
				}
		    sum = 0.0;
		    for (i1 = 0; i1 < l; i1++)
			sum += (row1 [i1] == row2 [i1]) ? row1 [i1]->f : 1.0;
		    sum /= (double) l;
		    diff [i][j].f += sum * weight;
		    diff [i][j].n += weight;
		    if (cAlpha)
			caDiff [caWords][i - 1][j - 1] = sum * weight;
		}
	    }
	    if (cAlpha)
		caWords++;
	}
    }

    used_n = 0;
    while (list) {
	if (used_n == used_max) {
	    used_max += 100;
	    used = (int *) s_realloc (used, used_max * sizeof (int));
	}
	used [used_n++] = list->used_n;
        tmp = list;
	list = list->next;
        free (tmp->s);
	free (tmp);
    }

}

void insertstring (int loc)
{
    if (loclist_n [loc] == loclist_max [loc]) {
	loclist_max [loc] += 16;
	loclist [loc] = (ITEM_ **) s_realloc (loclist [loc], loclist_max [loc] * sizeof (ITEM_ *));
    }
    loclist [loc][loclist_n [loc]++] = insertitem ();
}

int itemcmp (ITEM_ *item)
{
    int
	i,
        l,
	n;

    l = (item->len < ilen) ? item->len : ilen;
    for (i = 0; i < l; i++) {
	n = item->s [i] - ibuffer [i];
        if (n)
            return n;
    }
    return item->len - ilen;
}

ITEM_ *newitem ()
{
    ITEM_
        *item;
    int
        i;

    item = (ITEM_ *) s_malloc (sizeof (ITEM_));
    item->s = (int *) s_malloc (ilen * sizeof (int));
    for (i = 0; i < ilen; i++)
        item ->s [i] = ibuffer [i];
    item->len = ilen;
    item->n = 1;
    item->used_n = 0;
    item->left = item->right = NULL;
    item->next = list;
    list = item;
    return item;
}

ITEM_ *insertitem ()
{
    ITEM_
        *tmp;
    int
	i;

    if (root) {
        tmp = root;
	for (;;) {
	    i = itemcmp (tmp);
	    if (i == 0) {
		tmp->n++;
		return tmp;
	    }
	    if (i < 0) {
		if (tmp->left)
		    tmp = tmp->left;
		else {
		    tmp->left = newitem ();
		    return tmp->left;
		}
	    } else {
		if (tmp->right)
		    tmp = tmp->right;
		else {
		    tmp->right = newitem ();
		    return tmp->right;
		}
	    }

	}
    } else {
	root = newitem ();
	return root;
    }
}

unsigned long int min3 (unsigned long int i1, unsigned long int i2, unsigned long int i3)
{
    if (i1 < i2) {
        if (i1 < i3)
            return i1;
        else
            return i3;
    } else {
        if (i2 < i3)
            return i2;
        else
            return i3;
    }
}

void bytescheck (unsigned char *s, int n)
{
    int
	i;
    for (i = 0; i < n; i++)
	if (! s [i])
	    errit ("Invalid utf-8 code in %s, line %li", filename, lineno);
}

void limitcheck (long unsigned c)
{
    if (c > (long unsigned) max_code)
	errit ("Code out of range in %s, line %li", filename, lineno);

}

int bytes2 (unsigned char *s)
{

    long unsigned
	c;

    bytescheck (s, 2);

    c =   ( s [1] & 0x3F)
        | ((s [0] & 0x1F) << 6);

    limitcheck (c);

    return (int) c;

}

int bytes3 (unsigned char *s)
{

    long unsigned
	c;

    bytescheck (s, 3);

    c =   ( s [2] & 0x3F)
        | ((s [1] & 0x3F) <<  6)
        | ((s [0] & 0x0F) << 12);

    limitcheck (c);

    return (int) c;

}

int bytes4 (unsigned char *s)
{

    long unsigned
	c;

    bytescheck (s, 4);

    c =   ( s [3] & 0x3F)
        | ((s [2] & 0x3F) <<  6)
        | ((s [1] & 0x3F) << 12)
        | ((s [0] & 0x07) << 18);

    limitcheck (c);

    return (int) c;

}

int bytes5 (unsigned char *s)
{

    long unsigned
	c;

    bytescheck (s, 5);

    c =   ( s [4] & 0x3F)
        | ((s [3] & 0x3F) <<  6)
        | ((s [2] & 0x3F) << 12)
        | ((s [1] & 0x3F) << 18)
        | ((s [0] & 0x03) << 24);

    limitcheck (c);

    return (int) c;

}

int bytes6 (unsigned char *s)
{

    long unsigned
	c;

    bytescheck (s, 6);

    c =   ( s [5] & 0x3F)
        | ((s [4] & 0x3F) <<  6)
        | ((s [3] & 0x3F) << 12)
        | ((s [2] & 0x3F) << 18)
        | ((s [1] & 0x3F) << 24)
        | ((s [0] & 0x01) << 30);

    limitcheck (c);

    return (int) c;

}

void putstring ()
{
    int
	offset,
	j;

    unsigned char
	*buf;


    offset = 1;
    while (buffer [offset] && isspace ((unsigned char) buffer [offset]))
	offset++;

    if (utf8) {

	ilen = 0;
	for (buf = (unsigned char *) (buffer + offset); buf [0]; buf++) {
	    if        ((int) (buf [0]) >= 0374U) {
		ibuffer [ilen] = bytes6 (buf);
		buf += 5;
	    } else if ((int) (buf [0]) >= 0370U) {
		ibuffer [ilen] = bytes5 (buf);
		buf += 4;
	    } else if ((int) (buf [0]) >= 0360U) {
		ibuffer [ilen] = bytes4 (buf);
		buf += 3;
	    } else if ((int) (buf [0]) >= 0340U) {
		ibuffer [ilen] = bytes3 (buf);
		buf += 2;
	    } else if ((int) (buf [0]) >= 0300U) {
		ibuffer [ilen] = bytes2 (buf);
		buf += 1;
	    } else if ((int) (buf [0]) >= 0200U) {
		errit ("Invalid utf-8 code in %s, line %li", filename, lineno);
	    } else {
		limitcheck (buf [0]);
		ibuffer [ilen] = buf [0];
	    }
	    ilen++;
	}
	if (ilen == 0)
	    errit ("Missing string in %s, line %lu", filename, lineno);

    } else {

	ilen = strlen (buffer + offset);
	if (ilen == 0)
	    errit ("Missing string in %s, line %lu", filename, lineno);

	for (j = offset; buffer [j]; j++)
	    if ((unsigned char) buffer [j] > max_code)
		errit ("Code out of range in %s, line %li", filename, lineno);

	for (j = 0; j < ilen; j++)
	    ibuffer [j] = (unsigned char) buffer [offset + j];

    }

}

void putlist ()
{
    int
	i,
	n,
	val;

    ilen = 0;
    i = 1;
    while (sscanf (buffer + i, "%i%n", &val, &n) >= 1) {
	i += n;
	if (val < 1 || val > max_code)
	    errit ("Code out of range in %s, line %li", filename, lineno);
	ibuffer [ilen++] = val;
    }
    if (ilen == 0)
	errit ("Missing string in %s, line %lu", filename, lineno);
}

int getlocation ()
{
    char const
	*s;
    LABEL_
	*p;
    int
	i;

    i = 1;
    while (buffer [i] && isspace ((unsigned char) buffer [i]))
	i++;

    s = unquote (buffer + i);

    p = (LABEL_ *) bsearch (s, labels, n_locations, sizeof (LABEL_), searchcmp);

    if (! p) {
	if (skip_missing)
	    return -1;
	else
	    errit ("Undefined label \"%s\" in %s, line %li", s, filename, lineno);
    }

    return p->i;
}

int searchcmp (void const *key, void const *p)
{
    return strcmp ((char *) key, ((LABEL_ *)p)->s);
}

void open_read (char const *s)
{
    filename = s;
    lineno = 0;
    fp_in = fopen (s, "r");
    if (! fp_in)
	errit ("Opening file \"%s\": %s", s, strerror (errno));
}

void read_labelfile ()
{
    int
	i,
	n;

    open_read (label_file);
    labels = (LABEL_ *) s_malloc (n_locations * sizeof (LABEL_));
    for (i = 0; i < n_locations; i++) {
	labels [i].s = NULL;
	labels [i].i = i + 1;
    }
    while (GetLine (FALSE)) {
	if (sscanf (buffer, "%i%n", &i, &n) < 1)
	    errit ("Missing number in \"%s\", line %lu", filename, lineno);
	if (i < 1 || i > n_locations)
	    errit ("Number out of range in \"%s\", line %lu", filename, lineno);
	if (labels [i - 1].s)
	    errit ("Duplicate label for nr %i in \"%s\", line %lu", i, filename, lineno);
	while (buffer [n] && isspace ((unsigned char) buffer [n]))
	    n++;
	if (! buffer [n])
	    errit ("Missing label in \"%s\", line %lu", filename, lineno);
	labels [i - 1].s = s_strdup (unquote (buffer + n));
    }
    fclose (fp_in);
    for (i = 0; i < n_locations; i++)
	if (! labels [i].s)
	    errit ("Missing number and label for location nr %i in file %s", i + 1, filename);
    qsort (labels, n_locations, sizeof (LABEL_), cmps);
    for (i = 1; i < n_locations; i++)
	if (! strcmp (labels [i - 1].s, labels [i].s))
	    errit ("Duplicate label \"%s\" in file %s", labels [i].s, filename);
}

/*
 * get line from `fp_in' into `buffer'
 * remove leading and trailing white space
 * skip empty lines and lines starting with #
 */
char *GetLine (BOOL_ required)
{
    int
	i;

    for (;;) {
	lineno++;
	if (fgets (buffer, BUFSIZE, fp_in) == NULL) {
	    if (required)
		errit ("Reading file \"%s\", line %li: %s",
		       filename, lineno, errno ? strerror (errno) : "End of file");
	    else
		return NULL;
	}
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
	return buffer;
    }
}

int cmpi (void const *p1, void const *p2)
{
    return ((LABEL_ *)p1)->i - ((LABEL_ *)p2)->i;
}

int cmpd (void const *p1, void const *p2)
{
    double
	f;

    f = *((double *)p1) - *((double *)p2);
    if (f < 0.0)
	return -1;
    else if (f > 0.0)
	return 1;
    else
	return 0;
}

int cmpint (void const *p1, void const *p2)
{
    int
	i;

    i = *((int *)p1) - *((int *)p2);
    if (i < 0)
	return -1;
    else if (i > 0)
	return 1;
    else
	return 0;
}

int cmps (void const *p1, void const *p2)
{
    return strcmp (((LABEL_ *)p1)->s, ((LABEL_ *)p2)->s);
}

/*
 * add quotes if necessary
 */
char const *quote (char const *s, char *qbuffer)
{
    int
        i,
        j;

    for (;;) {
        if (s [0] == '"')
            break;
        for (i = 0; s [i]; i++)
            if (isspace ((unsigned char) s [i]))
                break;
        if (s [i])
            break;

        return s;
    }

    j = 0;
    qbuffer [j++] = '"';
    for (i = 0; s [i]; i++) {
        if (s [i] == '"' || s [i] == '\\')
            qbuffer [j++] = '\\';
        qbuffer [j++] = s [i];
    }
    qbuffer [j++] = '"';
    qbuffer [j] = '\0';
    return qbuffer;
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

void process_args ()
{
    while (arg_c > 1 && arg_v [1][0] == '-') {
        switch (arg_v [1][1]) {
	    case 'b':
		bootstrap = TRUE;
		bootperc = atof (get_arg ());
		break;
	    case 'c':
		cAlpha = TRUE;
		break;
	    case 'f':
		minf = atoi (get_arg ());
		if (minf < 2)
		    errit ("Illegal value for option -f, must be larger than 1");
		break;
	    case 'F':
		min2 = TRUE;
		break;
	    case 'l':
		label_file = get_arg ();
		break;
	    case 'L':
		skip_missing = TRUE;
		break;
            case 'n':
		n_locations = atoi (get_arg ());
		break;
	    case 'o':
		output_file = get_arg ();
		break;
	    case 'q':
		verbose = FALSE;
		break;
	    case 't':
		test = TRUE;
		break;
	    case 'x':
		cAlphaOld = TRUE;
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

void syntax (int err)
{
    fprintf (
	err ? stderr : stdout,
	"\n"
	"G.I.W., Version " giwVERSION "\n"
	"(c) P. Kleiweg 2000 - 2011\n"
	"\n"
        "Usage: %s -n number\n"
        "\t[-b percentage] [-c [-x]] [-f number] [-F] [-l filename] [-L] [-o filename] [-q] [-t] datafile(s)\n"
	"\n"
	"\t-b : do bootstrapping with given percentage (usually: 100)\n"
	"\t-c : Cronbach Alpha\n"
	"\t-f : minimum number of occurrences required for each variant\n"
	"\t-F : skip files with less than two variants, depends on option -f\n"
	"\t-l : file with location labels\n"
	"\t-L : skip locations that are not listed in the file with location labels\n"
	"\t-n : number of locations\n"
	"\t-o : output file\n"
	"\t-q : quiet\n"
	"\t-t : test all input files\n"
	"\t-x : also give Cronbach's Alpha using old (incorrect) formula\n"
	"\n",
	programname
    );
    exit (err);
}
