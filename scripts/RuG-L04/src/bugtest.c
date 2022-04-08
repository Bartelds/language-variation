/*
 * File: bugtest.c
 *
 * (c) Peter Kleiweg
 *     Thu Jan 26 00:32:30 2006,
 *         2007
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2,
 * or (at your option) any later version.
 *
 */

#define my_VERSION "0.02"

#define __NO_MATH_INLINES

#include <float.h>
#include <stdio.h>

int main (int argc, char *argv [])
{
    float
	av,
	sd,
	f;
    int
	bugs = 0;

    f = FLT_MAX;

    printf ("sizeof(float):  %i\tFLT_MAX: %g\nsizeof(double): %i\tDBL_MAX: %g\nTarget f: %g\n\n",
	    (int) sizeof(float), (float) FLT_MAX,
	    (int) sizeof(double), (double) DBL_MAX,
	    f);

    for (av = 0; av < 1.05; av += 0.1)
	for (sd = 0.05; sd < 1.0; sd += 0.1) {
	    f = FLT_MAX;
	    f = (f - av) / sd;
	    printf ("av: %g\t\tsd: %g\tf: %g\t\t%s\n", av, sd, f, (f < FLT_MAX) ? "BUG" : "safe");
	    if (f < FLT_MAX)
		bugs++;
	}

    if (bugs)
	printf ("\nBug manifested %i time%s\n\n", bugs, (bugs == 1) ? "" : "s");
    else
	printf ("\nBug did not manifest. You are probably safe.\n\n");

    return 0;
}
