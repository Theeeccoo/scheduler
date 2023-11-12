/*
 * Copyright(C) 2016 Pedro H. Penna <pedrohenriquepenna@gmail.com>
 * 
 * This file is part of Scheduler.
 *
 * Scheduler is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 * 
 * Scheduler is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with Scheduler; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <unistd.h>

#include <mylib/util.h>

#include <scheduler.h>
#include <workload.h>

/**
 * @name Program Parameters
 */
static struct
{
	workload_tt workload;              /**< Input workload.           */
<<<<<<< HEAD
	array_tt  threads;                 /**< Working threads.          */
	const struct scheduler *scheduler; /**< Loop scheduling strategy. */
	int chunksize;                     /**< Chunk size.               */
=======
    array_tt cores;                    /**< Cores to process tasks.   */
	const struct scheduler *scheduler; /**< Loop scheduling strategy. */
	const struct processer *processer; /**< Core processing strategy. */
	int batchsize;                     /**< Scheduling batch size.    */
>>>>>>> 7f1db94 (Removing thread structure from simulation)
	void (*kernel)(workload_tt);       /**< Application kernel.       */
} args = { NULL, NULL, NULL, 1, NULL };

/*============================================================================*
 * KERNELS                                                                    *
 *============================================================================*/

/**
 * @brief Applies a linear kernel in a workload
 *
 * @param w Target workload.
 */
static void kernel_linear(workload_tt w)
{
	for (int i = 0; i < workload_ntasks(w); i++)
	{
		int load;

		load = workload_task(w, i);
		workload_set_task(w, i, load);
	}
}

/**
 * @brief Applies a logarithmic kernel in a workload
 *
 * @param w Target workload.
 */
static void kernel_logarithmic(workload_tt w)
{
	for (int i = 0; i < workload_ntasks(w); i++)
	{
		int load;

		load = workload_task(w, i);
		load = floor(load*(log(load)/log(2.0)));
		workload_set_task(w, i, load);
	}
}

/**
 * @brief Applies a quadratic kernel in a workload
 *
 * @param w Target workload.
 */
static void kernel_quadratic(workload_tt w)
{
	for (int i = 0; i < workload_ntasks(w); i++)
	{
		int load;

		load = workload_task(w, i);
		load = load*load;
		workload_set_task(w, i, load);
	}
}

/*============================================================================*
 * ARGUMENT CHECKING                                                          *
 *============================================================================*/

/**
 * @brief Prints program usage and exits.
 */
static void usage(void)
{
	printf("Usage: simsched [options] <scheduler>\n");
	printf("Brief: loop scheduler simulator\n");
	printf("Options:\n");
<<<<<<< HEAD
	printf("  --arch <filename>     Architecture file.\n");
	printf("  --chunksize <number>  Chunk size.\n");
	printf("  --kernel <name>       Kernel complexity.\n");
	printf("           linear          Linear kernel\n");
	printf("           logarithmic     Logarithm kernel\n");
	printf("           quadratic       Quadratic kernel\n");
	printf("  --input <filename>    Input workload file\n");
	printf("  --nthreads <number>   Number of working threads.\n");
	printf("  --help                Display this message.\n");
	printf("Loop Schedulers:\n");
	printf("  guided   Guided Scheduling\n");
	printf("  dynamic  Dynamic Scheduling\n");
	printf("  hss      History-Aware Scheduling\n");
	printf("  kass     Knowledge-Based Scheduling\n");
	printf("  binlpt   Bin Packing LPT Scheduling\n");
	printf("  srr      Smart Round-Robin Scheduling\n");
	printf("  static   Static Scheduling\n");
=======
	printf("  --arch <filename>       Cores' architecture file.\n");
	printf("  --process <name>        Cores' processing strategy\n");
	printf("           non-preemptive       Non-preemptive\n");
	printf("           random-preemptive    Random preemptive.\n");
	printf("           rr-preemptive        Round-Robin Quantum = 10\n");
	printf("  --batchsize <number>    Batch size.\n");
	printf("  --kernel <name>         Kernel complexity.\n");
	printf("           linear               Linear kernel\n");
	printf("           logarithmic          Logarithm kernel\n");
	printf("           quadratic            Quadratic kernel\n");
	printf("  --input <filename>      Input workload file\n");
	printf("  --help                  Display this message.\n");
	printf("Schedulers:\n");
	// printf("  guided   Guided Scheduling\n");
	// printf("  dynamic  Dynamic Scheduling\n");
	// printf("  hss      History-Aware Scheduling\n");
	// printf("  kass     Knowledge-Based Scheduling\n");
	// printf("  binlpt   Bin Packing LPT Scheduling\n");
	// printf("  srr      Smart Round-Robin Scheduling\n");
	printf("  fcfs   First-Come, First-Served Scheduling\n");
>>>>>>> 7f1db94 (Removing thread structure from simulation)

	exit(EXIT_SUCCESS);
}

