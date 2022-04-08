/*
 * File: features.l
 *
 * (c) Peter Kleiweg
 *     2003 - 2010
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2,
 * or (at your option) any later version.
 *
 * Compile:
 *
 *    flex features.l
 *    gcc -s -Wall -o features lex.yy.c -lfl -lm
 *    rm lex.yy.c
 *
 */

%{

#define my_VERSION "2.03"

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
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFSIZE 4095

#define I1 " %i"
#define I2 " %*s %i"
#define F2 " %*s %f"
#define F3 " %*s %*s %f"
#define I4 " %*s %*s %*s %i"
#define F4 " %*s %*s %*s %f"
#define S1 " %4095s"
#define S2 " %*s %4095s"
#define S3 " %*s %*s %4095s"
#define S4 " %*s %*s %*s %4095s"

typedef enum { FALSE = 0, TRUE } BOOL_;

typedef enum { m_SQUARE, m_MINKOWSKI} METHOD_;
typedef enum { f_BITMAP, f_NUMERIC, f_DISCRETE } f_TYPE;
typedef enum { a_TEMPLATE, a_dDEF, a_nDEF, a_nADD, a_nSUB, a_nMUL, a_bDEF, a_bSET, a_bUNSET, a_bFLIP, a_UNDEF } a_TYPE;
typedef enum { s_HEAD, s_MOD, s_PRE } s_TYPE;
typedef enum { st_ANY, st_IS, st_ISNOT, st_AND, st_ANDNOT } st_TYPE;
typedef enum { e_STOP, e_GO, e_COUNTDOWN } es_TYPE;

typedef struct {
    int
	feature,
	st_val;
    a_TYPE
	type;
    st_TYPE
	st_type;
    union {
	int
	    b;
	int
	    d;
	float
	    n;
	int
	    t;
    } v;
} ACTION_;

typedef struct {
    char
	*s;
    int
        len,
        n_actions,
	state_val;
    s_TYPE
	type;
    st_TYPE
	state_type;
    BOOL_
	ignore;
    ACTION_
	*actions;
} STRING_;

typedef struct {
    BOOL_
        defined;
    union {
        int
            b;
        int
            d;
        float
            n;
    } val;
} VALUE_;

typedef struct {
    f_TYPE
        type;
    float
	weight,
	max;
    char
        *name;
} FEATURE_;

typedef struct {
    char
        *name;
    int
        n_actions;
    ACTION_
        *actions;
} TEMPLATE_;

typedef struct token_ {
    long int
	i;
    VALUE_
	*values;
    int
	*string_count,
	n_strings,
	max_strings;
    char
    	**string;
    struct token_
	*left,
	*right;
} TOKEN_;

typedef struct tokenstring_ {
    char
    	*s;
    struct tokenstring_
	*left,
	*right;
} TOKENSTRING_;

typedef struct {
    int
	string,
	state;
} PRE_;

typedef struct {
    float
        f,
        fmin,
	fmax;
} RANGE_;

BOOL_
    tokenesc = FALSE,
    erred,
    ansi = FALSE,
    toINT = TRUE,
    showlabels = FALSE,
    tokenized = FALSE;
 

es_TYPE
    errorstop = e_STOP;

METHOD_
    method = m_MINKOWSKI;

float
    rho = 1.0,
    f1,
    f2,
    f3;

TEMPLATE_
    *templates = NULL,
    *tmp_template;

FEATURE_
    *features = NULL,
    *fptr;

VALUE_
    *indelvalues = NULL,
    *scratch;

STRING_
    *strings = NULL;

TOKEN_
    *root = NULL,
    **a_tokens,
    **tokbuf_tok = NULL;

TOKENSTRING_
    *rootstring = NULL;

RANGE_
    *ranges = NULL;

ACTION_
    *actions = NULL;

st_TYPE
    state_type = st_ANY;

PRE_
    *pre = NULL;

long int
    n_tokens = 0,
    outbuf [BUFSIZE + 1];

int
    n_ranges = 0,
    max_ranges = 0,
    feat_version = -1,
    bufscan1,
    bufscan2,
    tokenstart,
    errorcount,
    errsum = 0,
    errorbase = 0,
    n_outbuf,
    arg_c,
    state_val,
    state_current = 0,
    state_new,
    i,
    j,
    *states = NULL,
    n_states = 0,
    startstate = 0,
    n_pre = 0,
    max_pre = 0,
    max_states = 0,
    n_features = 0,
    max_features = 0,
    n_templates = 0,
    max_templates = 0,
    n_strings = 0,
    max_strings = 0,
    n_actions = 0,
    max_actions = 0,
    n_tokbuf = 0,
    max_tokbuf = 0,
    weight_index,
    indel_index,
    state_index,
    offsets [256];


long int
    top = 65535,
    lineno;

float
    substmax = 1.0,
    indelmax = -1.0;

char const
    *infile;

char
    **tokbuf_str = NULL,
    **arg_v,
    buffer [BUFSIZE + 1],
    buffr2 [BUFSIZE + 1],
    bufesc [BUFSIZE + 1],
    currentlabel [BUFSIZE + 1],
    *diffile = "features.table.out",
    *logerrors = NULL,
    *logfeatures = NULL,
    *logtokens = NULL,
    *prefix,
    *programname,
    *no_mem_buffer,
    out_of_memory [] = "Out of memory",
    no_template [] = "No template to define features on";
FILE
    *fperrors = NULL;


void
    encodespace (int offset),
    run_actions (VALUE_* values, int n_actions, ACTION_* actions, s_TYPE),
    run_pre_states (int n_actions, ACTION_* actions),
    closetemplate (void),
    closestring (void),
    add_action_i (int, a_TYPE, int),
    add_action_f (int, a_TYPE, float),
    runtokens (TOKEN_ *t),
    tokflush (BOOL_ in_ignore),
    process_file (char const *filename),
    info (void),
    instr (void),
    isdefined (VALUE_ v),
    init_string (char const *s, char t, BOOL_ ignore),
    add_feature (char,  char const *, float, float),
    get_programname (char const *argv0),
    process_args (void),
    emessage (char const *format, ...),
    errit (es_TYPE errorstop, char const *format, ...),
    mexit (int retval),
    warning (char const *format, ...),
    syntax (void),
    *s_malloc (size_t size),
    *s_realloc (void *block, size_t size);
char
    *escape (void),
    *escape2 (char *s),
    *unescape (char *s),
    *get_arg (void),
    *get_string (char const *s, char const *format),
    *s_strdup (char const *s);
int
    get_int (char const *s, char const *format),
    get_feature (char const *s),
    get_template (char const *s),
    cmpfeature (void const *p1, void const *p2),
    cmptemplate (void const *p1, void const *p2),
    cmpstring (void const *p1, void const *p2),
    scanfeature (void const *key, void const *p),
    scantemplate (void const *key, void const *p),
    scanstring (void const *key, void const *p);
float
    get_float (char const *s, char const *format),
    tokendif (long int li, long int lj);
VALUE_
    *newvalues (void);

%}

%option batch noyywrap nounput 8bit caseful

SPC	([ \t\f\r]+)
SPC0	([ \t\f\r]*)
WORD	([^ \t\f\r\n]+)
INT	([+-]?[0-9]+)
FLOAT	([+-]?([0-9]+(\.[0-9]+)?|\.[0-9]+))

%Start _defines _features0 _features1 _templates _templates_cond _indels _indels_template _tokens _tokens_template _tokens_cond _error

%%

<_tokens_template>{

\n				{ lineno++;
				  state_type = st_ANY;
				  BEGIN _tokens;
				}

}


<_indels_template>{

\n				{ lineno++;
				  BEGIN _indels;
				}

}

<INITIAL,_defines,_features0,_features1,_templates,_indels,_indels_template,_tokens,_tokens_template>{

{SPC}				;

\#.*				;

\n				{ state_type = st_ANY;
				  lineno++;
				}

}

<INITIAL>{

DEFINES				{ BEGIN _defines; }

}

