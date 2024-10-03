#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <unistd.h>

#include <mylib/util.h>

#include <core.h>
#include <mmu.h>
#include <process.h>
#include <scheduler.h>
#include <statistics.h>
#include <workload.h>

/**
 * @name Program Parameters
 */
static struct
{
	workload_tt workload;              /**< Input workload.                            */
    array_tt cores;                    /**< Cores to process tasks.                    */
	const struct scheduler *scheduler; /**< Loop scheduling strategy.                  */
	const struct processer *processer; /**< Core processing strategy.                  */
	int optimize;                      /**< If scheduling optimization should be done. */
	int winsize;                       /**< Memory accesses window size.               */
	int batchsize;                     /**< Scheduling batch size.                     */
	int seed;                          /**< Seed.                                      */
	void (*kernel)(workload_tt);       /**< Application kernel.                        */
} args = { NULL, NULL, NULL, NULL, -1, 0, 1, 0, NULL };


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
		task_tt task;
		int load = 0;

		task = queue_peek(workload_tasks(w),i);
		load = task_workload(task);
		task_set_workload(task, load);
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
		task_tt task;
		int load = 0;

		task = queue_peek(workload_tasks(w), i);
		load = task_workload(task);
		load = floor(load*(log(load)/log(2.0)));
		task_set_workload(task, load);
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
		task_tt task;
		int load = 0;

		task = queue_peek(workload_tasks(w),i);
		load = task_workload(task);
		load = load*load;
		task_set_workload(task, load);
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
	printf("  --arch <filename>       Cores' architecture file.\n");
	printf("  --process <name>        Cores' processing strategy.\n");
	printf("           non-preemptive       Non-preemptive.\n");
	printf("           random-preemptive    Random preemptive.\n");
	printf("           rr-preemptive        Round-Robin Quantum = 10.\n");
	printf("  --batchsize <number>    Batch size.\n");
	printf("  --kernel <name>         Kernel complexity.\n");
	printf("           linear               Linear kernel.\n");
	printf("           logarithmic          Logarithm kernel.\n");
	printf("           quadratic            Quadratic kernel.\n");
	printf("  --input <filename>      Input workload file.\n");
	printf("  --ncores <number>       Number of working cores.\n");
	printf("  --winsize <number>      Memory Accesses Window size\n");
	printf("  --seed <number>         Seed value.\n");
	printf("  --optimize <number>     0 = No Opt. 1 = KMeans DTW. 2 = Simple OPT. 3 = Model OPT\n");
	printf("  --help                  Display this message.\n");
	printf("Schedulers:\n");
	printf("  fcfs               First-Come, First-Served Scheduling.\n");
	printf("  srtf               Shortest Remaining Time First.\n");
	printf("  sca                Same Core Always.\n");


	exit(EXIT_SUCCESS);
}

/**
 * @brief Gets workload.
 *
 * @param filename Input workload filename.
 * @param ncores   Number of working cores.
 *
 * @returns A workload.
 */
static workload_tt get_workload(const char *filename, int ncores)
{
	FILE *input;   /* Input workload file. */
	workload_tt w; /* Workload.            */

	input = fopen(filename, "r");
	if (input == NULL)
		error("cannot open input workload file");

	w = workload_read(input, ncores);

	fclose(input);

	return (w);
}

