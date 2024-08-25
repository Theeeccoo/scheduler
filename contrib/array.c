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

#include <mylib/util.h>
#include <mylib/array.h>

/**
 * @brief Array.
 */
struct array
{
	unsigned long int size; /**< Maximum size. */
	void **elements;        /**< Elements.     */
};

/**
 * @brief Creates an array.
 *
 * @param size Array size.
 */
struct array *array_create(unsigned long int size)
{
	struct array *a;

	/* Create array. */
	a = smalloc(sizeof(struct array));
	a->size = size;
	a->elements = smalloc(size*sizeof(void *));

	return (a);
}

/**
 * @brief Destroys an array.
 *
 * @param a Target array.
 */
void array_destroy(struct array *a)
{
	/* Sanity check. */
	assert(a != NULL);

	free(a->elements);
	free(a);
}

/**
 * @brief Returns the size of an array.
 *
 * @param a Target array.
 */
unsigned long int array_size(const struct array *a)
{
	/* Sanity check. */
	assert(a != NULL);

	return (a->size);
}

/**
 * @brief Sets the ith element of an array.
 *
 * @param a   Target array.
 * @param idx Index.
 * @param obj Object.
 */
void array_set(struct array *a, unsigned long int idx, void *obj)
{
	/* Sanity check. */
	assert(a != NULL);
	assert((idx < a->size));
	assert(obj != NULL);

	a->elements[idx] = obj;
}

/**
 * @brief Gets the ith element of an array.
 *
 * @param a   Target array.
 * @param idx Index.
 *
 * @returns The ith element of the target array.
 */
void *array_get(const struct array *a, unsigned long int idx)
{
	/* Sanity check. */
	assert(a != NULL);
	assert((idx < a->size));

	return (a->elements[idx]);
}

/**
 * @brief Shuffles an array.
 *
 * @param a Target array.
 */
void array_shuffle(struct array *a)
{
	/* Sanity check. */
	assert(a != NULL);

	/* Shuffle array. */
	for (unsigned long int i = 0; i < a->size - 1; i++)
	{
		int j;     /* Shuffle index.  */
		void *tmp; /* Temporary data. */

		j = rand()%a->size;

		tmp = a->elements[i];
		a->elements[i] = a->elements[j];
		a->elements[j] = tmp;
	}
}
