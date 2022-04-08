/*
 * file: leven.c
 *
 * (c) Peter Kleiweg 2000 - 2020
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2,
 * or (at your option) any later version.
 *
 */

/*
 * The following function is called when the Levenshtein distance
 * between two strings has been calculated. It is used to normalise
 * that distance.
 * The function is called with five arguments: the Levenshtein
 * distance, the lengths of both strings, and the minimum and maximum
 * length of the least-cost alignment(s).
 * A common normalisation is to devide by the sum of the lengths of
 * the strings.
 * You can select the type of normalisation on the command line.
 * You can add another type of normalisation within the code below.
 */
#define __NO_MATH_INLINES
#include <math.h>
#include <stdarg.h>
void errit (char const *format, ...);
int normalise_function = 1;  /* default is the first type of normalisation */
double normalise (double f, int len1, int len2, int minlen, int maxlen)
{
    double
	result = 0;

    switch (normalise_function) {
	case 1:
	    result = f / (len1 + len2);
	    break;
	case 2:
	    result = f / ((len1 > len2) ? len1 : len2);
	    break;
	case 3:
	    result = sqrt (f / (len1 + len2));
	    break;
        case 4:
	    result = f / minlen;
	    break;
        case 5:
	    result = f / maxlen;
	    break;
        case 6:
	    result = f;
	    break;
	default:
	    errit ("Normalisation function number %i undefined", normalise_function);
    }

    return result;
}

#define levenVERSION "1.82"

#if defined(LEVEN_REAL)
typedef float DIFFTYPE;
typedef double WEIGHTTYPE;
#elif defined(LEVEN_SMALL)
typedef unsigned char DIFFTYPE;
typedef unsigned long int WEIGHTTYPE;
#else
typedef unsigned short int DIFFTYPE;
typedef unsigned long int WEIGHTTYPE;
#endif

#ifndef LEVEN_REAL
#define DIFFMAX ((1 << (8 * sizeof (DIFFTYPE))) - 1)
#endif

#ifdef __WIN32__
#  define my_PATH_SEP '\\'
#else
#  define my_PATH_SEP '/'
#endif

#ifdef __WIN32__
#define log2(x) (log(x) / log(2))
#define round(x) (floor((x) + 0.5))
#else
double log2(double);
double round(double);
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
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <limits.h>
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
	index,
	n,
	used_n;
    struct item_struct
        *left,
	*right,
	*next;
} ITEM_;

typedef struct {
    int
        *s,
	len,
	max_len;
} PAIRITEM_;

typedef struct {
    WEIGHTTYPE
        i;
    int
        min,
	max,
        n;
    BOOL_
        above,
	left,
	aboveleft;
} TBL_;

ITEM_
    *root = NULL,
    *list = NULL,
    ***loclist;

PAIRITEM_
    *pairitem = NULL;

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
	s,
        n;
} DIFF_;

typedef struct {
    double
        *f;
    int
        n;
} DIFFM_;

BOOL_
    **part,
    indelsubstequal = FALSE,
    min2 = FALSE,
    binair = FALSE,
    hamming = FALSE,
    numeric = FALSE,
#ifndef LEVEN_REAL
    cGIW = FALSE,
#endif
    cAlpha = FALSE,
    cAlphaOld = FALSE,
    cAlphaMem = TRUE,
    verbose = TRUE,
    veryverbose = TRUE,
    pairwise = FALSE,
    *pairtest = NULL,
    bootstrap = FALSE,
    updatesubst = FALSE,
    utf8;

DIFF_
    **diff;

DIFFM_
    **diffm;

float
    ***caDiff,
    caUndef = -0.99 * FLT_MAX;

double
    scale = 1.0,
    **idiff = NULL,
    *token1,
    **token2,
    errsave,
    incnull = 0.001,
    alpha = 0.3,
    pmiExp = 1.0,
    minkowski = 2.0;

int
#ifndef LEVEN_REAL
    maxGIW = 0,
#endif
    minf = 1,
    *loclist_n,
    *loclist_max,
    ilen,
    rowlen = 0,
    *row1 = NULL,
    *row2 = NULL,
    maxindex,
    idiff_size = 0,
    tsize,
    n_str [2] = { 0, 0 },
    max_str [2] = { 0, 0 },
    arg_c,
    n_locations = 0,
    max_code = INT_MAX,
    caWords = 0,
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

unsigned long int
#ifndef LEVEN_REAL
    *nGIW = NULL,
    sumGIW,
#endif
    itemsum = 0;

TBL_
    **tbl;

long
    lineno;

DIFFTYPE
    *id,
    **w,
    **ws;

int
    ibuffer [BUFSIZE + 1];

char
    buffer [BUFSIZE + 1],
    buffer2 [BUFSIZE + 1],
    buf1 [2 * BUFSIZE + 3],
    buf2 [2 * BUFSIZE + 3],
    **pairword,
    *dynbuf,
    **fileStrings,
    **arg_v,
    *subst_file = NULL,
    *indel_file = NULL,
    *label_file = NULL,
    *part_file = NULL,
    *output_file = "leven.out",
    *updated_subst_file = NULL,
    *sd_file = NULL,
    *programname,
    *no_mem_buffer,
    out_of_memory [] = "Out of memory";

char const
    *filename;

FILE
    *fp_in = NULL,
    *fp_out = NULL,
    *fp_sdout = NULL;

BOOL_
    use_median = FALSE,
    use_sd = FALSE,
    median_warn = TRUE,
    test = FALSE,
    skip_missing = FALSE;

void
    putstring (int offset),
    putlist (int offset),
    insertstring (int loc),
    process_file (char const *s),
    open_read (char const *s),
    read_substfile (void),
    read_indelfile (void),
    read_labelfile (void),
    read_partfile (void),
    get_programname (char const *argv0),
    process_args (void),
    warn (char const *format, ...),
    syntax (int err),
    *s_malloc (size_t size),
    *s_realloc (void *block, size_t size),
    checkCancel(void),
    do_pairwise(void),
    update_subst (void),
    resubst_levenshtein (int const *s1, int len1, int n1, int const *s2, int len2, int n2),
    retrack (int const *s1, int y, int const *s2, int x, double n);
char
    *GetLine (BOOL_ required),
    *get_arg (void),
    *s_strdup (char const *s);
char const
    *quote (char const *s, char *buf),
    *unquote (char *s);

WEIGHTTYPE
    (*weight)(int, int),
    weight_plain (int, int),
    weight_indel (int, int),
    weight_subst (int, int),
#ifndef LEVEN_REAL
    weight_cGIW (int, int),
#endif
    min3 (WEIGHTTYPE i1, WEIGHTTYPE i2, WEIGHTTYPE i3);

int
    cmpi (void const *, void const *),
    cmpint (void const *, void const *),
    cmps (void const *, void const *),
    cmpd (void const *, void const *),
    searchcmp (void const *key, void const *p),
    getlocation (void);

double
    levenshtein (int const *s1, int len1, int const *s2, int len2),
    hammingDist (int const *s1, int len1, int const *s2, int len2),
    numericDist (int const *s1, int len1, int const *s2, int len2),
    get_diff (int i, int j);

ITEM_
    *insertitem (void);

BOOL_
    use_it (int i, int j);

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

    arg_c = argc;
    arg_v = argv;
    process_args ();

    if (arg_c == 1)
	syntax (0);

    if (access (output_file, F_OK) == 0)
	errit ("File exists: \"%s\"", output_file);

    if (subst_file)
	read_substfile ();
    else if (indel_file)
	read_indelfile ();
#ifndef LEVEN_REAL
    else if (cGIW) {
	maxGIW = 65536;
	nGIW = (long unsigned int *) s_malloc (maxGIW * sizeof (long unsigned int));
	scale = DIFFMAX / 2;
	weight = weight_cGIW;
    }