<_defines>{

FEATURES			{ if (indelmax < 0.0)
				      indelmax = substmax / 2.0;
				  prefix = "FEATURES: ";
				  if (feat_version < 0) {
				      feat_version = 0;
				      warning ("section DEFINES: VERSION missing, using VERSION 0");
				  }
				  if (feat_version == 0) {
				      BEGIN _features0;
				  } else {
				      BEGIN _features1;
				  }
				}

TOP{SPC}{INT}			{ top = get_int (yytext, I2);
				  if (top < 255 || top > 65535)
				      errit (e_STOP, "TOP out of range [255..65535]");
				}

SUBSTMAX{SPC}{FLOAT}		{ substmax = get_float (yytext, F2);
				  if (substmax <= 0.0)
				      errit (e_STOP, "SUBSTMAX must be positive");
				}

INDEL{SPC}{FLOAT}		{ indelmax = get_float (yytext, F2);
				  if (indelmax <= 0.0)
				      errit (e_STOP, "INDEL must be positive");
				}

METHOD{SPC}SUM			{ method = m_MINKOWSKI; rho = 1; }

METHOD{SPC}SQUARE		{ method = m_SQUARE; }

METHOD{SPC}EUCLID		{ method = m_MINKOWSKI; rho = 2; }

METHOD{SPC}MINKOWSKI{SPC}{FLOAT}  {
				  method = m_MINKOWSKI;
				  rho = get_float (yytext, F3);
				  if (rho <= 0.0)
				      errit (e_STOP, "Minkowski value should be positive");
				}

TOKENSTRING{SPC}RAW		{ tokenesc = FALSE; }

TOKENSTRING{SPC}ESC		{ tokenesc = TRUE; }

START{SPC}{INT}			{ startstate = get_int (yytext, I2); }

VERSION{SPC}0			{
				  feat_version = 0; 
				  warning ("section DEFINES: you should not use VERSION 0");
				}

VERSION{SPC}1			{ feat_version = 1; }

VERSION{SPC}2			{ feat_version = 2; }

RANGE{SPC}{FLOAT}{SPC}{FLOAT}{SPC}{FLOAT} {
                                  if (sscanf (yytext + 5, " %f %f %f", &f1, &f2, &f3) < 3)
				      errit (e_STOP, "sectione DEFINES: RANGE: illegal values");
				  if (n_ranges == max_ranges) {
				      max_ranges += 16;
				      ranges = (RANGE_ *) s_realloc (ranges, max_ranges * sizeof (RANGE_));
				  }
				  ranges [n_ranges].fmin = f1;
				  ranges [n_ranges].fmax = f2;
				  ranges [n_ranges].f = f3;
				  n_ranges += 1;
                                }

}

<_features0,_features1>{

TEMPLATES			{ if (n_features <= 3)
				      errit (e_STOP, "No features defined");
				  qsort (features, n_features, sizeof (FEATURE_), cmpfeature);
				  fptr = (FEATURE_ *) bsearch ("WEIGHT", features, n_features, sizeof (FEATURE_), scanfeature);
				  weight_index = fptr - features;
				  fptr = (FEATURE_ *) bsearch ("INDEL", features, n_features, sizeof (FEATURE_), scanfeature);
				  indel_index = fptr - features;
				  fptr = (FEATURE_ *) bsearch ("STATE", features, n_features, sizeof (FEATURE_), scanfeature);
				  state_index = fptr - features;

				  prefix = "TEMPLATES: ";
				  BEGIN _templates;
				}

}

<_features0>{

[BND]{SPC}{WORD}	        { add_feature (yytext[0], get_string (yytext, S2), 1.0, 1.0); }

[BND]{SPC}{FLOAT}{SPC}{WORD}	{ add_feature (yytext[0], get_string (yytext, S3), get_float (yytext, F2), 1.0); }

}

<_features1>{

[BND]{SPC}{FLOAT}{SPC}{WORD}	            { add_feature (yytext[0], get_string (yytext, S3), 1.0, get_float (yytext, F2)); }

[BND]{SPC}{FLOAT}{SPC}{FLOAT}{SPC}{WORD}    { add_feature (yytext[0], get_string (yytext, S4), get_float (yytext, F3), get_float (yytext, F2)); }

}

<_templates>{

INDELS				{ closetemplate ();
				  if (n_templates)
				      qsort (templates, n_templates, sizeof (TEMPLATE_), cmptemplate);
				  prefix = "INDELS: ";
				  BEGIN _indels;
				}

T{SPC}{WORD}			{ closetemplate ();
                                  get_string (yytext, S2);  /* put string in var buffer */
				  for (i = 0; i < n_templates; i++)
				       if (! strcmp (buffer, templates [i].name))
					  errit (e_STOP, "Template \"%s\" already defined", buffer);
				  if (n_templates == max_templates) {
				      max_templates += 64;
				      templates = (TEMPLATE_ *) s_realloc (templates, max_templates * sizeof (TEMPLATE_));
				  }
				  templates [n_templates].name = s_strdup (buffer);
				  templates [n_templates].actions = NULL;
				  templates [n_templates].n_actions = 0;
				  n_templates++;
				}

\={SPC}{INT}{SPC}		{ state_val = get_int (yytext, I2);
				  state_type = st_IS;
				  BEGIN _templates_cond;
				}

\^\={SPC}{INT}{SPC}		{ state_val = get_int (yytext, I2);
				  state_type = st_ISNOT;
				  BEGIN _templates_cond;
				}

\:{SPC}{INT}{SPC}		{ state_val = get_int (yytext, I2);
				  state_type = st_AND;
				  BEGIN _templates_cond;
				}

\^\:{SPC}{INT}{SPC}		{ state_val = get_int (yytext, I2);
				  state_type = st_ANDNOT;
				  BEGIN _templates_cond;
				}

}

<_templates,_templates_cond>{

F{SPC}{WORD}{SPC}\={SPC}{FLOAT}	{ if (! n_templates)
				      errit (e_STOP, no_template);
				  i = get_feature (get_string (yytext, S2));
				  switch (features [i].type) {
				      case f_BITMAP:
					  add_action_i (i, a_bDEF, get_int (yytext, I4));
					  break;
				      case f_NUMERIC:
					  add_action_f (i, a_nDEF, get_float (yytext, F4));
					  break;
				      case f_DISCRETE:
					  add_action_i (i, a_dDEF, get_int (yytext, I4));
					  break;
				  }
				  BEGIN _templates;
				}

F{SPC}{WORD}{SPC}\-{SPC}{FLOAT}	{ if (! n_templates)
				      errit (e_STOP, no_template);
				  i = get_feature (get_string (yytext, S2));
				  switch (features [i].type) {
				      case f_BITMAP:
					  add_action_i (i, a_bUNSET, get_int (yytext, I4));
					  break;
				      case f_NUMERIC:
					  add_action_f (i, a_nSUB, get_float (yytext, F4));
					  break;
				      case f_DISCRETE:
					  errit (e_STOP, "Illegal operator for discrete feature");
					  break;
				  }
				  BEGIN _templates;
				}

F{SPC}{WORD}{SPC}\+{SPC}{FLOAT}	{ if (! n_templates)
				      errit (e_STOP, no_template);
				  i = get_feature (get_string (yytext, S2));
				  switch (features [i].type) {
				      case f_BITMAP:
					  add_action_i (i, a_bSET, get_int (yytext, I4));
					  break;
				      case f_NUMERIC:
					  add_action_f (i, a_nADD, get_float (yytext, F4));
					  break;
				      case f_DISCRETE:
					  errit (e_STOP, "Illegal operator for discrete feature");
					  break;
				  }
				  BEGIN _templates;
				}

F{SPC}{WORD}{SPC}\*{SPC}{FLOAT}	{ if (! n_templates)
				      errit (e_STOP, no_template);
				  i = get_feature (get_string (yytext, S2));
				  switch (features [i].type) {
				      case f_BITMAP:
					  errit (e_STOP, "Illegal operator for bitmap feature");
					  break;
				      case f_NUMERIC:
					  add_action_f (i, a_nMUL, get_float (yytext, F4));
					  break;
				      case f_DISCRETE:
					  errit (e_STOP, "Illegal operator for discrete feature");
					  break;
				  }
				  BEGIN _templates;
				}

F{SPC}{WORD}{SPC}\!{SPC}{FLOAT}	{ if (! n_templates)
				      errit (e_STOP, no_template);
				  i = get_feature (get_string (yytext, S2));
				  switch (features [i].type) {
				      case f_BITMAP:
					  add_action_i (i, a_bFLIP, get_int (yytext, I4));
					  break;
				      case f_NUMERIC:
					  errit (e_STOP, "Illegal operator for numeric feature");
					  break;
				      case f_DISCRETE:
					  errit (e_STOP, "Illegal operator for discrete feature");
					  break;
				  }
				  BEGIN _templates;
				}

F{SPC}{WORD}{SPC}U		{ if (! n_templates)
				      errit (e_STOP, no_template);
				  i = get_feature (get_string (yytext, S2));
				  if (i == state_index)
				      errit (e_STOP, "Can't undefine STATE");
				  add_action_i (i, a_UNDEF, 0);
				  BEGIN _templates;
				}

}

<_indels>{

TOKENS				{ prefix = "TOKENS: ";
				  BEGIN _tokens; }

T{SPC}				{ if (indelvalues == NULL)
				      indelvalues = newvalues ();
				  BEGIN _indels_template;
				}

F{SPC}{WORD}{SPC}\={SPC}{FLOAT}	{ if (indelvalues == NULL)
				      indelvalues = newvalues ();
				  i = get_feature (get_string (yytext, S2));
				  switch (features [i].type) {
				      case f_BITMAP:
					  indelvalues [i].val.b = get_int (yytext, I4);
					  break;
				      case f_NUMERIC:
					  indelvalues [i].val.n = get_float (yytext, F4);
					  break;
				      case f_DISCRETE:
					  indelvalues [i].val.d = get_int (yytext, I4);
					  break;
				  }
				  indelvalues [i].defined = TRUE;
				}

}

