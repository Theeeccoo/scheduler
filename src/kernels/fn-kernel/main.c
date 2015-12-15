/*
 * Copyright(C) 2014 Pedro H. Penna <pedrohenriquepenna@gmail.com>
 * 
 * Friendly Numbers Benchmark Kernel.
 */

#include <omp.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <mylib/util.h>

/*
 * Computes friendly numbers.
 */
extern int friendly_numbers(int start, int end);

/*
 * Problem.
 */
struct problem
{
	int start; /* Range start. */
	int end;   /* Range end.   */
};

/* Problem sizes. */
static struct problem tiny        = { 1,   65536 };
static struct problem small       = { 1,  131072 };
static struct problem workstation = { 1,  262144 };
static struct problem standard    = { 1,  524288 };
static struct problem large       = { 1, 1048576 };

/* Benchmark parameters. */
int verbose = 0;                  /* Be verbose?        */
int nthreads = 1;                 /* Number of threads. */
static struct problem *p = &tiny; /* Problem.           */

/*
 * Prints program usage and exits.
 */
static void usage(void)
{
	printf("Usage: fn [options]\n");
	printf("Brief: Friendly Numbers Benchmark Kernel\n");
	printf("Options:\n");
	printf("  --help             Display this information and exit\n");
	printf("  --nthreads <value> Set number of threads\n");
	printf("  --class <name>     Set problem class:\n");
	printf("                       - small\n");
	printf("                       - workstation\n");
	printf("                       - standard\n");
	printf("                       - large\n");
	printf("  --verbose          Be verbose\n");
	exit(0);
}

/*
 * Reads command line arguments.
 */
static void readargs(int argc, char **argv)
{
	int i;     /* Loop index.       */
	char *arg; /* Working argument. */
	int state; /* Processing state. */
	
	/* State values. */
	#define READ_ARG     0 /* Read argument.         */
	#define SET_NTHREADS 1 /* Set number of threads. */
	#define SET_CLASS    2 /* Set problem class.     */
	
	state = READ_ARG;
	
	/* Read command line arguments. */
	for (i = 1; i < argc; i++)
	{
		arg = argv[i];
		
		/* Set value. */
		if (state != READ_ARG)
		{
			switch (state)
			{
				/* Set problem class. */
				case SET_CLASS :
					if (!strcmp(argv[i], "tiny"))
						p = &tiny;
					else if (!strcmp(argv[i], "small"))
						p = &small;
					else if (!strcmp(argv[i], "workstation"))
						p = &workstation;
					else if (!strcmp(argv[i], "standard"))
						p = &standard;
					else if (!strcmp(argv[i], "large"))
						p = &large;
					else 
						usage();
					state = READ_ARG;
					break;
				
				/* Set number of threads. */
				case SET_NTHREADS :
					nthreads = atoi(arg);
					state = READ_ARG;
					break;
				
				default:
					usage();			
			}
			
			continue;
		}
		
		/* Parse argument. */
		if (!strcmp(arg, "--verbose"))
			verbose = 1;
		else if (!strcmp(arg, "--nthreads"))
			state = SET_NTHREADS;
		else if (!strcmp(arg, "--class"))
			state = SET_CLASS;
		else
			usage();
	}
	
	/* Invalid argument(s). */
	if (nthreads < 1)
		usage();
}

/*
 * Runs benchmark.
 */
int main(int argc, char **argv)
{
	uint64_t end;   /* End time.   */
	uint64_t start; /* Start time. */
	
	readargs(argc, argv);
	
	omp_set_num_threads(nthreads);
	
	/* Compute friendly numbers. */
	if (verbose)
		printf("computing friendly numbers...\n");
	start = timer_get();
	friendly_numbers(p->start, p->end);
	end = timer_get();
	if (verbose)
		printf("  time spent: ");
	printf("%lf\n", (end - start)/1000.0);
	
	return (0);
}
