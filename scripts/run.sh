#
# Copyright(C) 2016 Pedro H. Penna <pedrohenriquepenna@gmail.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#  
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#  
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
# MA 02110-1301, USA.
# 

#
# $1: Number of threads.
# $2: Number of loop iterations.
#

# Directories.
BINDIR=$PWD/bin
CSVDIR=$PWD/csv

# Number of iterations.
NITERATIONS=$2

# Kernel type.
KERNEL_TYPE=linear

# Scheduling strategies.
STRATEGIES=(static dynamic srr)

# Workloads.
WORKLOAD=(beta gamma gaussian poisson)

# Workload sorting.
SORT=(ascending)

# Skewness
SKEWNESS=(0.50 0.55 0.60 0.65 0.70 0.75 0.80 0.85 0.90)

#===============================================================================
#                              UTILITY ROUTINES
#===============================================================================

#
# Maps threads on the cores.
#
# $1 Number of threads.
# $2 Is simultaneous multithreading (SMT) enabled?
#
function map_threads
{
	# Build thread map.
	if [ $2 == "true" ]; then
		for (( i=0; i < $1; i++ )); do
			AFFINITY[$i]=$((2*$i))
		done
	else
		for (( i=0; i < $1; i++ )); do
			AFFINITY[$i]=$i
		done
	fi
	
	export OMP_NUM_THREADS=$1
	export GOMP_CPU_AFFINITY="${AFFINITY[@]}"
}

#===============================================================================
#                              PARSING ROUTINES
#===============================================================================

#
# Extracts variables from raw results.
#   $1 Filename prefix.
#
function extract_variables
{	
	grep "Total Cycles" $1.tmp \
	| cut -d" " -f 3           \
	>> $CSVDIR/$1-cycles.csv
	
	grep "thread" $1.tmp \
	| cut -d" " -f 3           \
	>> $CSVDIR/$1-workload.csv
}

#
# Parses the benchmark.
#  $1 Scheduling strategy.
#  $2 Number of threads.
#  $3 Workload.
#  $4 Skewness.
#
function parse_benchmark
{
	extract_variables benchmark-$3-$4-$NITERATIONS-$1-$2
}

#===============================================================================
#                                 RUN ROUTINES
#===============================================================================

#
# Run synthetic benchmark.
#  $1 Scheduling strategy.
#  $2 Number of threads.
#  $3 Workload.
#  $4 Skewness
#
function run_benchmark
{
	echo "  Benchmark with $2 thread(s)"
	$BINDIR/scheduler \
		--kernel $KERNEL_TYPE      \
		--nthreads $2              \
		--niterations $NITERATIONS \
		--pdf $3                   \
		--skewness $4              \
		--sort ascending           \
		$1                         \
	2>> benchmark-$3-$4-$NITERATIONS-$1-$2.tmp
}

#===============================================================================
#                                 MAIN ROUTINE
#===============================================================================

mkdir -p $CSVDIR

for strategy in "${STRATEGIES[@]}"; do
	for skewness in "${SKEWNESS[@]}"; do
		for workload in "${WORKLOAD[@]}"; do
			echo "== Running $strategy $skewness $workload"
			run_benchmark $strategy $1 $workload $skewness
			parse_benchmark $strategy $1 $workload $skewness
			rm -f *.tmp
		done
	done
done