<_tokens>{

\={SPC}{INT}{SPC}		{ state_val = get_int (yytext, I2);
				  state_type = st_IS;
				  BEGIN _tokens_cond;
				}

\^\={SPC}{INT}{SPC}		{ state_val = get_int (yytext, I2);
				  state_type = st_ISNOT;
				  BEGIN _tokens_cond;
				}

\:{SPC}{INT}{SPC}		{ state_val = get_int (yytext, I2);
				  state_type = st_AND;
				  BEGIN _tokens_cond;
				}

\^\:{SPC}{INT}{SPC}		{ state_val = get_int (yytext, I2);
				  state_type = st_ANDNOT;
				  BEGIN _tokens_cond;
				}
}

<_tokens,_tokens_cond>{


T{SPC}				{ instr ();
				  BEGIN _tokens_template;
				}

[HMP]I?{SPC}{WORD}		{ closestring ();
				  init_string (	unescape (get_string (yytext, S2)),
						yytext [0],
						(yytext [1] == 'I') ? TRUE : FALSE );
				  BEGIN _tokens;
				}

EOT				{ closestring ();
				  init_string ( "\n", 'H', TRUE);
				  BEGIN _tokens;
				}


F{SPC}{WORD}{SPC}\={SPC}{FLOAT}	{ instr ();
				  i = get_feature (get_string (yytext, S2));
				  switch (features [i].type) {
				      case f_BITMAP:
					  add_action_i (i, a_bDEF, get_int (yytext, I4));
					  break;
				      case f_NUMERIC:
					  add_action_f (i, a_nDEF, get_float (yytext, F4));
					  break;
				      case f_DISCRETE:
					  add_action_i (i, a_dDEF, get_int (yytext, I4));
					  break;
				  }
				  BEGIN _tokens;
				}

F{SPC}{WORD}{SPC}\-{SPC}{FLOAT}	{ instr ();
				  i = get_feature (get_string (yytext, S2));
				  switch (features [i].type) {
				      case f_BITMAP:
					  add_action_i (i, a_bUNSET, get_int (yytext, I4));
					  break;
				      case f_NUMERIC:
					  add_action_f (i, a_nSUB, get_float (yytext, F4));
					  break;
				      case f_DISCRETE:
					  errit (e_STOP, "Illegal operator for discrete feature");
					  break;
				  }
				  BEGIN _tokens;
				}

F{SPC}{WORD}{SPC}\+{SPC}{FLOAT}	{ instr ();
				  i = get_feature (get_string (yytext, S2));
				  switch (features [i].type) {
				      case f_BITMAP:
					  add_action_i (i, a_bSET, get_int (yytext, I4));
					  break;
				      case f_NUMERIC:
					  add_action_f (i, a_nADD, get_float (yytext, F4));
					  break;
				      case f_DISCRETE:
					  errit (e_STOP, "Illegal operator for discrete feature");
					  break;
				  }
				  BEGIN _tokens;
				}

F{SPC}{WORD}{SPC}\*{SPC}{FLOAT}	{ instr ();
				  i = get_feature (get_string (yytext, S2));
				  switch (features [i].type) {
				      case f_BITMAP:
					  errit (e_STOP, "Illegal operator for bitmap feature");
					  break;
				      case f_NUMERIC:
					  add_action_f (i, a_nMUL, get_float (yytext, F4));
					  break;
				      case f_DISCRETE:
					  errit (e_STOP, "Illegal operator for discrete feature");
					  break;
				  }
				  BEGIN _tokens;
				}

F{SPC}{WORD}{SPC}\!{SPC}{FLOAT}	{ instr ();
				  i = get_feature (get_string (yytext, S2));
				  switch (features [i].type) {
				      case f_BITMAP:
					  add_action_i (i, a_bFLIP, get_int (yytext, I4));
					  break;
				      case f_NUMERIC:
					  errit (e_STOP, "Illegal operator for numeric feature");
					  break;
				      case f_DISCRETE:
					  errit (e_STOP, "Illegal operator for discrete feature");
					  break;
				  }
				  BEGIN _tokens;
				}

F{SPC}{WORD}{SPC}U		{ instr ();
				  i = get_feature (get_string (yytext, S2));
				  if (i == state_index)
				      errit (e_STOP, "Can't undefine STATE");
				  add_action_i (i, a_UNDEF, 0);
				  BEGIN _tokens;
				}

}

<_indels_template>{

{WORD}{SPC0}			{ tmp_template = (TEMPLATE_ *) bsearch (
						get_string (yytext, S1),
						templates,
						n_templates,
						sizeof (TEMPLATE_),
						scantemplate
					);
				  if (tmp_template == NULL)
				      errit (e_STOP, "Template \"%s\" not found", buffer);
				  run_actions (indelvalues, tmp_template->n_actions, tmp_template->actions, s_HEAD);
				}

}

<_tokens_template>{

{WORD}{SPC0}			{ add_action_i (0, a_TEMPLATE, get_template (get_string (yytext, S1))); }

}

<INITIAL,_defines,_features0,_features1,_templates,_templates_cond,_indels,_indels_template,_tokens,_tokens_template,_tokens_cond>{

.				{ yyless (0);
				  BEGIN _error;
				}

}

<_error>.*			{ errit (e_STOP, "Syntax error: %s", yytext); }


%%

int main (int argc, char *argv [])
{
    int
	i,
	j;
    long int
	li,
	lj;
    float
	f,
	maxdif;
    FILE
        *fp;

    currentlabel [0] = '\0';

    no_mem_buffer = (char *) malloc (1024);

    get_programname (argv [0]);

    tokenesc = TRUE;
    unescape ("\\050\\d050\\x50");
    if (bufesc [0] != '\050')
	errit (e_STOP, "TEST: Parsing octal escape sequence");
    if (bufesc [1] != '\062')
	errit (e_STOP, "TEST: Parsing decimal escape sequence");
    if (bufesc [2] != '\120')
	errit (e_STOP, "TEST: Parsing hexadecimal escape sequence");
    tokenesc = FALSE;

    arg_c = argc;
    arg_v = argv;
    process_args ();

    if (arg_c < 3)
	syntax ();

    if (logerrors) {
	fperrors = fopen (logerrors, "w");
	if (! fperrors)
	    errit (e_STOP, "Creating file \"%s\": %s", logerrors, strerror (errno));
    }

    add_feature ('N', "WEIGHT", 1.0, 1.0);
    add_feature ('N', "INDEL", 1.0, 1.0);
    add_feature ('B', "STATE", 1.0, 1.0);

    infile = arg_v [1];
    prefix = "";
    yyin = fopen (infile, "r");
    if (! yyin)
	errit (e_STOP, "Opening file \"%s\": %s", infile, strerror (errno));
    lineno = 1;
    yylex ();
    closestring ();
    lineno = 0;
    fclose (yyin);
    prefix = "";

    for (i = 0; i < n_strings; i++)
	if (strings [i].s [0] == '\n')
	    break;
    if (i == n_strings) {
	init_string ( "\n", 'H', TRUE);
	closestring ();
    }

    qsort (strings, n_strings, sizeof (STRING_), cmpstring);
    for (i = 0; i < 256; i++)
        offsets [i] = -1;
    for (i = 0; i < n_strings; i++) {
        j = (unsigned char) (strings [i].s [0]);
        if (offsets [j] < 0)
            offsets [j] = i;
    }

	/*
    info ();
    return 0;
	*/

    scratch = (VALUE_ *) s_malloc (n_features * sizeof (VALUE_));

    for (i = 2; i < arg_c; i++)
	process_file (arg_v [i]);

    a_tokens = (TOKEN_ **) s_malloc ((n_tokens + 1) * sizeof (TOKEN_ *));
    runtokens (root);
    maxdif = 0.0;
    for (li = 1; li <= n_tokens; li++)
	for (lj = 0; lj < li; lj++) {
	    f = tokendif (li, lj);
	    if (f > maxdif)
		maxdif = f;
    }
    fp = fopen (diffile, "w");
    if (! fp)
        errit (e_STOP, "Creating file \"%s\": %s", diffile, strerror (errno));
    if (toINT) {
	emessage ("Mapping [0, %g] -> [0, %li]\n", maxdif, top);
    } else {
	emessage ("No mapping from real to integer. Max real: %g\n", maxdif);
	fprintf (fp, "F: %g\n", maxdif);
    }

    fprintf (fp, "%li\n", n_tokens);
    for (li = 1; li <= n_tokens; li++)
	for (lj = 0; lj < li; lj++)
	    if (toINT)
		fprintf (fp, "%i\n", (int)(.5 + tokendif (li, lj) / maxdif * top));
	    else
		fprintf (fp, "%g\n", tokendif (li, lj));
    fclose (fp);
    if (errsum)
	emessage ("%i errors\n", errsum);
    printf ("Differences written to file \"%s\"\n", diffile);

    if (logfeatures != NULL) {
	fp = fopen (logfeatures, "w");
	if (! fp)
            errit (e_STOP, "Creating file \"%s\": %s", logfeatures, strerror (errno));
	for (li = 1; li <= n_tokens; li++) {
	    fprintf (fp, "TOKEN %li\n", a_tokens [li]->i);
	    for (j = 0; j < n_features; j++) {
		if (a_tokens [li]->values [j].defined == TRUE) {
		    switch (features [j].type) {
			case f_BITMAP:
			    fprintf (fp, "%12i\t%s\n", a_tokens [li]->values [j].val.b, features [j].name);
			    break;
			case f_NUMERIC:
			    fprintf (fp, "%12g\t%s\n", a_tokens [li]->values [j].val.n, features [j].name);
			    break;
			case f_DISCRETE:
			    fprintf (fp, "%12i\t%s\n", a_tokens [li]->values [j].val.d, features [j].name);
			    break;
		    }
		}
	    }
	    fprintf (fp, "\n");
	}
	fclose (fp);
	printf ("Feature sets saved to file \"%s\"\n", logfeatures);
    }

    if (logtokens != NULL) {
	fp = fopen (logtokens, "w");
	if (! fp)
            errit (e_STOP, "Creating file \"%s\": %s", logtokens, strerror (errno));

	for (li = 1; li <= n_tokens; li++) {
	    j = 0;
	    for (i = 0; i < a_tokens [li]->n_strings; i++)
		j += a_tokens [li]->string_count [i];
	    fprintf (fp, "TOKEN %li  %i\n", a_tokens [li]->i, j);
	    for (i = 0; i < a_tokens [li]->n_strings; i++)
		fprintf (fp, "%12i\t%s\n", a_tokens [li]->string_count [i], escape2 (a_tokens [li]->string [i]));
	    fprintf (fp, "\n");
	}

	fclose (fp);
	printf ("Token strings saved to file \"%s\"\n", logtokens);
    }

    if (fperrors ) {
	printf ("Errors written to file \"%s\"\n", logerrors);
	fclose (fperrors);
    }

    return 0;

}