#endif
    else
	weight = weight_plain;

    if (label_file)
	read_labelfile ();

    /* test for valid normalise function */
    normalise (1, 1, 1, 1, 1);

    if (pairwise) {
	do_pairwise ();
	return 0;
    }

    if (use_median && cAlpha)
	errit ("You can't combine options -c and -m");

    if (binair) {
	if (subst_file)
	    errit ("You can't use substitution values with binary comparison");
	if (indel_file)
	    errit ("You can't use indel values with binary comparison");
#ifndef LEVEN_REAL
	if (cGIW)
	    errit ("You can't use character-based G.I.W. with binary comparison");
#endif
	if (indelsubstequal)
	    errit ("You can't set indel equal to subst binary comparison");
    }

    if (hamming) {
	if (subst_file)
	    errit ("You can't use substitution values with Hamming distance");
	if (indel_file)
	    errit ("You can't use indel values with Hamming distance");
#ifndef LEVEN_REAL
	if (cGIW)
	    errit ("You can't use character-based G.I.W. with Hamming distance");
#endif
	if (indelsubstequal)
	    errit ("You can't set indel equal to subst Hamming distance");
    }

    if (numeric) {
	if (subst_file)
	    errit ("You can't use substitution values with numeric distance");
	if (indel_file)
	    errit ("You can't use indel values with numeric distance");
#ifndef LEVEN_REAL
	if (cGIW)
	    errit ("You can't use character-based G.I.W. with numeric distance");
#endif
	if (indelsubstequal)
	    errit ("You can't set indel equal to subst numeric distance");
    }

    if ((binair ? 1 : 0) + (hamming ? 1 : 0) + (numeric ? 1 : 0) > 1)
	errit ("You can use only one of -B -h -r");

    if (n_locations < 2)
	errit ("Illegal number of locations: %i", n_locations);

    if (use_sd && use_median)
	errit ("You can't get both medians and standard deviation\n");

    if (subst_file && indel_file)
	errit ("Use either substitution values or indel values, not both");

#ifndef LEVEN_REAL
    if (cGIW && (subst_file || indel_file))
	errit ("You can't use character-based G.I.W. with predefined indel or substitution values");
