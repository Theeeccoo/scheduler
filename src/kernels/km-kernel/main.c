/*
 * Copyright(C) 2014 Pedro H. Penna <pedrohenriquepenna@gmail.com>
 * 
 * Kmeans Benchmark Kernel.
 */

#include <omp.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>

#include <papi.h>

#include <mylib/util.h>

#include "vector.h"

/*
 * Clusters data. 
 */
extern int *kmeans(vector_t *_points, int _npoints, int _ncentroids, float _mindistance);

gsl_rng * r;

/*
 * Problem.
 */
struct problem
{
	int npoints;       /* Number of points.    */
	int dimension;     /* Data dimension.      */
	int ncentroids;    /* Number of centroids. */
	float mindistance; /* Minimum distance.    */
};

/* Problem sizes. */
static struct problem tiny        = {  32768, 16, 48, 0.0 };
static struct problem small       = {  65536, 16, 48, 0.0 };
static struct problem workstation = { 131072, 16, 96, 0.0 };
static struct problem standard    = { 262144, 16, 96, 0.0 };
static struct problem large       = { 524288, 16, 192, 0.0 };

/* Benchmark parameters. */
int verbose = 0;                  /* Be verbose?        */
static int seed = 0;              /* Seed value.        */
int nthreads = 1;                 /* Number of threads. */
struct problem *p = &tiny; /* Problem.           */

/*
 * Prints program usage and exits.
 */
static void usage(void)
{
	printf("Usage: kmeans [options]\n");
	printf("Brief: Kmeans Benchmark Kernel\n");
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
	int i;          /* Loop index.      */
	uint64_t end;   /* End time.        */
	uint64_t start; /* Start time.      */
	vector_t *data; /* Data points.     */
	int *map;       /* Map of clusters. */
	int events[2] = { PAPI_L1_DCM, PAPI_L2_DCM};
	long long hwcounters[2];
	const gsl_rng_type * T;
	
	/* Setup random number generator. */
	gsl_rng_env_setup();
	T = gsl_rng_default;
	r = gsl_rng_alloc(T);
	
	readargs(argc, argv);
	
	srandnum(seed);	
	omp_set_num_threads(nthreads);
	
	/* Benchmark initialization. */
	data = smalloc(p->npoints*sizeof(vector_t));
	for (i = 0; i < p->npoints; i++)
	{
		data[i] = vector_create(p->dimension);
		vector_random(data[i]);
	}
	
	/* Setup PAPI. */
	if (PAPI_start_counters(events, 2) != PAPI_OK)
	{
		fprintf(stderr, "failed to setup PAPI\n");
		exit(EXIT_FAILURE);
	}
	
	/* Cluster data. */
	start = timer_get();
	map = kmeans(data, p->npoints, p->ncentroids, p->mindistance);
	end = timer_get();
	
	/* Exit PAPI. */
	if (PAPI_stop_counters(hwcounters, sizeof(events)) != PAPI_OK)
	{
		fprintf(stderr, "failed to read hardware counters\n");
		exit(EXIT_FAILURE);
	}
	
	printf("L1 Misses: %lld\n", hwcounters[0]);
	printf("L2 Misses: %lld\n", hwcounters[1]);
	printf("Time:      %f\n", (end - start)/1000.0);
	
	/* House keeping. */
	gsl_rng_free(r);
	free(map);
	for (i = 0; i < p->npoints; i++)
		vector_destroy(data[i]);
	free(data);
	
	return (0);
}