void add_action (int feature, a_TYPE a_type)
{
    if (n_actions == max_actions) {
	max_actions += 256;
	actions = (ACTION_ *) s_realloc (actions, max_actions * sizeof (ACTION_));
    }
    actions [n_actions].feature = feature;
    actions [n_actions].type = a_type;
    if (state_type == st_ANY)
	actions [n_actions].st_type = st_ANY;
    else {
	actions [n_actions].st_type = state_type;
	actions [n_actions].st_val = state_val;
	if (a_type != a_TEMPLATE)
	    state_type = st_ANY;
    }
    n_actions++;
}

void add_action_i (int feature, a_TYPE a_type, int value)
{
    add_action (feature, a_type);
    switch (a_type) {
	case a_TEMPLATE:
	    actions [n_actions - 1].v.t = value;
	    break;
	case a_dDEF:
	    actions [n_actions - 1].v.d = value;
	    break;
	case a_bDEF:
	case a_bSET:
	case a_bUNSET:
	case a_bFLIP:
	    actions [n_actions - 1].v.b = value;
	    break;
	case a_nDEF:
	case a_nADD:
	case a_nSUB:
	case a_nMUL:
	case a_UNDEF:
	    break;
    }
}

void add_action_f (int feature, a_TYPE a_type, float value)
{
    add_action (feature, a_type);
    actions [n_actions - 1].v.n = value;
}

void closetemplate ()
{
    if (n_templates > 0 && templates [n_templates - 1].actions == NULL) {
	templates [n_templates - 1].actions = (ACTION_ *) s_malloc (n_actions * sizeof (ACTION_));
	memcpy (templates [n_templates - 1].actions, actions, n_actions * sizeof (ACTION_));
	templates [n_templates - 1].n_actions = n_actions;
    }
    n_actions = 0;
}

void closestring ()
{
    if (n_strings > 0 && strings [n_strings - 1].actions == NULL) {
	strings [n_strings - 1].actions = (ACTION_ *) s_malloc (n_actions * sizeof (ACTION_));
	memcpy (strings [n_strings - 1].actions, actions, n_actions * sizeof (ACTION_));
	strings [n_strings - 1].n_actions = n_actions;
    }
    n_actions = 0;
}

float valuesdif (VALUE_ *p1, VALUE_ *p2)
{
    float
	f,
	sum;
    int
	i;
    BOOL_
	found;

    found = FALSE;
    f = sum = 0;
    for (i = 0; i < n_features; i++) {
	if (i == weight_index || i == indel_index || i == state_index)
	    continue;
	if (p1 [i].defined && p2 [i].defined) {
	    found = TRUE;
	    switch (features [i].type) {
	        case f_BITMAP:
		    f = (p1 [i].val.b & p2 [i].val.b) ? 0.0 : 1.0;
		    break;
	        case f_NUMERIC:
		    f = fabs (p1 [i].val.n - p2 [i].val.n);
		    break;
	        case f_DISCRETE:
		    f = (p1 [i].val.d == p2 [i].val.d) ? 0.0 : 1.0;
		    break;
	    }
	} else if (feat_version == 1 || p1 [i].defined || p2 [i].defined)
	    f = features [i].max;
	else
	    f = 0.0;

	f *= features [i].weight;

	switch (method) {
	    case m_MINKOWSKI:
		sum += pow (f, rho);
		break;
	    case m_SQUARE:
		sum += f * f;
		break;
	}
    }

    if (feat_version == 0 && ! found)
	return substmax;

    if (sum < 0.0)
	sum = 0.0;

    switch (method) {
	case m_MINKOWSKI:
	    sum = pow (sum, 1.0 / rho);
	    break;
	case m_SQUARE:
	    break;
    }

    return sum * p1 [weight_index].val.n * p2 [weight_index].val.n;
}

float tokendif (long int li, long int lj)
{
    float
	f;
    VALUE_
	*pi;

    pi = a_tokens [li]->values;
    if (lj == 0) {
	if (pi [indel_index].defined)
	    f = pi [indel_index].val.n;
	else if (indelvalues != NULL)
	    f = valuesdif (pi, indelvalues);
	else
	    f = indelmax;
    } else
	f = valuesdif (pi, a_tokens [lj]->values);


    for (i = 0; i < n_ranges; i++)
	if (f >= ranges [i].fmin && f <= ranges [i].fmax)
	    return ranges [i].f;

    if (f < 0.0)
	f = 0.0;
    if (f > substmax)
	f = substmax;

    return f;
}

void runtokens (TOKEN_ *t)
{
    if (t == NULL)
	return;

    a_tokens [t->i] = t;
    runtokens (t->left);
    runtokens (t->right);
}

int checkstate (int i)
{
    int
	retval;

    retval = 0;
    switch (strings [i].state_type) {
	case st_ANY:
	    retval = 1;
	    break;
	case st_IS:
	    if (state_current == strings [i].state_val)
		retval = 1;
	    break;
	case st_ISNOT:
	    if (state_current != strings [i].state_val)
		retval = 1;
	    break;
	case st_AND:
	    if (state_current & strings [i].state_val)
		retval = 1;
	    break;
	case st_ANDNOT:
	    if (! (state_current & strings [i].state_val))
		retval = 1;
	    break;
    }
    return retval;
}

