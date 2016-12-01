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

#ifndef UTIL_H_
#define UTIL_H_

	#include <assert.h>
	#include <stdlib.h>
	#include <stdio.h>

	/**
	 * @brief Safe malloc().
	 *
	 * @param size Number of bytes to allocate.
	 *
	 * @returns Allocated block of memory.
	 */
	inline void *smalloc(size_t size)
	{
		void *p;

		p = malloc(size);
		assert(p != NULL);

		return (p);
	}
	
	/**
	 * @brief Prints an error message and terminates.
	 *
	 * @param msg Error message.
	 */
	inline void error(const char *msg)
	{
		fprintf(stderr, "error %s\n", msg);

		exit(EXIT_FAILURE);
	}

#endif /* UTIL_H_ */
