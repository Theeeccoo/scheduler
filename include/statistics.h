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

#ifndef STATISTICS_H_
#define STATISTICS_H_

	/**
	 * @brief Opaque pointer to a probability distribution.
	 */
	typedef struct distribution * distribution_tt;

	/**
	 * @brief Constant opaque pointer to a probability distribution.
	 */
	typedef const struct distribution * const_distribution_tt;

	/**
	 * @brief Opaque pointer to a histogram.
	 */
	typedef struct histogram * histogram_tt;

	/**
	 * @brief Constant opaque pointer to a histogram.
	 */
	typedef const struct histogram * const_histogram_tt;

	/**
	 * @name Operations on Histograms
	 */
	/**@{*/
	extern void histogram_destroy(histogram_tt);
	extern double histogram_class(const_histogram_tt, int);
	extern int histogram_nclasses(const_histogram_tt);
	/**@}*/

	/**
	 * @name Operations on Probability Distributions
	 */
	/**@{*/
	extern histogram_tt distribution_histogram(const_distribution_tt, int);
	extern void distribution_destroy(distribution_tt);
	/**@}*/

	/**
	 * @name Known Probability Distributions
	 */
	/**@{*/
	extern distribution_tt dist_beta(void);
	extern distribution_tt dist_exponential(void);
	extern distribution_tt dist_gamma(void);
	extern distribution_tt dist_gaussian(void);
	extern distribution_tt dist_uniform(void);
<<<<<<< HEAD
=======
	extern distribution_tt dist_poisson(void);
>>>>>>> 9aff985 (Initial commit.)
	/**@}*/

#endif /* Statistics. */