void process_file (char const *filename)
{
    BOOL_
        in_head,
        in_ignore;
    char
	c;
    int
        found,
	blockval,
        i,
        j,
        k,
        l,
	n;
    FILE
	*fpin,
	*fpout,
	*fpout2 = NULL;
    TOKEN_
	*tok;

    lineno = 0;
    infile = filename;
    currentlabel [0] = '\0';
    fpin = fopen (infile, "r");
    if (! fpin)
	errit (e_STOP, "Opening file \"%s\": %s", infile, strerror (errno));
    sprintf (buffer, "%s.ftr", infile);
    fpout = fopen (buffer, "w");
    if (! fpout)
	errit (e_STOP, "Creating file \"%s\": %s", buffer, strerror (errno));
    if (tokenized) {
	sprintf (buffer, "%s.ftr.tok", infile);
	fpout2 = fopen (buffer, "w");
	if (! fpout2)
	    errit (e_STOP, "Creating file \"%s\": %s", buffer, strerror (errno));
    }
    while (fgets (buffer, BUFSIZE, fpin) != NULL) {
	lineno++;
	if (buffer [0] == '+')
	    errit (e_STOP, "Invalid input");
	if (showlabels)
	    if (buffer [0] == ':' || isdigit ((unsigned char) buffer [0]))
		strcpy (currentlabel, buffer);
	if (buffer [0] != '-' && sscanf(buffer, "[ %d ] %c %n", &blockval, &c, &n) < 2) {
	    fputs (buffer, fpout);
	    if (tokenized)
		fputs (buffer, fpout2);
	    continue;
	}
	if (buffer [0] != '-' && c != '-')
	    errit (e_STOP, "Invalid input");
	n_outbuf = 0,
	erred = FALSE;
	j = strlen (buffer);
	while (j > 0 && isspace ((unsigned char) buffer [j - 1]))
	    buffer [--j] = '\0';
	buffer [j++] = '\n';
	buffer [j] = '\0';
	if (buffer [0] == '-') {
	    bufscan1 = 1;
	    blockval = -1;
	} else {
	    bufscan1 = n;
	    n = 0;
	    while (buffer [n] != ']')
		n++;
	    buffer [n + 1] = '\0';
	}
	while (buffer [bufscan1] && isspace ((unsigned char) buffer [bufscan1]))
	    bufscan1++;
	encodespace (bufscan1);
	bufscan2 = bufscan1;
	in_head = in_ignore = FALSE;
	state_current = startstate;
	n_pre = 0;
	tokenstart = bufscan2;
	n_tokbuf = 0;
	while (buffer [bufscan2]) {
	    if (erred)
		break;
	    found = -1;
	    l = 0;
	    k = offsets [(unsigned char) buffer [bufscan2] ]; /* SPACE REQUIRED FOR FLEX/M4 */
	    if (k >= 0)
		for ( ; k < n_strings && buffer [bufscan2] == strings [k].s [0]; k++)
		    if ((strings [k].len > l) &&
			(! memcmp (buffer + bufscan2, strings [k].s, strings [k].len * sizeof (char))) &&
			checkstate (k)
		    ) {
			found = k;
			l = strings [k].len;
		    }
	    if (found < 0) {
		j = strlen (buffer);
		while (j > 0 && isspace ((unsigned char) buffer [j - 1]))
		    buffer [--j] = '\0';
		if (buffer [bufscan2] == '\0')
		    errit (errorstop, "Illegal end of text in state %i: %s", state_current, escape ());
		else
		    errit (errorstop, "Illegal token in state %i: %s", state_current, escape ());
		break;
	    }

	    if (strings [found].type == s_HEAD) {
		if (in_head)
		    tokflush (in_ignore);

		if (n_pre && buffer [bufscan2] == '\n') {
		    j = strlen (buffer);
		    while (j > 0 && isspace ((unsigned char) buffer [j - 1]))
			buffer [--j] = '\0';
		    errit (errorstop, "Trailing pre-modifiers in state %i: %s", state_current, escape ());
		    break;
		}

		in_head = TRUE;
		in_ignore = strings [found].ignore;
		if (! in_ignore) {
		    /* initialize */
		    for (i = 0; i < n_features; i++)
			scratch [i].defined = FALSE;
		    scratch [weight_index].defined = TRUE;
		    scratch [weight_index].val.n = 1.0;
		    state_new = state_current;
		    run_actions (scratch, strings [found].n_actions, strings [found].actions, s_HEAD);
		    state_current = state_new;
		}
	    } else if (strings [found].type == s_MOD) {
		if (! in_head) {
		    j = strlen (buffer);
		    while (j > 0 && isspace ((unsigned char) buffer [j - 1]))
			buffer [--j] = '\0';
		    errit (errorstop, "Modifiers before head in state %i: %s", state_current, escape ());
		    break;
		}
		if ((! in_ignore) && ! strings [found].ignore) {
		    state_new = state_current;
		    run_actions (scratch, strings [found].n_actions, strings [found].actions, s_MOD);
		    state_current = state_new;
		}
	    } else { /* PRE */
		if (in_head) {
		    tokflush (in_ignore);
		    in_head = in_ignore = FALSE;
		    n_pre = 0;
		}
		if (! strings [found].ignore) {
		    if (n_pre == max_pre) {
			max_pre += 256;
			pre = (PRE_ *) s_realloc (pre, max_pre * sizeof (PRE_));
		    }
		    pre [n_pre].string = found;
		    pre [n_pre++].state = state_current;
		    state_new = state_current;
		    run_pre_states (strings [found].n_actions, strings [found].actions);
		    state_current = state_new;
		}
	    }
	    bufscan2 += strings [found].len;
	}
	if (erred)
	    continue;
	if (in_head)
	    tokflush (in_ignore);
	else if (n_pre) {
	    j = strlen (buffer);
	    while (j > 0 && isspace ((unsigned char) buffer [j - 1]))
		buffer [--j] = '\0';
	    errit (errorstop, "Trailing pre-modifiers in state %i: %s", state_current, escape ());
	    continue;
	}
	if (n_outbuf > 0) {
	    if (blockval >= 0)
	        fprintf (fpout, "[%i]", blockval);
	    fputs ("+", fpout);
	    for (i = 0; i < n_outbuf; i++)
		fprintf (fpout, " %li", outbuf [i]);
	    fputs ("\n", fpout);
	    if (tokenized) {
		if (blockval >= 0)
		    fprintf (fpout2, "[%i]", blockval);
		fputs (">", fpout2);
		for (i = 0; i < n_tokbuf; i++)
		    fprintf (fpout2, "\t%s", tokbuf_str [i]);
		fputs ("\n", fpout2);
	    }
	}

	if (logtokens != NULL) {
	    for (i = 0; i < n_tokbuf; i++) {
		tok = tokbuf_tok [i];
		for (j = 0; j < tok->n_strings; j++)
		    if (tok->string [j] == tokbuf_str [i])
			break;
		if (j == tok->n_strings) {
		    if (tok->n_strings == tok->max_strings) {
			tok->max_strings += 16;
			tok->string = (char **) s_realloc (tok->string, tok->max_strings * sizeof (char *));
			tok->string_count = (int *) s_realloc (tok->string_count, tok->max_strings * sizeof (int));
		    }
		    tok->string_count [tok->n_strings] = 0;
		    tok->string [tok->n_strings++] = tokbuf_str [i];
		}
		tok->string_count [j]++;
	    }
	}

    }
    if (tokenized)
	fclose (fpout2);
    fclose (fpout);
    fclose (fpin);
    lineno = 0;
}

TOKEN_ *newtoken ()
{
    int
	i;
    TOKEN_
	*tmp;

    tmp = (TOKEN_ *) s_malloc (sizeof (TOKEN_));
    tmp->i = ++n_tokens;
    tmp->values = (VALUE_ *) s_malloc (n_features * sizeof (VALUE_));
    for (i = 0; i < n_features; i++)
	tmp->values [i] = scratch [i];
    tmp->string_count = NULL;
    tmp->n_strings = tmp->max_strings = 0;
    tmp->string = NULL;
    tmp->left = tmp->right = NULL;
    return tmp;
}

int cmptoken (VALUE_ *p1, VALUE_ *p2)
{
    int
	i,
	n;
    float
	f;

    for (i = 0; i < n_features; i++) {
	if (p1 [i].defined) {
	    if (p2 [i].defined) {
		n = 0;
		switch (features [i].type) {
		    case f_BITMAP:
			n = p1 [i].val.b - p2 [i].val.d;
			break;
		    case f_NUMERIC:
			f = p1 [i].val.n - p2 [i].val.n;
			if (f < 0.0)
			    n = -1;
			else if (f > 0.0)
			    n = 1;
			break;
		    case f_DISCRETE:
			n = p1 [i].val.d - p2 [i].val.d;
			break;
		}
		if (n)
		    return n;
	    } else {
		return 1;
	    }
	} else {
	    if (p2 [i].defined) {
		return -1;
	    }
	}
    }
    return 0;
}

TOKENSTRING_ *newtokenstring (char *s)
{
    TOKENSTRING_
	*t;

    t = (TOKENSTRING_ *) s_malloc (sizeof (TOKENSTRING_));
    t->s = s_strdup (s);
    t->left = t->right = NULL;
    return t;
}

char *storetokenstring (char *s)
{
    int
	n;
    TOKENSTRING_
	*tmp;

    if (rootstring == NULL) {
	rootstring = newtokenstring(s);
	return rootstring->s;
    }

    tmp = rootstring;
    for (;;) {
	n = strcmp (s, tmp->s);
	if (n == 0)
	    break;
	if (n < 0) {
	    if (tmp->left == NULL) {
		tmp = tmp->left = newtokenstring(s);
		break;
	    } else
		tmp = tmp->left;
	} else {
	    if (tmp->right == NULL) {
		tmp = tmp->right = newtokenstring(s);
		break;
	    } else
		tmp = tmp->right;
	}
    }
    return tmp->s;
}

