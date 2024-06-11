#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <mylib/util.h>
#include <mylib/kmeans.h>


struct kmeans
{
    int max_iter;      /**< Max. number of iterations.         */
    int n_clusters;    /**< Current ideal number of clusters.  */

    int **vectors;     /**< Values to be analyzed.             */
    int n_vectors;     /**< Total number of arrays in vectors. */
    int v_length;      /**< Total of elements in each array.   */

    int **medoids;     /**< KMeans' medoids.                   */
    int **old_medoids; /**< Previous kmeans' medoids.          */
    int *labels;       /**< Current kmeans' clusters.          */
};

/**
 * @brief Initializes a new KMeans instance.
 * 
 * @param max_iter  Max number of desired iterations.
 * @param n_cluster Number of desired clusters. 
 * @param vectors   Arrays to be analyzed.
 * @param n_vectors Total number of arrays in vectors.
 * @param v_length  Total of elements in each array.
 * 
 * @returns New KMeans instance.
*/
kmeans_tt kmeans_create(int max_iter, int n_clusters, int **vectors, int n_vectors, int v_length)
{
    struct kmeans *k;

    /* Sanity check. */
    assert(n_vectors > 0);
    assert(v_length > 0);

    k = smalloc(sizeof(kmeans_tt));

    k->max_iter = max_iter;
    k->n_clusters = n_clusters;
    k->vectors = vectors;
    k->n_vectors = n_vectors;
    k->v_length = v_length;

    return k;
}

/**
 * @brief Checks if medoids have changed after an iteration. It's usefull to prevent unecessary iterations when the algorithm has already converged.
 * 
 * @param k Target KMeans instance.
 * 
 * @returns TRUE if atleast one medoid have changed. False otherwise.
*/
int kmeans_medoids_changed(struct kmeans *k)
{
    /* Sanity check. */
    assert(k != NULL);

    for ( int i = 0; i < k->n_clusters; i++ )
    {
        for ( int j = 0; j < k->n_vectors; j++ )
        {
            if ( k->medoids[i][j] != k->old_medoids[i][j] ) return true;
        }
    }
    return false;
}

/**
 * @brief Finds the newest medoid for a cluster. Medoid is the one (point) that minimizes the distance between all others (points) in a cluster. Jaccard distace used. 
 * 
 * @param k       Target KMeans instance.
 * @param cluster Target cluster.
*/
void kmeans_find_medoid(struct kmeans *k, int cluster)
{
    /* Sanity check. */
    assert(k != NULL);

    int cluster_points[k->n_vectors][k->v_length],
        cluster_size = 0;

    for ( int i = 0 ; i < k->n_vectors; i++ )
    {
        if ( k->labels[i] == cluster )
        {
            for ( int j = 0; j < k->v_length; j++ )
                cluster_points[cluster_size][j] = k->vectors[i][j];

            cluster_size++;
        }
    }

    double min_totalidst = INFINITY;
    int medoid_idx = -1;

    /* Finding which node will be the newest medoid. */
    for ( int i = 0; i < cluster_size; i++ )
    {
        double total_distance = 0.0;
        for ( int j = 0; j < cluster_size; j++ )
            total_distance += jaccard_distance(cluster_points[i], cluster_points[j], k->v_length);

        if ( total_distance < min_totalidst )
        {
            min_totalidst = total_distance;
            medoid_idx = i;
        }
    }

    if ( medoid_idx != -1 )
    {
        for ( int j = 0; j < k->v_length; j++ )
            k->medoids[cluster][j] = cluster_points[medoid_idx][j];
    }
}

/**
 * @brief Updates medoids.
 * 
 * @param k Target KMeans instance.
*/
void kmeans_update_medoids(struct kmeans *k)
{
    /* Sanity check. */
    assert(k != NULL);

    for ( int i = 0; i < k->n_clusters; i++ )
        kmeans_find_medoid(k, i);
}

/** 
 * @brief Finding nearest cluster from 'vector'. Calculating the distance between 'vector' and medoids using jaccard distance. 
 * 
 * @param k      Target KMeans instance.
 * @param vector Target vector.
 * 
 * @returns Index of nearest cluster. 
*/
int kmeans_find_nearest_cluster(struct kmeans *k, int *vector)
{
	/* Sanity check. */
    assert(k != NULL);
    assert(vector != NULL);

    double min_distance = jaccard_distance(vector, k->medoids[0], k->v_length);
    int nearest_cluster = 0;

    for ( int i = 1; i < k->n_clusters; i++ )
    {
        double distance = jaccard_distance(vector, k->medoids[i], k->v_length);
        if ( distance < min_distance )
        {
            min_distance = distance;
            nearest_cluster = i;
        }
    }
    return nearest_cluster;
}

/**
 * @brief Selecting random values in 'vectors' to be initial medoids.
 * 
 * @param k Target KMeans instance.
*/
void initialize_medoids(struct kmeans *k)
{
	/* Sanity check. */
    assert(k != NULL);

    int used_indices[k->n_vectors]; 
    for ( int i = 0; i < k->n_vectors; i++ ) used_indices[i] = 0;

    for ( int i = 0; i < k->n_clusters; i++ )
    {
        int idx; 
        /* Ensuring that medoids have different values. */
        do 
        {
            idx = rand() % k->n_vectors;
        } while ( used_indices[idx] );
        used_indices[idx] = 1;

        for ( int j = 0; k->v_length; j++ )
            k->medoids[i][j] = k->vectors[idx][i];
    }
}

void kmeans_start(struct kmeans *k)
{
	/* Sanity check. */
    assert(k != NULL);

    kmeans_initialize_medoids(k);

    for ( int iterations = 0; iterations < k->max_iter; iterations++ )
    {   
        /* Saving old medoids. */
        for ( int i = 0; i < k->n_clusters; i++ )
        {
            for ( int j = 0; j < k->v_length; j++ )
                k->old_medoids[i][j] = k->medoids[i][j];
        }

        for ( int i = 0; i < k->n_vectors; i++ )
            k->labels[i] = kmeans_find_nearest_cluster(k, k->vectors[i]);

        kmeans_update_medoids(k);

        if ( !kmeans_medoids_changed(k) )
            break;
    }
}

/**
 * @brief Destroys KMeans instance.
 * 
 * @param k Target KMeans instance.
*/
void kmeans_destroy(struct kmeans *k)
{
    /* Sanity check. */
    assert(k != NULL);

    for ( int i = 0; i < k->n_clusters; i++ )
    {
        free(k->medoids[i]);
        free(k->old_medoids[i]);
    }
    free(k->medoids);
    free(k->old_medoids);
    free(k->labels);
}