#endif

    if (indelsubstequal && (subst_file || indel_file))
	errit ("You can't set indel equal to substitution with predefined indel or substitution values");

    if (part_file)
	read_partfile ();

    if (use_sd && access (sd_file, F_OK) == 0)
	errit ("File exists: \"%s\"", sd_file);

    if (! test) {
	fp_out = fopen (output_file, "w");
	if (! fp_out)
	    errit ("Creating file \"%s\": %s", output_file, strerror (errno));
    }

    if (use_sd && ! test) {
	fp_sdout = fopen (sd_file, "w");
	if (! fp_sdout)
	    errit ("Creating file \"%s\": %s", sd_file, strerror (errno));
    }

    if (use_median) {
	diffm = (DIFFM_ **) s_malloc ((n_locations + 1) * sizeof (DIFFM_ *));
	for (i = 2; i <= n_locations; i++) {
	    diffm [i] = (DIFFM_ *) s_malloc (i * sizeof (DIFFM_));
	    for (j = 1; j < i; j++) {
		diffm [i][j].n = 0;
		diffm [i][j].f = (double *) s_malloc (arg_c * sizeof (double));
	    }
	}
    } else {
	diff = (DIFF_ **) s_malloc ((n_locations + 1) * sizeof (DIFF_ *));
	for (i = 2; i <= n_locations; i++) {
	    diff [i] = (DIFF_ *) s_malloc (i * sizeof (DIFF_));
	    for (j = 1; j < i; j++)
		diff [i][j].n = diff [i][j].f = diff [i][j].s = 0.0;
	}
    }

    loclist = (ITEM_ ***) s_malloc ((n_locations + 1) * sizeof (ITEM_ **));
    loclist_n = (int *) s_malloc ((n_locations + 1) * sizeof (int));
    loclist_max = (int *) s_malloc ((n_locations + 1) * sizeof (int));
    for (i = 1; i <= n_locations; i++) {
        loclist_max [i] = 16;
	loclist [i] = (ITEM_ **) s_malloc (loclist_max [i] * sizeof (ITEM_ *));
    }

    tsize = 16;
    tbl = (TBL_ **) s_malloc (tsize * sizeof (TBL_ *));
    for (i = 0; i < tsize; i++)
	tbl [i] = (TBL_ *) s_malloc (tsize * sizeof (TBL_));

    if (cAlpha) {
	caDiff = (float ***) malloc (arg_c * sizeof (float **));
	if (caDiff == NULL)
	    cAlphaMem = FALSE;
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


    if (updatesubst && ! test)
	update_subst ();


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
	if (! cAlphaMem) {
	    if (verbose)
		fprintf (stderr, "\nCronbach Alpha... out of memory\n");
	} else {
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

    }

    time (&tp);
    fprintf (
        fp_out,
	"# Created by: %s, Version " levenVERSION "\n",
	programname
    );

    if (bootstrap)
	fprintf (fp_out, "# Using bootstrapping: %g%% (%i draws)\n", bootperc, bootn);

    if (use_median)
	fprintf (fp_out, "# Output of median values\n");

    if (binair)
	fprintf (fp_out, "# Binary comparison\n");
    else if (hamming)
	fprintf (fp_out, "# Hamming distance\n");
    else if (numeric)
	fprintf (fp_out, "# Numeric distance, Minkowski value: %g\n", minkowski);
    if (! binair)
	fprintf (fp_out, "# Normalisation function number: %i\n", normalise_function);

    if (indel_file)
	fprintf (fp_out, "# Indel file: %s\n", indel_file);
    else if (subst_file) {
	fprintf (fp_out, "# Substitution file: %s\n", subst_file);
	if (updated_subst_file) {
	    fprintf (fp_out, "# Updated substitution file: %s\n", updated_subst_file);
	    fprintf (fp_out, "#     remaining sum of square errors: %g\n", errsave);
	    fprintf (fp_out, "#     alpha: %g\n", alpha);
	    fprintf (fp_out, "#     increment of unused alignments: %g\n", incnull);
	    fprintf (fp_out, "#     exponent in PMI formula: %g\n", pmiExp);
	}
    } else if (indelsubstequal)
	fprintf (fp_out, "# Weight of indel equals weight of substitution\n");

    if (minf > 1)
	fprintf (fp_out, "# Minimum number of occurrences per variant: %i\n", minf);

    if (min2)
	fprintf (fp_out, "# Items with less than two variants removed\n");

    fprintf (fp_out, "# Items used: %li\n", itemsum);

    if (cAlpha) {
	fprintf (fp_out, "# Cronbach Alpha: ");
	if (cAlphaMem) {
	    if (caValueUsed)
		fprintf (fp_out, "%g\n", caValue);
	    else
		fprintf (fp_out, "Undefined\n");
	} else
	    fprintf (fp_out, "Out of memory\n");
    }
    if (cAlphaOld) {
	fprintf (fp_out, "# Cronbach Alpha: ");
	if (cAlphaMem) {
	    if (caValueUsedOld)
		fprintf (fp_out, "%g", caValueOld);
	    else
		fprintf (fp_out, "Undefined");
	} else
	    fprintf (fp_out, "Out of memory");
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

    if (use_sd) {
	fprintf (
	    fp_sdout,
	    "# Standard deviation values\n"
	    "# Created by: %s, Version " levenVERSION "\n",
	    programname
	);

	if (bootstrap)
	    fprintf (fp_sdout, "# Using bootstrapping: %g%% (%i draws)\n", bootperc, bootn);

	if (binair)
	    fprintf (fp_sdout, "# Binary comparison\n");
	else if (hamming)
	    fprintf (fp_sdout, "# Hamming distance\n");
	else if (numeric)
	    fprintf (fp_sdout, "# Numeric distance, Minkowski value: %g\n", minkowski);
	if (! binair)
	    fprintf (fp_sdout, "# Normalisation function number: %i\n", normalise_function);

	if (indel_file)
	    fprintf (fp_sdout, "# Indel file: %s\n", indel_file);
	else if (subst_file) {
	    fprintf (fp_sdout, "# Substitution file: %s\n", subst_file);
	    if (updated_subst_file) {
		fprintf (fp_sdout, "# Updated substitution file: %s\n", updated_subst_file);
		fprintf (fp_sdout, "#     remaining sum of square errors: %g\n", errsave);
		fprintf (fp_sdout, "#     alpha: %g\n", alpha);
		fprintf (fp_sdout, "#     increment of unused alignments: %g\n", incnull);
		fprintf (fp_sdout, "#     exponent in PMI formula: %g\n", pmiExp);
	    }
	}
	else if (indelsubstequal)
	    fprintf (fp_sdout, "# Weight of indel equals weight of substitution\n");

	if (minf > 1)
	    fprintf (fp_sdout, "# Minimum number of occurrences per variant: %i\n", minf);

	if (min2)
	    fprintf (fp_sdout, "# Items with less than two variants removed\n");

	fprintf (fp_sdout, "# Items used: %li\n", itemsum);

	if (cAlpha) {
	    fprintf (fp_sdout, "# Cronbach Alpha: ");
	    if (cAlphaMem) {
		if (caValueUsed)
		    fprintf (fp_sdout, "%g\n", caValue);
		else
		    fprintf (fp_sdout, "Undefined\n");
	    } else 
		fprintf (fp_sdout, "Out of memory\n");
	}
	if (cAlphaOld) {
	    fprintf (fp_sdout, "# Cronbach Alpha: ");
	    if (cAlphaMem) {
		if (caValueUsedOld)
		    fprintf (fp_sdout, "%g", caValueOld);
		else
		    fprintf (fp_sdout, "Undefined");
	    } else 
		fprintf (fp_sdout, "Out of memory");
	    fprintf (fp_sdout, "  (old, imprecise method)\n");
	}

	fprintf (
	    fp_sdout,
	    "# Date: %s\n",
	    asctime (localtime (&tp))
	);

	if (bootstrap)
	    fprintf (fp_sdout, "#   Items  Locations  [draws] File\n");
	else
	    fprintf (fp_sdout, "#   Items  Locations  File\n");
	fprintf (fp_sdout, "#                     Variants: frequency(repeats)\n");
	for (i = 1; i < arg_c; i++) {
	    if (bootstrap) {
	    if (booti [i])
		fprintf (fp_sdout, "#%8i %8i    [%i] %s\n", fileItems [i], fileLocs [i], booti [i], arg_v [i]);
	    else
		fprintf (fp_sdout, "#                     [%i] %s\n", booti [i], arg_v [i]);
	    } else
		fprintf (fp_sdout, "#%8i %8i    %s\n", fileItems [i], fileLocs [i], arg_v [i]);
	    if (fileItems [i])
		fprintf (fp_sdout, "%s\n", fileStrings [i]);
	}
	fprintf (fp_sdout, "\n");
    }

    if (label_file)
	qsort (labels, n_locations, sizeof (LABEL_), cmpi);

    if (part_file == NULL) {
	fprintf (fp_out, "%i\n", n_locations);
	if (use_sd)
	    fprintf (fp_sdout, "%i\n", n_locations);
	if (label_file) {
	    for (i = 0; i < n_locations; i++) {
		fprintf (fp_out, "%s\n", labels [i].s);
		if (use_sd)
		    fprintf (fp_sdout, "%s\n", labels [i].s);
	    }
	} else
	    for (i = 1; i <= n_locations; i++) {
		fprintf (fp_out, "%i\n", i);
		if (use_sd)
		    fprintf (fp_sdout, "%i\n", i);
	    }
    }

    for (i = 2; i <= n_locations; i++)
	for (j = 1; j < i; j++)
	    if (use_it (i, j)) {
		if ((use_median ? diffm [i][j].n : diff [i][j].n) > 0.0)
		    fprintf (fp_out, "%g", get_diff (i, j));
		else {
		    fprintf (fp_out, "NA");
		    if (verbose)
			fprintf (stderr, "No value for: %i \\ %i\n", j, i);
		}
		if (use_sd) {
		    if (diff [i][j].n < 2) {
			fprintf (fp_sdout, "NA");
			if (verbose)
			    fprintf (stderr, "No standard deviation value for: %i \\ %i\n", j, i);
		    } else
			fprintf (
			    fp_sdout,
			    "%g",
			    sqrt((diff [i][j].s - diff [i][j].f * diff [i][j].f / diff [i][j].n) / (diff [i][j].n - 1))
			);
		}
		if (part_file) {
		    fprintf (fp_out, "\t%i\t%i", i, j);
		    if (use_sd)
			fprintf (fp_sdout, "\t%i\t%i", i, j);
		    if (label_file) {
			fprintf (fp_out, "\t%s\t%s", quote (labels [i - 1].s, buf1), quote (labels [j - 1].s, buf2));
			if (use_sd)
			    fprintf (fp_sdout, "\t%s\t%s", quote (labels [i - 1].s, buf1), quote (labels [j - 1].s, buf2));
		    }
		}
		fprintf (fp_out, "\n");
		if (use_sd)
		    fprintf (fp_sdout, "\n");
	    }

    fclose (fp_out);
    if (verbose)
	fprintf (stderr, "\a\nResults written to %s\n", output_file);

    if (use_sd) {
	fclose (fp_sdout);
	if (verbose)
	    fprintf (stderr, "Standard deviations written to %s\n", sd_file);
    }

    if (verbose)
	fprintf (stderr, "Items used: %li\n\n", itemsum);

    return 0;
}

void resetpairs ()
{
    int
	i;

    for (i = 0; i < n_locations; i++)
	pairitem [i].len = 0;
}

void pairs_insertstring (int p)
{
    int
	i;

    if (pairitem [p].len)
	errit ("Only one item per location / word allowed");
    if (ilen > pairitem [p].max_len) {
	pairitem [p].max_len = ilen + 32;
	pairitem [p].s = (int *) s_realloc (pairitem [p].s , pairitem [p].max_len * sizeof (int));
    }
    for (i = 0; i < ilen; i++)
	pairitem [p].s [i] = ibuffer [i];
    pairitem [p].len = ilen;
}

void pairwise_out (FILE *fp_out, BOOL_ number, BOOL_ string, int loc)
{
    int
	i,
	j;
    double
	f;

    checkCancel ();

    if (number)
	fprintf (fp_out, "%i\n", loc);
    else
	fprintf (fp_out, ": %s\n", buffer2);

    for (i = 0; i < n_locations; i++)
	for (j = i + 1; j < n_locations; j++)
	    if (pairtest [i] || pairtest [j]) {
		if (pairitem [i].len && pairitem [j].len) {
		    if (binair) {
			f = 0.0;
			if (pairitem [i].len != pairitem [j].len)
			    f = 1.0;
			else if (memcmp (pairitem [i].s, pairitem [j].s, pairitem [i].len * sizeof (int)))
			    f = 1.0;
		    } else
			f = levenshtein (pairitem [i].s, pairitem [i].len, pairitem [j].s, pairitem [j].len) / scale;
		    fprintf (fp_out, "%i %i %g\n", i + 1, j + 1, f);
		} else {
		    fprintf (fp_out, "%i %i NA\n", i + 1, j + 1);
		}
	    }
    fprintf (fp_out, "\n");
}

void do_pairwise ()
{
    char
	c;
    int
	i,
	d,
	n,
	loc;
    BOOL_
	number,
	string,
	rowstart;
    FILE
	*fp_out;
    time_t
        tp;

    if (arg_c != 2)
	errit ("You need exactly one datafile for pairwise");

    if (n_locations < 2)
	errit ("Invalid number of locations");

    open_read (arg_v [1]);
    fp_out = fopen (output_file, "w");

    pairitem = (PAIRITEM_ *) s_malloc (n_locations * sizeof (PAIRITEM_));
    for (i = 0; i < n_locations; i++) {
	pairitem [i].len = 0;
	pairitem [i].max_len = 32;
	pairitem [i].s = (int *) s_malloc (pairitem [i].max_len * sizeof (int));
    }
    if (pairtest == NULL) {
	pairtest = (BOOL_ *) s_malloc (n_locations * sizeof (BOOL_));
	for (i = 0; i < n_locations; i++)
	    pairtest [i] = TRUE;
    }

    time (&tp);
    fprintf (
        fp_out,
	"# Created by: %s, Version " levenVERSION "\n",
	programname
    );

    c = ':';
    fprintf (fp_out, "# Output of pairwise measurements\n# Target columns");
    for (i = 0; i < n_locations; i++)
	if (pairtest [i]) {
	    fprintf (fp_out, "%c %i", c, i + 1);
	    c = ',';
	}
    fprintf (fp_out, "\n");

    if (binair)
	fprintf (fp_out, "# Binary comparison\n");
    else
	fprintf (fp_out, "# Normalisation function number: %i\n", normalise_function);

    if (indel_file)
	fprintf (fp_out, "# Indel file: %s\n", indel_file);
    else if (subst_file)
	fprintf (fp_out, "# Substitution file: %s\n", subst_file);
    else if (indelsubstequal)
	fprintf (fp_out, "# Weight of indel equals weight of substitution\n");

    fprintf (
        fp_out,
	"# Date: %s\n",
	asctime (localtime (&tp))
    );

    utf8 = FALSE;
    rowstart = number = string = FALSE;
    loc = 0;
    while (GetLine (FALSE)) {
	if (buffer [0] == '%') {
	    if (memcmp (buffer, "%utf8", 5) && memcmp (buffer, "%UTF8", 5))
		errit ("Syntax error, in file %s, line %li", filename, lineno);
	    utf8 = TRUE;
	} else if (buffer [0] == ':') {
	    if (rowstart) {
		pairwise_out (fp_out, number, string, loc);
		resetpairs ();
	    }

	    i = 1;
	    while (buffer [i] && isspace ((unsigned char) buffer [i]))
		i++;
	    if (! buffer [i])
		errit ("Missing label in file \"%s\", line %li", filename, lineno);
	    strcpy (buffer2, buffer + i);

	    number = FALSE;
	    string = TRUE;
	    rowstart = TRUE;
	} else if (isdigit ((unsigned char) buffer [0])) {
	    if (rowstart) {
		pairwise_out (fp_out, number, string, loc);
		resetpairs ();
	    }

	    loc = atoi (buffer);

	    number = TRUE;
	    string = FALSE;
	    rowstart = TRUE;
	} else if (sscanf (buffer, "[ %d ] %c %n", &d, &c, &n) >= 2) {
	    if (! (number || string))
		errit ("No location before string, in file %s, line %li", filename, lineno);
	    if (d < 1 || d > n_locations)
		errit ("Index %i out of range, in file %s, line %li", d, filename, lineno);
	    if (c == '-')
		putstring (n);
	    else if (c == '+')
		putlist (n);
	    else
		errit ("Syntax error, in file %s, line %li", filename, lineno);
	    pairs_insertstring (d - 1);
	} else
	    errit ("Syntax error, in file %s, line %li", filename, lineno);
    }
    if (rowstart)
	pairwise_out (fp_out, number, string, loc);
    fclose (fp_out);
    fclose (fp_in);

    if (verbose)
	fprintf (stderr, "Results written to %s\n", output_file);

}

void checkCancel ()
{
    if (test)
	return;
    if (access ("_CANCEL_.L04", F_OK) == 0) {
	if (fp_out)
	    fclose (fp_out);
	if (fp_sdout)
	    fclose (fp_sdout);
	if (use_sd)
	    unlink (sd_file);
	unlink (output_file);
	errit ("CANCELLED");
    }
}

double get_diff (int i, int j)
{
    int
	n;
    double
	*f;

    if (use_median) {
	f = diffm [i][j].f;
	n = diffm [i][j].n;
	qsort (f, n, sizeof (double), cmpd);
	if (n % 2)
	    return f [(n - 1) / 2];
	else
	    return (f [n / 2 - 1] + f [n / 2]) / 2.0;
    } else
	return diff [i][j].f / diff [i][j].n;
}

BOOL_ use_it (int i, int j)
{
    return (part_file == NULL) ? TRUE : part [i - 1][j - 1];
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
        newsize;
    double
	weight,
	sum;
    ITEM_
        *tmp,
	*tmp2;
    BOOL_
	found,
	doIt;


    if (veryverbose) {
	fprintf (stderr, " %-78s\r", s);
	fflush (stderr);
    }

    loc = 0;

    maxindex = 0;

    for (i = 1; i <= n_locations; i++)
	loclist_n [i] = 0;

    root = list = NULL;

    weight = 1.0;

#ifndef LEVEN_REAL
    if (cGIW) {
	for (i = 0; i < maxGIW; i++)
	    nGIW [i] = 0;
	sumGIW = 0;
    }
#endif

    utf8 = FALSE;
    open_read (s);
    while (GetLine (FALSE)) {
	if (buffer [0] == '*') {
	    if ((use_median || use_sd) && median_warn) {
		if (verbose)
		    warn ("Ignoring weights");
		median_warn = FALSE;
	    }
	    if (! use_sd) {
		weight = atof (buffer + 1);
		if (weight <= 0)
		    errit ("Invalid weight, %s, line %li", filename, lineno);
	    }
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
		putstring (1);
		insertstring (loc);
	    }
	} else if (buffer [0] == '+') {
	    if (loc == 0)
		errit ("No location before string, in file %s, line %li", s, lineno);
	    if (loc != -1) {
		putlist (1);
		insertstring (loc);
	    }
	} else
	    errit ("Syntax error, in file %s, line %li", s, lineno);
    }
    fclose (fp_in);

#ifndef LEVEN_REAL
    /*
    if (cGIW) {
	sumGIW = 0;
	for (i = 0; i < maxGIW; i++)
	    if (nGIW [i] > sumGIW)
		sumGIW = nGIW [i];
    }
    */
#endif

    if (maxindex > idiff_size) {
	newsize = maxindex * 1.2;
        idiff = (double **) s_realloc (idiff, newsize * sizeof (double *));
	for (i = 0; i < idiff_size; i++)
	    idiff [i] = (double *) s_realloc (idiff [i], newsize * sizeof (double));
	for (i = idiff_size; i < newsize; i++) {
	    idiff [i] = (double *) s_malloc (newsize * sizeof (double));
	    idiff [i][i] = 0.0;
	}
	idiff_size = newsize;
	if (binair)
	    for (i = 1; i < idiff_size; i++)
		for (j = 0; j < i; j++)
		    idiff [i][j] = idiff [j][i] = 1.0;
    }

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

	    if (cAlpha && cAlphaMem) {
		caDiff [caWords] = (float **) malloc (n_locations * sizeof (float *));
		if (caDiff [caWords] == NULL)
		    cAlphaMem = FALSE;
		else {
		    for (i = 1; i < n_locations; i++)
			caDiff [caWords][i] = NULL;
		    for (i = 1; i < n_locations; i++) {
			caDiff [caWords][i] = (float *) malloc (i * sizeof (float));
			if (caDiff [caWords][i] == NULL) {
			    cAlphaMem = FALSE;
			    break;
			} else {
			    for (j = 0; j < i; j++)
				caDiff [caWords][i][j] = caUndef;
			}
		    }		    
		}
		if (! cAlphaMem) {
		    for (i = caWords; i >= 0; i--)
			if (caDiff [i]) {
			    for (j = n_locations - 1; j >= 1; j--)
				if (caDiff [i][j])
				    free (caDiff [i][j]);
			    free (caDiff [i]);
			}
		    if (verbose)
			fprintf (stderr, "\nNo memory for Cronbach Alpha... discontinuing\n");
		}
		    
	    }

	    for (i = 1; i <= n_locations; i++) {
		itemsum += loclist_n [i];
		fileitems += loclist_n [i];
		if (loclist_n [i])
		    filelocs++;
	    }

	    if (! binair)
		for (tmp = list; tmp; tmp = tmp->next) {
		    checkCancel ();
		    if (tmp->n < minf)
			continue;
		    i = tmp->index;
		    for (tmp2 = list; tmp2 != tmp; tmp2 = tmp2->next) {
			if (tmp2->n < minf)
			    continue;
			j = tmp2->index;
			idiff [i][j] = idiff [j][i] = levenshtein (tmp->s, tmp->len, tmp2->s, tmp2->len) / scale;
		    }
		}

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
		    if (! use_it (i, j))
			continue;
		    l = kgv (l1, l2);
		    if (l > rowlen) {
			rowlen = l;
			row1 = (int *) s_realloc (row1, rowlen * sizeof (int));
			row2 = (int *) s_realloc (row2, rowlen * sizeof (int));
		    }
		    for (n = 0; n < l; n++) {
			row1 [n] = loclist [i][n % loclist_n [i]]->index;
			row2 [n] = loclist [j][n % loclist_n [j]]->index;
		    }
		    for (found = TRUE; found; ) {
			found = FALSE;
			for (i1 = 0; i1 < l; i1++)
			    for (i2 = i1 + 1; i2 < l; i2++)
				if (idiff [row1 [i1]][row2 [i1]] + idiff [row1 [i2]][row2 [i2]] >
				    idiff [row1 [i1]][row2 [i2]] + idiff [row1 [i2]][row2 [i1]]) {
				    n = row2 [i1];
				    row2 [i1] = row2 [i2];
				    row2 [i2] = n;
				    found = TRUE;
				}
		    }
		    sum = 0.0;
		    for (i1 = 0; i1 < l; i1++)
			sum += idiff [row1 [i1]][row2 [i1]];
		    sum /= l;
		    if (use_median)
			diffm [i][j].f [diffm [i][j].n++] = sum;
		    else {
			diff [i][j].f += sum * weight;
			diff [i][j].s += sum * sum;
			diff [i][j].n += weight;
		    }
		    if (cAlpha && cAlphaMem)
			caDiff [caWords][i - 1][j - 1] = sum * weight;
		}
	    }
	    if (cAlpha && cAlphaMem)
		caWords++;
	}
    }

    if (veryverbose) {
	fprintf (stderr, " %-78s\r", "");
	fflush (stderr);
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
    item->index = maxindex++;
    item->n = 1;
    item->used_n = 0;
    item->left = item->right = NULL;
    item->next = list;
    list = item;
    return item;
}