/**
 * @brief Gets cores.
 * 
 * @param afilename Input architecture filename.
 * @param ncores    Number of cores in architecture, will be obtained from "afilename"
 * 
 * @return Working cores. 
*/
static array_tt get_cores(const char *filename, int ncores)
{
    FILE *file; /* Architecture file. */
	int read_cores;
    array_tt cores;

	assert(ncores > 0);

	if ((file = fopen(filename, "r")) == NULL)
		error("failed to open architecture file");

	assert(fscanf(file, "%d", &read_cores) == 1);
	if (read_cores < 1)
		error("bad architecture file");

	assert(ncores <= read_cores);

	cores = array_create(ncores);
	
	for (int i = 0; i < ncores; i++)
	{
        core_tt c;      /** Core.                       */
        int capacity;   /** Core's processing capacity. */
		int cache_line; /** Number of cache lines.      */
		int cache_ways; /** Number of cache ways.       */
		int num_blocks; /** Number of blocks per way    */
		
		assert(fscanf(file, "%d", &capacity) == 1);
		assert(fscanf(file, "%d", &cache_line) == 1);
		assert(fscanf(file, "%d", &cache_ways) == 1);
		assert(fscanf(file, "%d\n", &num_blocks) == 1);
		c = core_create(capacity, cache_line, cache_ways, num_blocks);
		array_set(cores, i, c);
	}

	/* House keeping. */
	fclose(file);

	return (cores);
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
 * @param wfilename   Input workload filename.
 * @param afilename   Input architecture filename.
 * @param kernelname  Application kernel name.
 * @param ncores      Number of cores in our simulation.
 * @param has_winsize Checks is winsize was informed.
*/
static void checkargs(const char *wfilename, const char *afilename, const char *kernelname, int ncores, int has_winsize)
{
	if (afilename == NULL)
		error("missing architecture file.");
	if (wfilename == NULL)
		error("missing input workload file.");
	if (kernelname == NULL)
		error("missing kernel name.");
	if (!(ncores > 0))
		error("missing cores.");
	if (args.processer == NULL)
		error("missing cores' processing strategy.");
	if (args.scheduler == NULL)
		error("missing loop scheduling strategy.");
	if (args.optimize < 0 || args.optimize > 3)
		error("missing optimization decision.");
	if (!has_winsize)
		error("missing window size.");
	if (args.winsize > QUANTUM)
		error("window size must be equal or smaller than QUANTUM.");
}

/**
 * @brief Reads command line arguments.
 *
 * @param argc Argument count.
 * @param argv Argument variables.
 */
static void readargs(int argc, const char **argv)
{
	const char *wfilename  = NULL;
	const char *afilename  = NULL;
	const char *kernelname = NULL;
    int ncores      = 0,
		has_winsize = 0;


	/* Parse command line arguments. */
	for (int i = 1; i < argc; i++)
	{
		if (!strcmp(argv[i], "--arch"))
			afilename = argv[++i];
		else if (!strcmp(argv[i], "--process"))	{
			i ++;
			if (!strcmp(argv[i], "non-preemptive"))
				args.processer = (non_preemptive);
			else if (!strcmp(argv[i], "random-preemptive"))
				args.processer = (random_preemptive);
			else if (!strcmp(argv[i], "rr-preemptive"))
				args.processer = (rr_preemptive);
			else 
				/* Sanity check. */
				error("invalid core processing strategy.");
		} else if (!strcmp(argv[i], "--batchsize"))
			args.batchsize = atoi(argv[++i]);
		else if (!strcmp(argv[i], "--input"))
			wfilename = argv[++i]; 
		else if (!strcmp(argv[i], "--kernel"))
			kernelname = argv[++i];
		else if (!strcmp(argv[i], "--ncores"))
			ncores = atoi(argv[++i]);
		else if (!strcmp(argv[i], "--winsize"))
		{
			args.winsize = atoi(argv[++i]);
			has_winsize = 1;
		}
		else if (!strcmp(argv[i], "--seed"))
			args.seed = atoi(argv[++i]);
		else if (!strcmp(argv[i], "--optimize"))
			args.optimize = atoi(argv[++i]);
		else if (!strcmp(argv[i], "--help"))
			usage();
		else
		{
            if (!strcmp(argv[i], "fcfs"))
                args.scheduler = sched_fcfs;
			else if (!strcmp(argv[i], "srtf"))
				args.scheduler = sched_srtf;
			else if (!strcmp(argv[i], "sca"))
				args.scheduler = sched_sca;
			else
				error("invalid option or unsupported scheduling strategy");

		}
	}


	checkargs(wfilename, afilename, kernelname, ncores, has_winsize);

	args.workload = get_workload(wfilename, ncores);
    args.cores = get_cores(afilename, ncores);
	args.kernel = get_kernel(kernelname);
}

/*============================================================================*
 * TASK SCHEDULER SIMULATOR                                                   *
 *============================================================================*/

/**
 * @brief A scheduler simulator
 */
int main(int argc, const char** argv)
{
    readargs(argc, argv);

	args.kernel(args.workload);

	srand(args.seed);

	workload_sort(args.workload, WORKLOAD_ARRIVAL);

	simsched(args.workload, args.cores, args.scheduler, args.processer, args.batchsize, args.winsize, args.optimize);

	/* House keeping, */
	for ( unsigned long int i = 0; i < array_size(args.cores); i++)
	{
		core_tt c = array_get(args.cores, i);
		core_destroy(c);
	}
	array_destroy(args.cores);
	workload_destroy(args.workload);

    return (EXIT_SUCCESS);
}