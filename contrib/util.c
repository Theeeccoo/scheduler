/*
 * Copyright(C) 2016 Pedro H. Penna <pedrohenriquepenna@gmail.com>
 * 
 * This file is part of MyLib.
 *
 * MyLib is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 * 
 * MyLib is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with MyLib; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

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
void *smalloc(size_t size)
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
void error(const char *msg)
{
	fprintf(stderr, "error %s\n", msg);

	exit(EXIT_FAILURE);
}

double jaccard_distance(int *v1, int *v2, int size)
{
    /* Sanity check. */
    assert(v1 != NULL);
    assert(v2 != NULL);
    assert(size > 0);

	int intersection = 0,
	    union_size   = 0;
	double jaccard = 0.0; /**< Final result. */

	for ( int i = 0; i < size; i++ )
	{
		for ( int j = 0; j < size; j++ )
		{
			if ( v1[i] == v2[j] )
			{
				intersection ++;
				break;
			}
		}
		union_size++;
	}

	for ( int i = 0; i < size; i++ )
	{
		int found = 0; 
		for ( int j = 0; j < size; j++ )
		{
			if ( v2[i] == v1[i] )
			{
				found = 1;
				break;
			}
		}
		if ( !found ) union_size ++;
	}
	
	jaccard = (double) intersection / union_size;
	return 1.0 - jaccard;
}