void tokflush (BOOL_ in_ignore)
{
    char
	c,
	*s;
    int
	n,
	old_state;
    TOKEN_
	*tmp;

    if (in_ignore) {
	n_pre = 0;
	tokenstart = bufscan2;
	return;
    }

    if (logtokens != NULL || tokenized) {
	c = buffer [bufscan2];
	buffer [bufscan2] = '\0';
	s = storetokenstring (buffer + tokenstart);
	buffer [bufscan2] = c;
	tokenstart = bufscan2;
	if (n_tokbuf == max_tokbuf) {
	    max_tokbuf += 128;
	    tokbuf_str = (char   **) s_realloc (tokbuf_str, max_tokbuf * sizeof (char   *));
	    tokbuf_tok = (TOKEN_ **) s_realloc (tokbuf_tok, max_tokbuf * sizeof (TOKEN_ *));
	}
	tokbuf_str [n_tokbuf] = s;
    }

    old_state = state_current;
    for (n = n_pre - 1; n >= 0; n--) {
	state_current = pre [n].state;
	run_actions (scratch, strings [pre [n].string].n_actions, strings [pre [n].string].actions, s_PRE);
    }
    n_pre = 0;
    state_current = old_state;

    if (n_tokens == 0) {
	root = newtoken ();
	outbuf [n_outbuf++] = root->i;
	if (logtokens != NULL || tokenized)
	    tokbuf_tok [n_tokbuf++] = root;
	return;
    }
    tmp = root;
    for (;;) {
	n = cmptoken (scratch, tmp->values);
	if (n == 0)
	    break;
	if (n < 0) {
	    if (tmp->left == NULL) {
		tmp = tmp->left = newtoken ();
		break;
	    } else
		tmp = tmp->left;
	} else {
	    if (tmp->right == NULL) {
		tmp = tmp->right = newtoken ();
		break;
	    } else
		tmp = tmp->right;
	}
    }
    outbuf [n_outbuf++] = tmp->i;
    if (logtokens != NULL || tokenized)
	tokbuf_tok [n_tokbuf++] = tmp;
}

void modcheck (VALUE_ *values, int i)
{
    if (! values [i].defined)
	errit (errorstop, "Modifying undefined value (%s) in state %i: %s", features [i].name, state_current, escape ());
}

void defcheck (VALUE_ *values, int i)
{
    if (! values [i].defined)
	warning ("Defining value (%s) not defined by head in state %i: %s", features [i].name, state_current, escape ());
}

BOOL_ state_match (st_TYPE st_type, int st_val)
{
    BOOL_
	retval;

    retval = FALSE;

    switch (st_type) {
        case st_ANY:
	    retval = TRUE;
	    break;
        case st_IS:
	    retval = (state_current == st_val) ? TRUE : FALSE;
	    break;
        case st_ISNOT:
	    retval = (state_current == st_val) ? FALSE : TRUE;
	    break;
        case st_AND:
	    retval = (state_current & st_val) ? TRUE : FALSE;
	    break;
        case st_ANDNOT:
	    retval = (state_current & st_val) ? FALSE : TRUE;
	    break;
    }
    return retval;
}

void run_actions (VALUE_ *values, int n_actions, ACTION_* actions, s_TYPE s_type)
{
    int
	a,
	v;
    for (a = 0; a < n_actions; a++)
	if (state_match (actions [a].st_type, actions [a].st_val)) {
	    v = actions [a].feature;
	    switch (actions [a].type) {
	        case a_TEMPLATE:
		    run_actions (values, templates [actions [a].v.t].n_actions, templates [actions [a].v.t].actions, s_type);
		    break;
	        case a_dDEF:
		    if (s_type != s_HEAD)
			defcheck (values, actions [a].feature);
		    values [v].defined = TRUE;
		    values [v].val.d = actions [a].v.d;
		    break;
	        case a_nDEF:
		    if (s_type != s_HEAD)
			defcheck (values, actions [a].feature);
		    values [v].defined = TRUE;
		    values [v].val.n = actions [a].v.n;
		    break;
	        case a_nADD:
		    modcheck (values, actions [a].feature);
		    values [v].val.n += actions [a].v.n;
		    break;
	        case a_nSUB:
		    modcheck (values, actions [a].feature);
		    values [v].val.n -= actions [a].v.n;
		    break;
	        case a_nMUL:
		    modcheck (values, actions [a].feature);
		    values [v].val.n *= actions [a].v.n;
		    break;
	        case a_bDEF:
		    if (actions [a].feature == state_index)
			state_new = actions [a].v.b;
		    else {
			if (s_type != s_HEAD)
			    defcheck (values, actions [a].feature);
			values [v].defined = TRUE;
			values [v].val.b = actions [a].v.b;
		    }
		    break;
	        case a_bSET:
		    if (actions [a].feature == state_index)
			state_new |= actions [a].v.b;
		    else {
			modcheck (values, actions [a].feature);
			values [v].val.b |= actions [a].v.b;
		    }
		    break;
	        case a_bUNSET:
		    if (actions [a].feature == state_index)
			state_new ^= (actions [a].v.b & state_new);
		    else {
			modcheck (values, actions [a].feature);
			values [v].val.b ^= (actions [a].v.b & values [v].val.b);
		    }
		    break;
	        case a_bFLIP:
		    if (actions [a].feature == state_index)
			state_new ^= actions [a].v.b;
		    else {
			modcheck (values, actions [a].feature);
			values [v].val.b ^= actions [a].v.b;
		    }
		    break;
	        case a_UNDEF:
		    values [v].defined = FALSE;
		    break;
	    }
	}
}

void run_pre_states (int n_actions, ACTION_* actions)
{
    int
	a;

    for (a = 0; a < n_actions; a++)
	if (state_match (actions [a].st_type, actions [a].st_val))
	    switch (actions [a].type) {
	    	case a_TEMPLATE:
	    	    run_pre_states (templates [actions [a].v.t].n_actions, templates [actions [a].v.t].actions);
	    	    break;
	    	case a_dDEF:
	    	case a_nDEF:
	    	case a_nADD:
	    	case a_nSUB:
	    	case a_nMUL:
	    	case a_UNDEF:
	    	    break;
	    	case a_bDEF:
	    	    if (actions [a].feature == state_index)
	    		state_new = actions [a].v.b;
	    	    break;
	    	case a_bSET:
	    	    if (actions [a].feature == state_index)
	    		state_new |= actions [a].v.b;
	    	    break;
	    	case a_bUNSET:
	    	    if (actions [a].feature == state_index)
	    		state_new ^= (actions [a].v.b & state_new);
	    	    break;
	    	case a_bFLIP:
	    	    if (actions [a].feature == state_index)
	    		state_new ^= actions [a].v.b;
	    	    break;
	    }
}