/**
 * @brief Gets workload.
 *
 * @param filename Input workload filename.
 *
 * @returns A workload.
 */
static workload_tt get_workload(const char *filename)
{
	FILE *input;   /* Input workload file. */
	workload_tt w; /* Workload.            */

	input = fopen(filename, "r");
	if (input == NULL)
		error("cannot open input workload file");

	w = workload_read(input);

	fclose(input);

	return (w);
}

/**
<<<<<<< HEAD
 * @brief Gets threads.
 *
 * @param filename Architecture filename.
 * @param nthreads Number of working threads.
 *
 * @returns Working threads.
 */
static array_tt get_threads(const char *filename, int nthreads, int ntasks)
=======
 * @brief Gets cores.
 * 
 * @param afilename        Input architecture filename.
 * @param ncores           Number of cores in architecture, will be obtained from "afilename"
 * 
 * @return Working cores. 
*/
static array_tt get_cores(const char *filename, int ncores)
>>>>>>> 7f1db94 (Removing thread structure from simulation)
{
	FILE *file;       /* Architecture file. */
	int ncores;       /* Number of cores.   */
	array_tt threads; /* Working threads.   */

	assert(nthreads > 0);

	if ((file = fopen(filename, "r")) == NULL)
		error("failed to open architecture file");

	assert(fscanf(file, "%d", &ncores) == 1);
<<<<<<< HEAD
	if (nthreads < 1)
		error("bad architecture file");

	if (nthreads > ncores)
		error("too many threads for target architecture");

	threads = array_create(nthreads);

	for (int i = 0; i < nthreads; i++)
=======
	if (ncores < 1)
		error("bad architecture file");

	cores = array_create(ncores);

	for (int i = 0; i < ncores; i++)
>>>>>>> 7f1db94 (Removing thread structure from simulation)
	{
		thread_tt t;  /* Thread.                       */
		int capacity; /* Thread's processing capacity. */
		
		assert(fscanf(file, "%d", &capacity) == 1);
<<<<<<< HEAD
		
		t = thread_create(capacity, ntasks);
		array_set(threads, i, t);
=======
		c = core_create(capacity);
		array_set(cores, i, c);
>>>>>>> 7f1db94 (Removing thread structure from simulation)
	}

	/* House keeping. */
	fclose(file);

<<<<<<< HEAD
	return (threads);
=======
	return (cores);
>>>>>>> 7f1db94 (Removing thread structure from simulation)
}

/**
 * @brief Gets application kernel.
 *
 * @param kernelname Kernel name.
 *
 * @returns Application kernel.
 */
static void (*get_kernel(const char *kernelname))(workload_tt)
{
	if (!strcmp(kernelname, "linear"))
		return (kernel_linear);
	if (!strcmp(kernelname, "logarithmic"))
		return (kernel_logarithmic);
	if (!strcmp(kernelname, "quadratic"))
		return (kernel_quadratic);

	error("unsupported application kernel");

	/* Never gets here. */
	return (NULL);
}

/**
 * @brief Checks program arguments.
 *
 * @param wfilename  Input workload filename.
 * @param afilename  Input architecture filename.
 * @param kernelname Application kernel name.
<<<<<<< HEAD
 * @param nthreads   Number of workin threads.
 */
static void checkargs(const char *wfilename, const char *afilename, const char *kernelname, int nthreads)
=======
 * @param quantum    
*/
static void checkargs(const char *wfilename, const char *afilename, const char *kernelname)
>>>>>>> 7f1db94 (Removing thread structure from simulation)
{
	if (afilename == NULL)
		error("missing architecture file");
	if (wfilename == NULL)
		error("missing input workload file");
	if (kernelname == NULL)
		error("missing kernel name");
<<<<<<< HEAD
	if (nthreads == 0)
		error("missing number of working threads");
=======
	if (args.processer == NULL)
		error("missing cores' processing strategy.");
>>>>>>> 7f1db94 (Removing thread structure from simulation)
	if (args.scheduler == NULL)
		error("missing loop scheduling strategy");
}

