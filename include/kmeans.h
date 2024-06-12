#ifndef KMEANS_H
#define KMEANS_H

    #include <stdbool.h>
	#include <mylib/array.h>
	#include <mylib/queue.h>

	/**
	 * @brief Opaque pointer to a KMeans Function.
	 */
	typedef struct kmeans * kmeans_tt;

	/**
	 * @brief Constant pointer to a KMeans Function.
	 */
	typedef const struct kmeans * const_kmeans_tt;

	/**
	 * @name Kmeans' operations.
	 */
	/**@{*/
	extern kmeans_tt kmeans_create(int, int, int);
	extern void      kmeans_destroy(kmeans_tt);

    extern void kmeans_start(kmeans_tt, array_tt, queue_tt, int**, int);
	extern void kmeans_set_nvectors(kmeans_tt, int);
    void kmeans_initialize_medoids(kmeans_tt, int**, int**);
    int kmeans_find_nearest_cluster(kmeans_tt, int*, int**);
    void kmeans_update_medoids(kmeans_tt, int*, int**, int**);
    void kmeans_find_medoid(kmeans_tt, int, int*, int**, int**);
	int kmeans_medoids_changed(kmeans_tt, int**, int**);

	void min_max_normalize(int**, double**, int, int, double, double);
	void min_max_normalize_two(int*, double*, int*, double*, int);
	double dtw_distance(double*, double*, int);

	extern double jaccard_distance(int*, int*, int);
    /**@}*/

#endif /* KMEANS_H */