void info ()
{
    char
	c;

    printf ("Features with weights:\n");
    for (i = 0; i < n_features; i++) {
	if (features [i].type == f_BITMAP  ) printf ("BIT");
	if (features [i].type == f_NUMERIC ) printf ("NUM");
	if (features [i].type == f_DISCRETE) printf ("DIS");
	printf ("\t%s\t%g\n", features [i].name, features [i].weight);
    }
    printf ("================\nTemplates:\n");
    for (i = 0; i < n_templates; i++) {
	printf ("%s\n", templates [i].name);
	for (j = 0; j < templates [i].n_actions; j++) {
	    switch (templates [i].actions [j].st_type) {
		case st_ANY:
		    break;
		case st_IS:
		    printf ("  = %i", templates [i].actions [j].st_val);
		    break;
		case st_ISNOT:
		    printf (" ^= %i", templates [i].actions [j].st_val);
		    break;
		case st_AND:
		    printf ("  : %i", templates [i].actions [j].st_val);
		    break;
		case st_ANDNOT:
		    printf (" ^: %i", templates [i].actions [j].st_val);
		    break;
	    }
	    switch (templates [i].actions [j].type) {
		case a_TEMPLATE:
		    printf ("\tTEMPL %s\n", templates [templates [i].actions [j].v.t].name);
		   break;
	    	case a_dDEF:
		    printf ("\tDIS %s = %i\n", features [templates [i].actions [j].feature].name, templates [i].actions [j].v.d);
		    break;
	    	case a_nDEF:
		    printf ("\tNUM %s = %g\n", features [templates [i].actions [j].feature].name, templates [i].actions [j].v.n);
		    break;
	    	case a_nADD:
		    printf ("\tNUM %s + %g\n", features [templates [i].actions [j].feature].name, templates [i].actions [j].v.n);
		    break;
	    	case a_nSUB:
		    printf ("\tNUM %s - %g\n", features [templates [i].actions [j].feature].name, templates [i].actions [j].v.n);
		    break;
	    	case a_nMUL:
		    printf ("\tNUM %s * %g\n", features [templates [i].actions [j].feature].name, templates [i].actions [j].v.n);
		    break;
	    	case a_bDEF:
		    printf ("\tBIT %s = %i\n", features [templates [i].actions [j].feature].name, templates [i].actions [j].v.b);
		    break;
	    	case a_bSET:
		    printf ("\tBIT %s + %i\n", features [templates [i].actions [j].feature].name, templates [i].actions [j].v.b);
		    break;
	    	case a_bUNSET:
		    printf ("\tBIT %s - %i\n", features [templates [i].actions [j].feature].name, templates [i].actions [j].v.b);
		    break;
	    	case a_bFLIP:
		    printf ("\tBIT %s ! %i\n", features [templates [i].actions [j].feature].name, templates [i].actions [j].v.b);
		    break;
	    	case a_UNDEF:
		    printf ("\tUNDEF %s\n", features [templates [i].actions [j].feature].name);
		    break;
	    }
	}
    }

    printf ("================\n");
    if (indelvalues == NULL) {
	printf ("Indel value: %g\n", indelmax);
    } else {
	printf ("INDEL features with values:\n");
	for (i = 0; i < n_features; i++)
	    if (indelvalues [i].defined) {
		if (features [i].type == f_BITMAP) printf ("BIT %i", indelvalues [i].val.b);
		if (features [i].type == f_NUMERIC) printf ("NUM %g", indelvalues [i].val.n);
		if (features [i].type == f_DISCRETE) printf ("DIS %i", indelvalues [i].val.d);
		printf ("\t%s\n", features [i].name);
	    }
    }

    c = ' ';
    printf ("================\nInput tokens:\n");
    for (i = 0; i < n_strings; i++) {
	switch (strings [i].state_type) {
	    case st_ANY:
		break;
	    case st_IS:
		printf ("  = %i\t", strings [i].state_val);
		break;
	    case st_ISNOT:
		printf (" ^= %i\t", strings [i].state_val);
		break;
	    case st_AND:
		printf ("  : %i\t", strings [i].state_val);
		break;
	    case st_ANDNOT:
		printf (" ^: %i\t", strings [i].state_val);
		break;
	}
	switch (strings [i].type) {
	    case s_HEAD: c = 'H'; break;
	    case s_MOD : c = 'M'; break;
	    case s_PRE : c = 'P'; break;
	}
	printf ("%c%c  %s\n", c, strings [i].ignore ? 'I' : ' ', strings [i].s);
	for (j = 0; j < strings [i].n_actions; j++) {
	    switch (strings [i].actions [j].st_type) {
		case st_ANY:
		    break;
		case st_IS:
		    printf ("  = %i", strings [i].actions [j].st_val);
		    break;
		case st_ISNOT:
		    printf (" ^= %i", strings [i].actions [j].st_val);
		    break;
		case st_AND:
		    printf ("  : %i", strings [i].actions [j].st_val);
		    break;
		case st_ANDNOT:
		    printf (" ^: %i", strings [i].actions [j].st_val);
		    break;
	    }
	    switch (strings [i].actions [j].type) {
		case a_TEMPLATE:
		    printf ("\tTEMPL %s\n", templates [strings [i].actions [j].v.t].name);
		   break;
	    	case a_dDEF:
		    printf ("\tDIS %s = %i\n", features [strings [i].actions [j].feature].name, strings [i].actions [j].v.d);
		    break;
	    	case a_nDEF:
		    printf ("\tNUM %s = %g\n", features [strings [i].actions [j].feature].name, strings [i].actions [j].v.n);
		    break;
	    	case a_nADD:
		    printf ("\tNUM %s + %g\n", features [strings [i].actions [j].feature].name, strings [i].actions [j].v.n);
		    break;
	    	case a_nSUB:
		    printf ("\tNUM %s - %g\n", features [strings [i].actions [j].feature].name, strings [i].actions [j].v.n);
		    break;
	    	case a_nMUL:
		    printf ("\tNUM %s * %g\n", features [strings [i].actions [j].feature].name, strings [i].actions [j].v.n);
		    break;
	    	case a_bDEF:
		    printf ("\tBIT %s = %i\n", features [strings [i].actions [j].feature].name, strings [i].actions [j].v.b);
		    break;
	    	case a_bSET:
		    printf ("\tBIT %s + %i\n", features [strings [i].actions [j].feature].name, strings [i].actions [j].v.b);
		    break;
	    	case a_bUNSET:
		    printf ("\tBIT %s - %i\n", features [strings [i].actions [j].feature].name, strings [i].actions [j].v.b);
		    break;
	    	case a_bFLIP:
		    printf ("\tBIT %s ! %i\n", features [strings [i].actions [j].feature].name, strings [i].actions [j].v.b);
		    break;
	    	case a_UNDEF:
		    printf ("\tUNDEF %s\n", features [strings [i].actions [j].feature].name);
		    break;
	    }
	}
    }

}


void instr ()
{
    if (n_strings == 0)
	errit (errorstop, "Setting blind features");
}

void init_string (char const *s, char st, BOOL_ ignore)
{
    s_TYPE
	t;

    if (n_strings == max_strings) {
	max_strings += 256;
	strings = (STRING_ *) s_realloc (strings, max_strings * sizeof (STRING_));
    }

    t = s_HEAD;
    switch (st) {
	case 'H': t = s_HEAD; break;
	case 'M': t = s_MOD;  break;
	case 'P': t = s_PRE;  break;
    }
    strings [n_strings].s = s_strdup (s);
    strings [n_strings].len = strlen (s);
    strings [n_strings].type = t;
    strings [n_strings].ignore = ignore;
    strings [n_strings].actions = NULL;
    strings [n_strings].n_actions = 0;
    if (state_type == st_ANY)
	strings [n_strings].state_type = st_ANY;
    else {
	strings [n_strings].state_type = state_type;
	strings [n_strings].state_val = state_val;
	state_type = st_ANY;
    }
    n_strings++;
}

int cmpfeature (void const *p1, void const *p2)
{
    return strcmp (((FEATURE_ *) p1)->name, ((FEATURE_ *) p2)->name);
}

int cmptemplate (void const *p1, void const *p2)
{
    return strcmp (((TEMPLATE_ *) p1)->name, ((TEMPLATE_ *) p2)->name);
}

int cmpstring (void const *p1, void const *p2)
{
    STRING_
	*s1,
	*s2;
    int
	i;

    s1 = (STRING_ *) p1;
    s2 = (STRING_ *) p2;

    i = strcmp (s1->s, s2->s);

    if (i)
	return i;

    /* zonder voorwaarden altijd groter dan met voorwaarden */
    if (s1->state_type == st_ANY)
        return 1;
    if (s2->state_type == st_ANY)
        return -1;

    i = ((int) s1->state_type) - (int) s2->state_type;
    if (i)
	return i;

    return s1->state_val - s2->state_val;

}

int scanfeature (void const *key, void const *p)
{
    return strcmp ((char *) key, ((FEATURE_ *) p)->name);
}

int scantemplate (void const *key, void const *p)
{
    return strcmp ((char *) key, ((TEMPLATE_ *) p)->name);
}

int scanstring (void const *key, void const *p)
{
    return strcmp ((char *) key, ((STRING_ *) p)->s);
}

void isdefined (VALUE_ v)
{
    if (! v.defined)
	errit (errorstop, "Operation on undefined value");
}

VALUE_ *newvalues ()
{
    int
	i;
    VALUE_
	*t;

    t = (VALUE_ *) s_malloc (n_features * sizeof (VALUE_));
    for (i = 0; i < n_features; i++)
	t [i].defined = FALSE;
    t [weight_index].defined = TRUE;
    t [weight_index].val.n = 1.0;
    return t;
}

void add_feature (char c, char const *s, float f, float max)
{
    int
	i;
    f_TYPE
	type;

    for (i = 0; i < n_features; i++)
	if (! strcmp (s, features [i].name))
	    errit (e_STOP, "Feature \"%s\" already defined", s);

    if (f <= 0.0)
	errit (e_STOP, "Weight must be greater than 0");

    if (max < 0.0)
	errit (e_STOP, "Default difference must be greater or equal than 0");

    type = f_NUMERIC;
    switch (c) {
	case 'B': type = f_BITMAP;   break;
	case 'N': type = f_NUMERIC;  break;
	case 'D': type = f_DISCRETE; break;
    }

    if (n_features == max_features) {
	max_features += 64;
	features = (FEATURE_ *) s_realloc (features, max_features * sizeof (FEATURE_ ));
    }
    features [n_features].type = type;
    features [n_features].name = s_strdup (s);
    features [n_features].weight = f;
    features [n_features].max = max;
    n_features++;
}

int get_feature (char const *s)
{
    FEATURE_
	*tmp;

    tmp = (FEATURE_ *) bsearch (s, features, n_features, sizeof (FEATURE_), scanfeature);
    if (tmp == NULL)
	errit (e_STOP, "Feature \"%s\" undefined", s);
    return (tmp - features);
}

int get_template (char const *s)
{
    TEMPLATE_
	*tmp;

    tmp = (TEMPLATE_ *) bsearch (s, templates, n_templates, sizeof (TEMPLATE_), scantemplate);
    if (tmp == NULL)
	errit (e_STOP, "Template \"%s\" undefined", s);
    return (tmp - templates);
}

int get_int (char const *s, char const *format)
{
    int
	i;

    if (sscanf (s, format, &i) < 1)
	errit (e_STOP, "No integer value found");
    return i;
}

float get_float (char const *s, char const *format)
{
    float
	f;

    if (sscanf (s, format, &f) < 1)
	errit (e_STOP, "No float value found");
    return f;
}

char *get_string (char const *s, char const *format)
{
    if (sscanf (s, format, buffer) < 1)
	errit (e_STOP, "No string found");
    return buffer;
}

int escape0 (int i)
{
    if (ansi) {
	strcpy (bufesc + i, "\033[0m");
	i += 4;
    }
    return i;
}

int escape1 (int i)
{
    if (ansi) {
	strcpy (bufesc + i, "\033[31;1m");
	i += 7;
    } else {
	bufesc [i++] = ' ';
	bufesc [i++] = ' ';
    }
    return i;
}