/**
 * @brief Reads command line arguments.
 *
 * @param argc Argument count.
 * @param argv Argument variables.
 */
static void readargs(int argc, const char **argv)
{
	const char *wfilename = NULL;
	const char *afilename = NULL;
	const char *kernelname = NULL;
<<<<<<< HEAD
	int nthreads = 0;
=======
    int ncores = 0;
>>>>>>> 7f1db94 (Removing thread structure from simulation)

	/* Parse command line arguments. */
	for (int i = 1; i < argc; i++)
	{	
		if (!strcmp(argv[i], "--arch"))
			afilename = argv[++i];
		else if (!strcmp(argv[i], "--chunksize"))
			args.chunksize = atoi(argv[++i]);
		else if (!strcmp(argv[i], "--input"))
			wfilename = argv[++i];
		else if (!strcmp(argv[i], "--kernel"))
			kernelname = argv[++i];
		else if (!strcmp(argv[i], "--help"))
			usage();
		else
		{
<<<<<<< HEAD
			if (!strcmp(argv[i], "guided"))
				args.scheduler = sched_guided;
			else if (!strcmp(argv[i], "dynamic"))
				args.scheduler = sched_dynamic;
			else if (!strcmp(argv[i], "hss"))
				args.scheduler = sched_hss;
			else if (!strcmp(argv[i], "kass"))
				args.scheduler = sched_kass;
			else if (!strcmp(argv[i], "binlpt"))
				args.scheduler = sched_binlpt;
			else if (!strcmp(argv[i], "srr"))
				args.scheduler = sched_srr;
			else if (!strcmp(argv[i], "static"))
				args.scheduler = sched_static;
			else
				error("unsupported loop scheduling strategy");
=======
			// if (!strcmp(argv[i], "guided"))
			// 	args.scheduler = sched_guided;
			// else if (!strcmp(argv[i], "dynamic"))
			// 	args.scheduler = sched_dynamic;
			// else if (!strcmp(argv[i], "hss"))
			// 	args.scheduler = sched_hss;
			// else if (!strcmp(argv[i], "kass"))
			// 	args.scheduler = sched_kass;
			// else if (!strcmp(argv[i], "binlpt"))
			// 	args.scheduler = sched_binlpt;
			// else if (!strcmp(argv[i], "srr"))
			// 	args.scheduler = sched_srr;
			// else if (!strcmp(argv[i], "static"))

            if (!strcmp(argv[i], "fcfs"))
                args.scheduler = sched_fcfs;
			else
				error("unsupported scheduling strategy");

>>>>>>> 7f1db94 (Removing thread structure from simulation)
		}
	}

	checkargs(wfilename, afilename, kernelname);

	args.workload = get_workload(wfilename);
<<<<<<< HEAD
	args.threads = get_threads(afilename, nthreads, workload_ntasks(args.workload));
=======
    args.cores = get_cores(afilename, ncores);
>>>>>>> 7f1db94 (Removing thread structure from simulation)
	args.kernel = get_kernel(kernelname);
}

/*============================================================================*
 * LOOP SCHEDULER SIMULATOR                                                   *
 *============================================================================*/

/**
 * @brief A loop scheduler simulator
 */
int main(int argc, const char **argv)
{
	readargs(argc, argv);

	args.kernel(args.workload);

	srand(time(NULL)^getpid());

<<<<<<< HEAD
	simshed(args.workload, args.threads, args.scheduler, args.chunksize);

	/* House keeping, */
	for (int i = 0; i < array_size(args.threads); i++)
	{
		thread_tt t = array_get(args.threads, i);
		thread_destroy(t);
	}
	array_destroy(args.threads);
=======
	workload_sort(args.workload, WORKLOAD_ARRIVAL);

	simsched(args.workload, args.cores, args.scheduler, args.processer, args.batchsize);

	/* House keeping, */
	for (int i = 0; i < array_size(args.cores); i++)
	{
		core_tt c = array_get(args.cores, i);
		core_destroy(c);
	}
	array_destroy(args.cores);
>>>>>>> 7f1db94 (Removing thread structure from simulation)
	workload_destroy(args.workload);

	return (EXIT_SUCCESS);
}
