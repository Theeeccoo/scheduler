/*
 * Copyright(C) 2016 Pedro H. Penna <pedrohenriquepenna@gmail.com>
 * 
 * This file is part of WorkloadGen.
 *
 * WorkloadGen is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 * 
 * WorkloadGen is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with WorkloadGen; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#ifndef STATISTICS_H_
#define STATISTICS_H_

	/**
	 * @brief Opaque pointer to a probability distribution.
	 */
	typedef const struct distribution * distribution_tt;

	/**
	 * @name Operations
	 */
	/**@{*/
	extern const double *histgen(distribution_tt, int);
	extern void distfree(distribution_tt);
	/**@}*/

	/**
	 * @name Known Probability Distributions
	 */
	/**@{*/
	extern distribution_tt beta(double);
	extern distribution_tt gamma(double);
	extern distribution_tt gaussian(double);
	extern distribution_tt uniform(double);
	/**@}*/

#endif /* Statistics. */
