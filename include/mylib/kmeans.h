#ifndef KMEANS_H
#define KMEANS_H

    #include <stdbool.h>

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
	extern kmeans_tt kmeans_create(int, int, int**, int, int);
	extern void      kmeans_destroy(kmeans_tt);

    extern void kmeans_start(kmeans_tt);
    void kmeans_initialize_medoids(kmeans_tt);
    int  kmeans_find_nearest_cluster(kmeans_tt, int*);
    void kmeans_update_medoids(kmeans_tt);
    void kmeans_find_medoid(kmeans_tt, int);
	int kmeans_medoids_changed(kmeans_tt);
    /**@}*/

#endif /* KMEANS_H */