#ifndef LEVEN_REAL
void count_GIW ()
{
    int
	i;

    for (i = 0; i < ilen; i++) {
	if (ibuffer [i] >= maxGIW)
	    errit ("cGIW index out of range");
	nGIW [ibuffer [i]]++;
    }
    sumGIW += ilen;
}
#endif

ITEM_ *insertitem ()
{
    ITEM_
        *tmp;
    int
	i;

#ifndef LEVEN_REAL
    if (cGIW)
	count_GIW ();
#endif

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

double hammingDist (int const *s1, int l1, int const *s2, int l2)
{
    int
	i;
    double
	sum = 0.0;

    if (l1 != l2)
	errit ("Strings not of same length: %i, %i", l1, l2);
    for (i = 0; i < l1; i++)
	if (s1 [i] != s2 [i])
	    sum += 1.0;
    return normalise ((double) sum, l1, l1, l1, l1);
}

double numericDist (int const *s1, int l1, int const *s2, int l2)
{
    int
	i;
    double
	sum = 0.0;
	
    if (l1 != l2)
	errit ("Strings not of same length: %i, %i", l1, l2);
    for (i = 0; i < l1; i++)
	sum += pow (fabs (((double) (s1 [i])) - (double) (s2 [i])), minkowski);
    sum = pow (sum, 1.0 / minkowski);
    return normalise ((double) sum, l1, l1, l1, l1);
}

double levenshtein (int const *s1, int l1, int const *s2, int l2)
{
    int
	i,
	x,
	y,
	newsize,
	minlink,
	maxlink;
    WEIGHTTYPE
	aboveleft,
	above,
	left;

#ifndef LEVEN_REAL
    WEIGHTTYPE
	sum0;
#endif

    if (hamming)
	return hammingDist (s1, l1, s2, l2);

    if (numeric)
	return numericDist (s1, l1, s2, l2);

    if (l1 >= tsize || l2 >= tsize) {
	newsize = tsize + 16;
	while (l1 >= newsize || l2 >= newsize)
	    newsize += 16;
	for (i = 0; i < tsize; i++)
	    tbl [i] = (TBL_ *) s_realloc (tbl [i], newsize * sizeof (TBL_));
	tbl = (TBL_ **) s_realloc (tbl, newsize * sizeof (TBL_ *));
	while (tsize < newsize)
	    tbl [tsize++] = (TBL_ *) s_malloc (newsize * sizeof (TBL_));
    }
    tbl [0][0].i = 0;
    tbl [0][0].min = tbl [0][0].max = 0;
    for (x = 1; x <= l2; x++) {
        tbl [x][0].i = tbl [x - 1][0].i + weight (0, s2 [x - 1]);
	tbl [x][0].min = tbl [x][0].max = x;
    }
    for (y = 1; y <= l1; y++) {
        tbl [0][y].i = tbl [0][y - 1].i + weight (s1 [y - 1], 0);
        tbl [0][y].min = tbl [0][y].max = y;
    }
    for (x = 1; x <= l2; x++)
        for (y = 1; y <= l1; y++) {
            aboveleft = tbl [x - 1][y - 1].i + weight (s1 [y - 1], s2 [x - 1]);
            above     = tbl [x    ][y - 1].i + weight (s1 [y - 1], 0         );
            left      = tbl [x - 1][y    ].i + weight (0,          s2 [x - 1]);
            tbl [x][y].i = min3 (aboveleft, above, left);
	    minlink = l1 + l2;
	    maxlink = 0;
	    if (aboveleft <= above && aboveleft <= left) {
		if (minlink > tbl [x - 1][y - 1].min)
		    minlink = tbl [x - 1][y - 1].min;
		if (maxlink < tbl [x - 1][y - 1].max)
		    maxlink = tbl [x - 1][y - 1].max;
	    }
	    if (above <= aboveleft && above <= left) {
		if (minlink > tbl [x][y - 1].min)
		    minlink = tbl [x][y - 1].min;
		if (maxlink < tbl [x][y - 1].max)
		    maxlink = tbl [x][y - 1].max;
	    }
	    if (left <= aboveleft && left <= above) {
		if (minlink > tbl [x - 1][y].min)
		    minlink = tbl [x - 1][y].min;
		if (maxlink < tbl [x - 1][y].max)
		    maxlink = tbl [x - 1][y].max;
	    }
	    tbl [x][y].min = minlink + 1;
	    tbl [x][y].max = maxlink + 1;
        }

#ifndef LEVEN_REAL
    if (cGIW) {
	sum0 = 0;
	for (i = 0; i < l1; i++)
	    sum0 += weight (s1 [i], s1 [i]);
	for (i = 0; i < l2; i++)
	    sum0 += weight (s2 [i], s2 [i]);
	tbl [l2][l1].i -= sum0 / 2;
    }
#endif

    return normalise ((double) tbl [l2][l1].i, l1, l2, tbl [l2][l1].min, tbl [l2][l1].max);
}

WEIGHTTYPE min3 (WEIGHTTYPE i1, WEIGHTTYPE i2, WEIGHTTYPE i3)
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

void putstring (int offset)
{
    int
	j;

    unsigned char
	*buf;


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

void putlist (int offset)
{
    int
	n,
	val;

    ilen = 0;
    while (sscanf (buffer + offset, "%i%n", &val, &n) >= 1) {
	offset += n;
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

WEIGHTTYPE weight_plain (int i, int j)
{
    if (i == j)
	return 0;
    if ((i && j) || indelsubstequal)
	return 2;
    return 1;
}

WEIGHTTYPE weight_indel (int i, int j)
{
    if (i == j)
	return 0;
    return id [i] + id [j];
}

WEIGHTTYPE weight_subst (int i, int j)
{
    if (i == j)
	return 0;
    if (i > j)
	return w [i][j];
    return w [j][i];
}

#ifndef LEVEN_REAL
WEIGHTTYPE weight_cGIW (int i, int j)
{
    if (i == j)
	return (nGIW [i] * DIFFMAX) / sumGIW;
    if ((i && j) || indelsubstequal)
	return DIFFMAX;
    return DIFFMAX / 2;
}
#endif

void open_read (char const *s)
{
    filename = s;
    lineno = 0;
    fp_in = fopen (s, "r");
    if (! fp_in)
	errit ("Opening file \"%s\": %s", s, strerror (errno));
}

void read_substfile ()
{
#ifdef LEVEN_REAL
    float
	c;
#else
    long int
	c;
#endif
    int
	i,
	j;

    weight = weight_subst;

    open_read (subst_file);
    GetLine (TRUE);
#ifdef LEVEN_REAL
    if (buffer [0] != 'F' || buffer [1] != ':') {
	errit ("File \"%s\" is not a table with real values. You should use 'leven' or 'leven-s'", filename);
    }
    GetLine (TRUE);
#else
    if (buffer [0] == 'F' && buffer [1] == ':') {
	errit ("File \"%s\" is not a table with integer values. You should use 'leven-r'", filename);
    }
#endif
    max_code = atoi (buffer);
    if (max_code < 1)
	errit ("Illegal table size in \"%s\", line %li: %i", filename, lineno, max_code);
    w = (DIFFTYPE **) s_malloc ((max_code + 1) * sizeof (DIFFTYPE *));
    for (i = 1; i <= max_code; i++) {
	w [i] = (DIFFTYPE *) s_malloc (i * sizeof (DIFFTYPE));
	for (j = 0; j < i; j++) {
	    GetLine (TRUE);
#ifdef LEVEN_REAL
	    if (sscanf (buffer, "%f", &c) != 1)
#else
	    if (sscanf (buffer, "%li", &c) != 1)
#endif
		errit ("Reading value in \"%s\", line %li", filename, lineno);
#ifdef LEVEN_REAL
	    if (c < 0.0)
#else
	    if (c < 0 || c > DIFFMAX)
#endif
		errit ("Value out of range in \"%s\", line %li", filename, lineno);
	    w [i][j] = (DIFFTYPE) c;
	    if (j > 0)
		c /= 2;
	    if (c > scale)
		scale = c;
	}
    }
    fclose (fp_in);
}

void read_indelfile ()
{
#ifdef LEVEN_REAL
    float
	c;
#else
    long int
	c;
#endif
    int
	i;

    weight = weight_indel;

    open_read (indel_file);
    GetLine (TRUE);
#ifdef LEVEN_REAL
    if (buffer [0] != 'F' || buffer [1] != ':') {
	errit ("File \"%s\" is not a table with real values. You should use 'leven' or 'leven-s'", filename);
    }
    GetLine (TRUE);
#else
    if (buffer [0] == 'F' && buffer [1] == ':') {
	errit ("File \"%s\" is not a table with integer values. You should use 'leven-r'", filename);
    }
#endif
    max_code = atoi (buffer);
    if (max_code < 1)
	errit ("Illegal table size in \"%s\", line %li: %i", filename, lineno, max_code);
    id = (DIFFTYPE *) s_malloc ((max_code + 1) * sizeof (DIFFTYPE));
    id [0] = 0;
    for (i = 1; i <= max_code; i++) {
	GetLine (TRUE);
#ifdef LEVEN_REAL
	if (sscanf (buffer, "%f", &c) != 1)
#else
	if (sscanf (buffer, "%li", &c) != 1)
#endif
	    errit ("Reading value in \"%s\", line %li", filename, lineno);
#ifdef LEVEN_REAL
	if (c < 0.0)
#else
	if (c < 0 || c > DIFFMAX)
#endif
	    errit ("Value out of range in \"%s\", line %li", filename, lineno);
	id [i] = c;
	if (c > scale)
	    scale = c;
    }
    fclose (fp_in);
}

void read_partfile ()
{
    int
	i,
	j;

    part = (BOOL_ **) s_malloc (n_locations * sizeof (BOOL_ *));
    for (i = 0; i < n_locations; i++) {
	part [i] = (BOOL_ *) s_malloc (n_locations * sizeof (BOOL_));
	for (j = 0; j < n_locations; j++)
	    part [i][j] = FALSE;
    }

    open_read (part_file);
    while (GetLine (FALSE))
	if (buffer [0] == ':') {
	    if (sscanf (buffer + 1, "%i%i", &i, &j) != 2)
		errit ("Syntax error in \"%s\", line %lu", filename, lineno);
	    if (i < 1 || i > n_locations || j < 1 || j > n_locations)
		errit ("Index out of range in \"%s\", line %lu", filename, lineno);
	    i--;
	    j--;
	    part [i][j] = part [j][i] = TRUE;
	}
    fclose (fp_in);
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

void saveweights ()
{
    int
	i,
	j;

    for (i = 0; i <= max_code; i++)
	for (j = 0; j < i; j++)
	    ws [i][j] = w [i][j];
}

void restoreweights ()
{
    int
	i,
	j;
    for (i = 0; i <= max_code; i++)
	for (j = 0; j < i; j++)
	    w [i][j] = ws [i][j];
}

void update_subst ()
{
    int
	i,
	j,
	file_id,
	runs;
    double
	token1count,
	token2count,
	w_min,
	w_max,
#ifdef LEVEN_REAL
	w_max1,
#endif
	min2,
	max2,
	val,
	f,
	ssum,
	errmin;
    DIFFTYPE
	val2;
    ITEM_
        *tmp,
	*tmp2;
    BOOL_
	found,
	firstrun;
    FILE
	*fp;

    errmin = FLT_MAX;

    w_min = 1;


    if (! subst_file)
	errit ("Missing substitution file (option -s) with option -S");

    if (access (updated_subst_file, F_OK) == 0)
	errit ("File exists: \"%s\"", updated_subst_file);

    ws = (DIFFTYPE **) s_malloc ((max_code + 1) * sizeof (DIFFTYPE *));
    for (i = 1; i <= max_code; i++)
	ws [i] = (DIFFTYPE *) s_malloc (i * sizeof (DIFFTYPE));

    token1 = (double *) s_malloc ((max_code + 1) * sizeof (double));
    token2 = (double **) s_malloc ((max_code + 1) * sizeof (double *));
    for (i = 0; i <= max_code; i++)
	token2 [i] = (double *) s_malloc ((i + 1) * sizeof (double));

    firstrun = TRUE;
    for (runs = 8; runs; ) {
	for (i = 0; i <= max_code; i++) {
	    token1 [i] = 0.0;
	    for (j = 0; j <= i; j++)
		token2 [i][j] = 0.0;
	}

	for (file_id = 1; file_id < arg_c; file_id++) {
	    if (veryverbose) {
		fprintf (stderr, " %-78s\r", arg_v [file_id]);
		fflush (stderr);
	    }

	    root = list = NULL;

	    utf8 = FALSE;
	    open_read (arg_v [file_id]);
	    while (GetLine (FALSE)) {
		if (buffer [0] == '*')
		    ;
		else if (buffer [0] == '%') {
		    if (memcmp (buffer, "%utf8", 5) && memcmp (buffer, "%UTF8", 5))
			errit ("Syntax error, in file %s, line %li", arg_v [file_id], lineno);
		    utf8 = TRUE;
		} else if (buffer [0] == ':')
		    ;
		else if (isdigit ((unsigned char) buffer [0]))
		    ;
		else if (buffer [0] == '-') {
		    putstring (1);
		    insertitem ();
		} else if (buffer [0] == '+') {
		    putlist (1);
		    insertitem ();
		} else
		    errit ("Syntax error, in file %s, line %li", arg_v [file_id], lineno);
	    }
	    fclose (fp_in);

	    for (tmp = list; tmp; tmp = tmp->next) {
		checkCancel ();
		i = tmp->index;
		for (tmp2 = list; tmp2 != tmp; tmp2 = tmp2->next) {
		    j = tmp2->index;
		    resubst_levenshtein (tmp->s, tmp->len, tmp->n, tmp2->s, tmp2->len, tmp2->n);
		}
	    }

	    while (list) {
		tmp = list;
		list = list->next;
		free (tmp->s);
		free (tmp);
	    }

	}

	if (veryverbose) {
	    fprintf (stderr, " %-78s\r", "");
	    fflush (stderr);
	}

	for (i = 0; i <= max_code; i++)
	    for (j = 0; j <= i; j++) {
		token1 [i] += incnull;
		token1 [j] += incnull;
		token2 [i][j] += incnull;
	    }

	token1count = 0.0;
	token2count = 0.0;
	for (i = 0; i <= max_code; i++) {
	    token1count += token1 [i];
	    for (j = 0; j <= i; j++)
		token2count += token2 [i][j];
	}

	min2 = FLT_MAX;
	max2 = -FLT_MAX;
#ifndef LEVEN_REAL
	if (firstrun)
	    w_min = DIFFMAX;
#else
	if (firstrun)
	    w_min = FLT_MAX;
#endif
	w_max = 0;
	for (i = 0; i <= max_code; i++)
	    for (j = 0; j < i; j++)
		if (token2 [i][j] > 0.0 && w [i][j]) {
		    val = -log2 ( pow (token2 [i][j] / token2count, pmiExp) / ( (token1 [i] / token1count) * (token1 [j] / token1count) ) );
		    if (val < min2)
			min2 = val;
		    if (val > max2)
			max2 = val;
		    if (firstrun)
			if (w [i][j] < w_min)
			    w_min = w [i][j];
		    if (w [i][j] > w_max)
			w_max = w [i][j];

		}
	if (min2 == max2) {
	    if (min2 == 0.0) {
		min2 = -1.0;
		max2 = 1.0;
	    } else {
		min2 *= 0.5;
		max2 *= 2.0;
		if (min2 > max2) {
		    val = min2;
		    min2 = max2;
		    max2 = val;
		}
	    }
	}
	if (w_min == w_max) {
	    found = FALSE;
	    for (i = 0; i <= max_code; i++)
		for (j = 0; j < i; j++)
		    if (found) {
			if (w [i][j] > w_min && w [i][j] < w_max)
			    w_max = w [i][j];
		    } else {
			if (w [i][j] > w_min) {
			    w_max = w [i][j];
			    found = TRUE;
			}
		    }
	    if (found) {
		w_max = w_min + 0.9 * (w_max - w_min);
		if (firstrun)
		    w_min *= 0.5;
	    } else {
		w_min *= 0.5;
		w_max *= 2.0;
	    }
	} else if (firstrun)
	    w_min *= 0.5;

#ifndef LEVEN_REAL
	if (w_min < 1.0)
	    w_min = 1.0;
	if (w_max > DIFFMAX)
	    w_max = DIFFMAX;
#endif

	ssum = 0.0;
	for (i = 0; i <= max_code; i++)
	    for (j = 0; j < i; j++)
		if (token2 [i][j] > 0.0) {
		    val = -log2 ( pow (token2 [i][j] / token2count, pmiExp) / ( (token1 [i] / token1count) * (token1 [j] / token1count) ) );
		    val -= min2;
		    val /= (max2 - min2);
		    /* val = sqrt(val); */
		    val *= (w_max - w_min);
		    val += w_min;
#ifndef LEVEN_REAL
		    val = round (val);
		    if (val < 1.0)
			val = 1.0;
		    if (val > DIFFMAX)
			val = DIFFMAX;
#endif
		    val2 = val;
		    f = (w [i][j] - val2) / w_max;
		    ssum += f * f;
		}

	for (i = 0; i <= max_code; i++)
	    for (j = 0; j < i; j++)
		if (token2 [i][j] > 0.0) {
		    val = -log2 ( pow (token2 [i][j] / token2count, pmiExp) / ( (token1 [i] / token1count) * (token1 [j] / token1count) ) );
		    val -= min2;
		    val /= (max2 - min2);
		    /* val = sqrt(val); */
		    val *= (w_max - w_min);
		    val += w_min;
#ifndef __WIN32__
		    assert (! isinf (val));
		    assert (! isnan (val));
#endif
#ifndef LEVEN_REAL
		    val = round (val);
		    if (val < 1.0)
			val = 1.0;
		    if (val > DIFFMAX)
			val = DIFFMAX;
		    w [i][j] = round (((double) w [i][j]) * (1.0 - alpha) + alpha * val);
#else
		    w [i][j] = w [i][j] * (1.0 - alpha) + alpha * val;
#endif
		}

	if (verbose) {
	    fprintf (stderr, "Sum of square errors: %g\n", ssum);
	    fflush (stderr);
	}

	if (firstrun) {
	    saveweights ();
	    errsave = ssum;
	    errmin = ssum;
	    firstrun = FALSE;
	} else {
	    if (ssum < errmin) {
		saveweights ();
		errsave = ssum;
		errmin = ssum;
		runs = 6;
	    } else
		runs--;
	}

	if (ssum == 0.0)
	    break;

    }

    restoreweights ();

    if (verbose) {
	fprintf (stderr, "Sum of square errors: %g (final)\n", errsave);
	fflush (stderr);
    }


    fp = fopen (updated_subst_file, "w");
#ifdef LEVEN_REAL
    w_max1 = 0;
    for (i = 1; i <= max_code; i++)
	for (j = 0; j < i; j++)
	    if (w [i][j] > w_max1)
		w_max1 = w [i][j];
    fprintf (fp, "F: %g\n", w_max1);
#endif
    fprintf (fp, "%i\n", max_code);
    for (i = 1; i <= max_code; i++)
	for (j = 0; j < i; j++)
#ifdef LEVEN_REAL
	    fprintf (fp, "%g\n", w [i][j]);
#else
	    fprintf (fp, "%i\n", w [i][j]);
#endif
    fclose (fp);
    if (verbose)
	fprintf (stderr, "Updated substitution values written to %s\n", updated_subst_file);

}

void resubst_levenshtein (int const *s1, int l1, int n1, int const *s2, int l2, int n2)
{
    int
	i,
	x,
	y,
	newsize;
    WEIGHTTYPE
	aboveleft,
	above,
	left;

    if (l1 >= tsize || l2 >= tsize) {
	newsize = tsize + 16;
	while (l1 >= newsize || l2 >= newsize)
	    newsize += 16;
	for (i = 0; i < tsize; i++)
	    tbl [i] = (TBL_ *) s_realloc (tbl [i], newsize * sizeof (TBL_));
	tbl = (TBL_ **) s_realloc (tbl, newsize * sizeof (TBL_ *));
	while (tsize < newsize)
	    tbl [tsize++] = (TBL_ *) s_malloc (newsize * sizeof (TBL_));
    }
    for (x = 0; x <= l2; x++)
	for (y = 0; y <= l1; y++) {
	    tbl [x][y].above = FALSE;
	    tbl [x][y].left = FALSE;
	    tbl [x][y].aboveleft = FALSE;
	}
    tbl [0][0].i = 0;
    tbl [0][0].n = 1;
    for (x = 1; x <= l2; x++) {
        tbl [x][0].i = tbl [x - 1][0].i + weight (0, s2 [x - 1]);
	tbl [x][0].n = 1;
	tbl [x][0].left = TRUE;
    }
    for (y = 1; y <= l1; y++) {
        tbl [0][y].i = tbl [0][y - 1].i + weight (s1 [y - 1], 0);
	tbl [0][y].n = 1;
	tbl [0][y].above = TRUE;
    }
    for (x = 1; x <= l2; x++)
        for (y = 1; y <= l1; y++) {
            aboveleft = tbl [x - 1][y - 1].i + weight (s1 [y - 1], s2 [x - 1]);
            above     = tbl [x    ][y - 1].i + weight (s1 [y - 1], 0         );
            left      = tbl [x - 1][y    ].i + weight (0,          s2 [x - 1]);
            tbl [x][y].i = min3 (aboveleft, above, left);
	    i = 0;
	    if (aboveleft <= above && aboveleft <= left) {
		tbl [x][y].aboveleft = TRUE;
		i += tbl [x - 1][y - 1].n;
	    }
	    if (above <= aboveleft && above <= left) {
		tbl [x][y].above = TRUE;
		i += tbl [x][y - 1].n;
	    }
	    if (left <= aboveleft && left <= above) {
		tbl [x][y].left = TRUE;
		i += tbl [x - 1][y].n;
	    }
	    tbl [x][y].n = i;
        }

    retrack (s1, l1, s2, l2, ((double) (n1 * n2)) / tbl [l2][l1].n);

    /*

    printf ("\n\n%g\n\n", ((double) (n1 * n2)) / tbl [l2][l1].n);

    for (y = 0; y <= l1; y++) {
	printf ("\n");
	for (x = 0; x <= l2; x++)
	    printf ("%i  ", tbl [x][y].n);
    }

    printf ("\n\n");
    for (i = 1; i < 4; i++)
	printf ("%i %g\n", i, token1 [i]);
    for (i = 0; i < 4; i++)
	for (x = 0; x < i; x++)
	    printf ("%i %i %g\n", i, x, token2 [i][x]);
    exit(1);

    */
}

void retrack (int const *s1, int y, int const *s2, int x, double n)
{
    int
	c,
	c1,
	c2;
    double
	n2;

    if (tbl [x][y].aboveleft) {
	c1 = s1 [y - 1];
	c2 = s2 [x - 1];
	if (c1 != c2) {
	    n2 = n * ((double) (tbl [x - 1][y - 1].n));
	    token1 [c1] += n2;
	    token1 [c2] += n2;
	    token2 [(c1 > c2) ? c1 : c2][(c1 < c2) ? c1 : c2] += n2;
	}
	retrack (s1, y - 1, s2, x - 1, n);
    }
    if (tbl [x][y].above) {
	c = s1 [y - 1];
	n2 = n * ((double) (tbl [x][y - 1].n));
	token1 [0] += n2;
	token1 [c] += n2;
	token2 [c][0] += n2;
	retrack (s1, y - 1, s2, x, n);
    }
    if (tbl [x][y].left) {
	c = s2 [x - 1];
	n2 = n * ((double) (tbl [x - 1][y].n));
	token1 [0] += n2;
	token1 [c] += n2;
	token2 [c][0] += n2;
	retrack (s1, y, s2, x - 1, n);
    }
}

void process_args ()
{
    int
	i,
	j;
    while (arg_c > 1 && arg_v [1][0] == '-') {
        switch (arg_v [1][1]) {
	    case 'a':
		alpha = atof (get_arg ());
		if (alpha <= 0.0 || alpha > 1.0)
		    errit ("Value for -a out of range");
		break;
	    case 'b':
		bootstrap = TRUE;
		bootperc = atof (get_arg ());
		break;
	    case 'B':
		binair = TRUE;
		break;
	    case 'c':
		cAlpha = TRUE;
		break;
	    case 'C':
		if (! n_locations)
		    errit ("You can't use option -C before option -n");
		i = atoi (get_arg ());
		if (i < 1 || i > n_locations)
		    errit ("value for option -C out of range");
		if (pairtest == NULL) {
		    pairtest = (BOOL_ *) s_malloc (n_locations * sizeof (BOOL_));
		    for (j = 0; j < n_locations; j++)
			pairtest [j] = FALSE;
		}
		pairtest [i - 1] = TRUE;
	    case 'd':
		sd_file = get_arg ();
		use_sd = TRUE;
		break;
	    case 'e':
		indelsubstequal = TRUE;
		break;
	    case 'f':
		minf = atoi (get_arg ());
		if (minf < 2)
		    errit ("Illegal value for option -f, must be larger than 1");
		break;
	    case 'F':
		min2 = TRUE;
		break;
#ifndef LEVEN_REAL
	    case 'g':
		cGIW = TRUE;
		break;
#endif
	    case 'i':
		indel_file = get_arg ();
		break;
	    case 'h':
		hamming = TRUE;
		break;
	    case 'l':
		label_file = get_arg ();
		break;
	    case 'L':
		skip_missing = TRUE;
		break;
	    case 'm':
	        use_median = TRUE;
		break;
            case 'n':
		n_locations = atoi (get_arg ());
		pairtest = NULL;
		break;
            case 'N':
		normalise_function = atoi (get_arg ());
		break;
	    case 'o':
		output_file = get_arg ();
		break;
	    case 'p':
		part_file = get_arg ();
		break;
	    case 'P':
		pairwise = TRUE;
		break;
	    case 'q':
		verbose = FALSE;
		veryverbose = FALSE;
		break;
	    case 'Q':
		verbose = TRUE;
		veryverbose = FALSE;
		break;
	    case 'r':
		numeric = TRUE;
		break;
	    case 'R':
		minkowski = atof (get_arg ());
		if (minkowski <= 0.0)
		    errit ("Value for -R must be positive");
		break;
	    case 's':
		subst_file = get_arg ();
		break;
	    case 'S':
		updated_subst_file = get_arg ();
		updatesubst = TRUE;
		break;
	    case 't':
		test = TRUE;
		break;
	    case 'v':
		incnull = atof (get_arg ());
		if (incnull < 0.0 || incnull > 1.0)
		    errit ("Value for -v cannot be negative or larger dan 1");
		break;
	    case 'x':
		cAlphaOld = TRUE;
		break;
	    case 'z':
		pmiExp = atof (get_arg ());
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
	"Levenshtein, Version " levenVERSION "\n"
	"(c) P. Kleiweg 2000 - 2020\n"
	"\n"
    );
#ifdef LEVEN_REAL
    fprintf (
	err ? stderr : stdout,
	"Compiled with option LEVEN_REAL\n"
	"\n"
    );
#endif
#ifdef LEVEN_SMALL
    fprintf (
	err ? stderr : stdout,
	"Compiled with option LEVEN_SMALL\n"
	"\n"
    );
#endif
    fprintf (
	err ? stderr : stdout,
        "Usage: %s -n number\n"
        "\t[-d filename] [-i filename | -s filename"
#ifndef LEVEN_REAL
        " | -g"
#endif
        "] [-l filename] [-L]\n"
	"\t[-o filename] [-p filename] [-b percentage] [-B] [-c [-x]] [-e]\n"
	"\t[-h] [-r] [-R float]\n"
	"\t[-S filename] [-a float] [-v float] [-z float]\n"
	"\t[-f number] [-F] [-m] [-N number] [-q|-Q] [-t] datafile(s)\n"
	"\n"
        "Usage: %s -P -n number [-C number]\n"
        "\t[-i filename | -s filename] [-o filename] [-B] [-e]\n"
	"\t[-h] [-r] [-R float]\n"
	"\t[-S filename] [-a float] [-v float] [-z float]\n"
	"\t[-N number] [-q|-Q] datafile\n"
	"\n"
	"\t-a : weight learning: parameter alpha (now: %g)\n"
	"\t-b : do bootstrapping with given percentage (usually: 100)\n"
	"\t-B : binary comparison instead of Levenshtein\n"
	"\t-c : Cronbach's Alpha\n"
	"\t-C : select column (this option can be used more than once)\n"
	"\t-d : output file for standard deviation\n"
	"\t-e : equal weight for indel and subst\n"
	"\t-f : minimum number of occurrences required for each variant\n"
	"\t-F : skip files with less than two variants, depends on option -f\n"
#ifndef LEVEN_REAL
	"\t-g : character-based G.I.W.\n"
#endif
	"\t-i : file with indel values\n"
	"\t-h : Hamming distance instead of Levenshtein\n"
	"\t-l : file with location labels\n"
	"\t-L : skip locations that are not listed in the file with location labels\n"
	"\t-m : calculate median difference instead of mean difference\n"
	"\t-n : number of locations\n"
	"\t-N : select another normalisation function\n"
	"\t-o : output file\n"
	"\t-p : file defining a linkage partition\n"
	"\t-P : do pairwise calculations\n"
	"\t-q : quiet\n"
	"\t-Q : a bit quiet\n"
	"\t-r : numeric distance instead of Levenshtein\n"
	"\t-R : with -r: Minkowski value (now: %g)\n"
	"\t-s : file with substitution and indel values\n"
	"\t-S : weight learning: output file with updated substitution and indel values\n"
	"\t-t : test all input files\n"
	"\t-x : also give Cronbach's Alpha using old (incorrect) formula\n"
	"\t-v : weight learning: increment value for all alignments (now: %g)\n"
	"\t-z : weight learning: exponent in PMI formula (now: %g)\n"
	"\n",
	programname,
	programname,
	alpha,
	minkowski,
	incnull,
	pmiExp
    );
    exit (err);
}