char *escape ()
{
    int
	i,
	j,
	n;

    if (errorbase == 0) {
	j = 0;
	for (i = bufscan1; buffer [i]; i++) {
	    if (i == bufscan2)
		j = escape1 (j);
	    bufesc [j++] = buffer [i];
	}
	if (i == bufscan2)
	    j = escape1 (j);
	j = escape0 (j);
	bufesc [j] = '\0';
	return bufesc;
    }

    j = 0;
    for (i = bufscan1; buffer [i]; i++) {

	if (i == bufscan2)
	    j = escape1 (j);
	
	n = (unsigned char) buffer [i];

	if (n > 32 && n < 127 && buffer [i] != '\\') {
	    bufesc [j++] = buffer [i];
	    continue;
	}

	bufesc [j++] = '\\';
	switch (errorbase) {
	    case 8:
		sprintf (bufesc + j, "%03o", n);
		j += 3;
		break;
	    case 10:
		bufesc [j++] = 'd';
		sprintf (bufesc + j, "%03d", n);
		j += 3;
		break;
	    case 16:
		bufesc [j++] = 'x';
		sprintf (bufesc + j, "%02X", n);
		j += 2;
		break;
	}

    }
    if (i == bufscan2)
	j = escape1 (j);
    j = escape0 (j);
    bufesc [j] = '\0';
    return bufesc;

}

char *escape2 (char *s)
{
    int
	i,
	j,
	n;

    if (errorbase == 0)
	return s;

    j = 0;
    for (i = 0; s [i]; i++) {

	n = (unsigned char) s [i];

	if (n > 32 && n < 127 && s [i] != '\\') {
	    bufesc [j++] = s [i];
	    continue;
	}

	bufesc [j++] = '\\';
	switch (errorbase) {
	    case 8:
		sprintf (bufesc + j, "%03o", n);
		j += 3;
		break;
	    case 10:
		bufesc [j++] = 'd';
		sprintf (bufesc + j, "%03d", n);
		j += 3;
		break;
	    case 16:
		bufesc [j++] = 'x';
		sprintf (bufesc + j, "%02X", n);
		j += 2;
		break;
	}

    }
    bufesc [j] = '\0';
    return bufesc;

}

char *unescape (char *s)
{
    char
	tmp [4];
    unsigned char
	uc;
    int
	i,
	j,
	d,
	si;
    unsigned int
	u;
    BOOL_
	err;

    if (! tokenesc)
	return s;

    tmp [3] = '\0';

    for (i = j = 0; s [i]; i++) {

	if (s [i] != '\\') {
	    bufesc [j++] = s [i];
	    continue;
	}

	i++;

	if (s [i] == '\\') {
	    bufesc [j++] = '\\';
	    continue;
	}

	err = FALSE;
	si = u = d = 0;
	if (s [i] == 'd' || s [i] == 'D') {
	    if (isdigit ((unsigned char) s [i + 1]) &&
		isdigit ((unsigned char) s [i + 2]) &&
		isdigit ((unsigned char) s [i + 3])
	    ) {
		memcpy (tmp, s + i + 1, 3);
		sscanf (tmp, "%d", &si);
		u = si;
		d = 3;
	    } else
		err = TRUE;
	} else if (s [i] == 'x' || s [i] == 'X') {
	    if (isxdigit ((unsigned char) s [i + 1]) &&
		isxdigit ((unsigned char) s [i + 2])
	    ) {
		memcpy (tmp, s + i + 1, 2);
		tmp [2] = '\0';
		sscanf (tmp, "%x", &u);
		d = 2;
	    } else
		err = TRUE;
	} else if ((s [i    ] >= '0' && s [i    ] <= '7') &&
		   (s [i + 1] >= '0' && s [i + 1] <= '7') &&
		   (s [i + 2] >= '0' && s [i + 2] <= '7')
	) {
	    memcpy (tmp, s + i, 3);
	    sscanf (tmp, "%o", &u);
	    d = 2;
	} else
	    err = TRUE;

	if (err)
	    errit (e_STOP, "Illegal escape sequence: %s", s + i - 1);
	if (u < 1 || u > 255)
	    errit (e_STOP, "Escape sequence out of range: %s", s + i - 1);

	uc = (unsigned char) u;

	if (isspace (uc))
	    errit (e_STOP, "Illegal escape sequence (white space): %s", s + i - 1);

	bufesc [j++] = uc;
	i += d;
    }

    bufesc [j] = '\0';

    return bufesc;
}

void encodespace (int offset)
{
    int
	i,
	j;
    BOOL_
	found;

    found = FALSE;
    j = 0;
    for (i = offset; buffer [i]; i++) {
	if (buffer [i] == ' ') {
	    strcpy (buffr2 + j, "[" "[SP]" "]");  /* SPACE REQUIRED FOR FLEX/M4 */
	    j += 6;
	    found = TRUE;
	} else {
	    buffr2 [j++] = buffer [i];
	}
    }
    if (found) {
	buffr2 [j] = '\0';
	strcpy (buffer + offset, buffr2);
    }
}


void process_args ()
{
    while (arg_c > 1 && arg_v [1][0] == '-') {
        switch (arg_v [1][1]) {
	    case 'a':
	        ansi = TRUE;
		break;
            case 'c':
                errorstop = e_GO;
                break;
            case 'C':
                errorstop = e_COUNTDOWN;
		errorcount = atoi (get_arg ());
                break;
	    case 'd':
	        errorbase = 10;
		break;
	    case 'e':
		logerrors = get_arg ();
		break;
	    case 'f':
		logfeatures = get_arg ();
		break;
	    case 'g':
		toINT = FALSE;
		break;
	    case 'l':
	        showlabels = TRUE;
		break;
	    case 'o':
	        errorbase = 8;
		break;
	    case 't':
		logtokens = get_arg ();
		break;
	    case 'T':
		tokenized = TRUE;
		break;
	    case 'x':
	        errorbase = 16;
		break;
            default:
                errit (e_STOP, "Illegal option '%s'", arg_v [1]);
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
        errit (e_STOP, "Missing argument for '%s'", arg_v [1]);

    arg_v++;
    arg_c--;
    return arg_v [1];
}

void mexit (int retval)
{
    if (fperrors) {
	printf ("Errors written to file \"%s\"\n", logerrors);
	fclose (fperrors);
    }
    exit (retval);
}

void errit (es_TYPE stop, char const *format, ...)
{
    int
	i;
    va_list
	list;

    if (showlabels) {
	for (i = strlen (currentlabel); i > 0; i--)
	    if (isspace ((unsigned char) currentlabel [i - 1]))
		currentlabel [i - 1] = '\0';
	    else
		break;
    }

    if (lineno)
	emessage ("%s:%li:%s", infile, lineno, prefix);
    else
	emessage ("\nError %s: ", programname);

    va_start (list, format);
    vfprintf (stderr, format, list);
    if (showlabels)
	fprintf (stderr, "  %s", currentlabel);
    fprintf (stderr, lineno ? "\n" : "\n\n");

    if (fperrors) {
	va_start (list, format);
	vfprintf (fperrors, format, list);
	if (showlabels)
	    fprintf (fperrors, "  %s", currentlabel);
	fprintf (fperrors, lineno ? "\n" : "\n\n");
    }

    if (stop == e_STOP)
	mexit (1);

    erred = TRUE;
    errsum++;

    if (stop == e_GO)
	return;

    if (--errorcount <= 0) {
	emessage ("Too many errors, STOP\n\n");
	mexit (1);
    }
}

void emessage (char const *format, ...)
{
    va_list
	list;

    va_start (list, format);
    vfprintf (stderr, format, list);
    if (fperrors) {
	va_start (list, format);
	vfprintf (fperrors, format, list);
    }
}

void warning (char const *format, ...)
{
    va_list
	list;

    emessage ("%s:%li: warning: ", infile, lineno);

    va_start (list, format);
    vfprintf (stderr, format, list);
    if (fperrors) {
	va_start (list, format);
	vfprintf (fperrors, format, list);
    }

    emessage ("\n");
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
	errit (e_STOP, out_of_memory);
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
	errit (e_STOP, out_of_memory);
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
	"Usage: %s [-a] [-c] [-C int] [-d|-o|-x] [-g] [-l] [-T] [-e filename] [-f filename] [-t filename] configfile file(s).fon\n"
	"\n"
	"\t-a : use ANSI escape sequences for error messages\n"
	"\t-c : continue on data error\n"
	"\t-C : continue on data error, stop after this many errors\n"
	"\t-d : decimal codes in token list (option -t) and error messages\n"
	"\t-e : save to file: error log\n"
	"\t-f : save to file: list of all feature sets\n"
	"\t-g : don't map reals to integers\n"
	"\t-l : show labels in error messages\n"
	"\t-o : octal codes in token list (option -t) and error messages\n"
	"\t-t : save to file: list of all token strings\n"
	"\t-T : save tokenized files\n"
	"\t-x : hexadecimal codes in token list (option -t) and error messages\n"
	"\n",
	programname
    );
    exit (1);
}
