#include <assert.h>

#include <stdlib.h>
#include <math.h>
#include <float.h>

#include <mylib/queue.h>
#include <kmeans.h>
#include <task.h>


struct kmeans
{
    int max_iter;   /**< Max. number of iterations.             */
    int n_clusters; /**< Current ideal number of clusters.      */

    int n_vectors;  /**< Total number of arrays in vectors.     */
    int v_length;   /**< Total of elements in each array.       */
};

/**
 * @brief Initializes a new KMeans instance.
 * 
 * @param max_iter  Max number of desired iterations.
 * @param n_cluster Number of desired clusters. 
 * @param v_length  Total of elements in each array.
 * 
 * @returns New KMeans instance.
*/
kmeans_tt kmeans_create(int max_iter, int n_clusters, int v_length)
{
    struct kmeans *k;

    /* Sanity check. */
    assert(v_length > 0);

    k = (struct kmeans*) malloc(sizeof(struct kmeans));
    k->max_iter = max_iter;
    k->n_clusters = n_clusters;
    k->n_vectors = 0;
    k->v_length = v_length;

    return k;
}

/**
 * @brief Sets the number of vectors in current iteration
 * 
 * @param k        Target Kmeans struct
 * @param nvectors Number of vectors
 */
void kmeans_set_nvectors(struct kmeans *k, int nvectors)
{
    /* Sanity check. */
    assert(k != NULL);
    assert( nvectors >= 0 ); 

    k->n_vectors = nvectors;
}

/**
 * @brief Checks if medoids have changed after an iteration. It's usefull to prevent unecessary iterations when the algorithm has already converged.
 * 
 * @param k  Target KMeans instance.
 * @param om Old medoids.
 * @param me Current medoids.
 * 
 * @returns TRUE if atleast one medoid have changed. False otherwise.
*/
int kmeans_medoids_changed(struct kmeans *k, int **om, int **me)
{
    /* Sanity check. */
    assert(k != NULL);
    assert(om != NULL);

    for ( int i = 0; i < k->n_clusters; i++ )
    {
        for ( int j = 0; j < k->v_length; j++ )
        {
            if ( me[i][j] != om[i][j] ) 
            {
                return true;
            }
        }
    }
    return false;
}

/**
 * @brief Finds the newest medoid for a cluster. Medoid is the one (point) that minimizes the distance between all others (points) in a cluster. Jaccard distace used. 
 * 
 * @param k       Target KMeans instance.
 * @param cluster Target cluster.
 * @param labels  Current labels
 * @param me      Current medoids.
 * @param vectors All vectors.
*/
void kmeans_find_medoid(struct kmeans *k, int cluster, int *labels, int **me, int **vectors)
{
    /* Sanity check. */
    assert(k != NULL);

    int cluster_size = 0;
    for ( int i = 0 ; i < k->n_vectors; i++ )
    {
        if ( labels[i] == cluster ) cluster_size++;
    }

    int cluster_points[cluster_size][k->v_length];
    double n_cluster_points[cluster_size][k->v_length];
    cluster_size = 0;

    for ( int i = 0 ; i < k->n_vectors; i++ )
    {
        if ( labels[i] == cluster )
        {
            for ( int j = 0; j < k->v_length; j++ )
            {
                cluster_points[cluster_size][j] = vectors[i][j];   
            } 

            cluster_size++;            
        }
    }

    double min = DBL_MAX, max = -DBL_MAX;

    for ( int i = 0; i < cluster_size; i++ )
    {
        for ( int j = 0; j < k->v_length; j++ )
        {
            if ( cluster_points[i][j] < min ) min = cluster_points[i][j];
            if ( cluster_points[i][j] > max ) max = cluster_points[i][j];
        }
    }

    for ( int i = 0; i < cluster_size; i++ )
    {
        for ( int j = 0; j < k->v_length; j++ )
        {
            n_cluster_points[i][j] = (cluster_points[i][j] - min) / (max - min);
        }
    }

    double min_totalidst = INFINITY;
    int medoid_idx = -1;

    /* Finding which node will be the newest medoid. */
    for ( int i = 0; i < cluster_size; i++ )
    {
        double total_distance = 0.0;
        for ( int j = 0; j < cluster_size; j++ )
            total_distance += dtw_distance(n_cluster_points[i], n_cluster_points[j], k->v_length);

        if ( total_distance < min_totalidst )
        {
            min_totalidst = total_distance;
            medoid_idx = i;
        }
    }

    if ( medoid_idx != -1 )
    {
        for ( int j = 0; j < k->v_length; j++ )
            me[cluster][j] = cluster_points[medoid_idx][j];
    }

}

/**
 * @brief Updates medoids.
 * 
 * @param k       Target KMeans instance.
 * @param labels  Current labels.
 * @param me      Current medoids.
 * @param vectors All vectors.
*/
void kmeans_update_medoids(struct kmeans *k, int *labels, int **me, int **vectors)
{
    /* Sanity check. */
    assert(k != NULL);

    for ( int i = 0; i < k->n_clusters; i++ )
    {
        kmeans_find_medoid(k, i, labels, me, vectors);
    }
}

/** 
 * @brief Finding nearest cluster from 'vector'. Calculating the distance between 'vector' and medoids using jaccard distance. 
 * 
 * @param k      Target KMeans instance.
 * @param vector Target vector.
 * @param me     Current medoids.
 * 
 * @returns Index of nearest cluster. 
*/
int kmeans_find_nearest_cluster(struct kmeans *k, int *vector, int **me)
{
	/* Sanity check. */
    assert(k != NULL);
    assert(vector != NULL);

    double min = DBL_MAX, max = -DBL_MAX;

    for ( int i = 0; i < k->n_clusters; i++ )
    {
        for ( int j = 0; j < k->v_length; j++ )
        {
            int v_me = me[i][j];
            if (v_me < min) min = v_me;
            if (v_me > max) max = v_me;

        }
    }
    for ( int j = 0; j < k->v_length; j++ )
    {
        if (vector[j]< min) min = vector[j];
        if (vector[j]> max) max = vector[j];
    }

    double n_vector[k->v_length];
    double n_medoids[k->n_clusters][k->v_length];

    for ( int i = 0; i < k->n_clusters; i++ )
    {
        for ( int j = 0; j < k->v_length; j++ )
        {
            n_medoids[i][j] = (me[i][j] - min) / (max - min);
        }
    }
    for ( int i = 0; i < k->v_length; i++ )
    {
        n_vector[i] = (vector[i] - min) / (max - min);
    }

    double min_distance = dtw_distance(n_vector, n_medoids[0], k->v_length);
    int nearest_cluster = 0;

    for ( int i = 1; i < k->n_clusters; i++ )
    {
        double distance = dtw_distance(n_vector, n_medoids[i], k->v_length);
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
 * @param k       Target KMeans instance.
 * @param me      Current medoids.
 * @param vectors All vectors to be analysed.
*/
void kmeans_initialize_medoids(struct kmeans *k, int **me, int **vectors)
{
    /* Sanity check. */
    assert(k != NULL);

    int selected[k->n_vectors];
    for ( int i = 0; i < k->n_vectors; i++ ) selected[i] = 0;

    int first_medoid_idx = rand() % k->n_vectors;
    selected[first_medoid_idx] = 1;

    for ( int i= 0; i < k->v_length; i++)
        me[0][i] = vectors[first_medoid_idx][i];

    for ( int i = 1; i < k->n_clusters; i++ )
    {
        double maxmin_dist = -INFINITY;
        int next_medoid_idx = -1;

        double min = DBL_MAX, max = -DBL_MAX;
        for ( int j = 0; j < k->n_vectors; j++ )
        {
            for ( int m = 0; m < k->v_length; m++ )
            {
                if(vectors[j][m] < min) min = vectors[j][m];
                if(vectors[j][m] > max) max = vectors[j][m];
            }
        }

        double n_vectors[k->n_vectors][k->v_length];
        double n_medoids[k->n_clusters][k->v_length];

        for ( int j = 0; j < k->n_vectors; j++ )
        {
            for ( int l = 0; l < k->v_length; l++ )
            {
                n_vectors[j][l] = (vectors[j][l] - min) / (max - min);
            }
        }

        for ( int j = 0; j < k->n_clusters; j++ )
        {
            for ( int l = 0; l < k->v_length; l++ )
            {
                n_medoids[j][l] = (me[j][l] - min) / (max - min);
            }
        }
        
        for ( int j = 0; j < k->n_vectors; j++ )
        {
            if (selected[j]) continue;

            double min_dist = INFINITY;

            for ( int m = 0; m < i; m++ )
            {
                double distance = dtw_distance(n_vectors[j], n_medoids[m], k->v_length);
                if ( distance < min_dist ) min_dist = distance;
            }

            if ( min_dist > maxmin_dist )
            {
                maxmin_dist = min_dist;
                next_medoid_idx = j;
            }
        }


        if ( next_medoid_idx != -1 )
        {
            selected[next_medoid_idx] = 1;
            for ( int j = 0; j < k->v_length; j++ )
                me[i][j] = vectors[next_medoid_idx][j];
        }
    }
}

/**
 * @brief Start KMeans grouping. At the moment, the distance algorithm is jaccard. 
 * 
 * @param k           Target KMeans.
 * @param buckets     Target array of buckets.
 * @param tasks       Target tasks to be inserted ineach bucket.
 * @param vectors     Vectors to be grouped.
 * @param num_vectors Number of vectors.
 */
void kmeans_start(struct kmeans *k, struct array *buckets, queue_tt tasks, int **vectors, int num_vectors)
{
	/* Sanity check. */
    assert(k != NULL);
    assert(buckets != NULL);
    assert(tasks != NULL);
    assert(vectors != NULL);
    assert(num_vectors >= 0);

    kmeans_set_nvectors(k, num_vectors);
    int **medoids = (int**) malloc(sizeof(int*) * k->n_clusters);
    int **old_medoids = (int**) malloc(sizeof(int*) * k->n_clusters);
    int *labels= (int*) malloc(sizeof(int) * k->n_vectors);

    for ( int i = 0; i < k->n_clusters; i++ )
    {
        medoids[i] = (int*) malloc(sizeof(int) * k->v_length);
        old_medoids[i] = (int*) malloc(sizeof(int) * k->v_length);
        for ( int j = 0; j < k->v_length; j++ )
        {
            medoids[i][j] = -1;
            old_medoids[i][j] = -1;
        }
    }

    kmeans_initialize_medoids(k, medoids, vectors);


    for ( int iterations = 0; iterations < k->max_iter; iterations++ )
    {   
        /* Saving old medoids. */
        for ( int i = 0; i < k->n_clusters; i++ )
        {   
            for ( int j = 0; j < k->v_length; j++ )
            {
                old_medoids[i][j] = medoids[i][j];
            }
        }

        for ( int i = 0; i < k->n_vectors; i++ )
        {
            labels[i] = kmeans_find_nearest_cluster(k, vectors[i], medoids);
        }

        kmeans_update_medoids(k, labels, medoids, vectors);

        if ( !kmeans_medoids_changed(k, old_medoids, medoids) )
        {
            break;
        }

    }

    // Inserting tasks into buckets
    for ( int i = 0; i < num_vectors; i++ )
    {
        queue_tt bucket = array_get(buckets, labels[i]);
        queue_insert(bucket, queue_remove(tasks));
    }

    for ( int i = 0; i < k->n_clusters; i++ )
    {
        free(medoids[i]);    
        free(old_medoids[i]);
    }
    free(medoids);
    free(old_medoids);
    free(labels);
}

void min_max_normalize(int **v1, double **normalized, int n_vectors, int vector_length, double min, double max)
{

    for (int i = 0; i < n_vectors; i++) 
    {
        for (int j = 0; j < vector_length; j++) 
        {
            normalized[i][j] = (v1[i][j] - min) / (max - min);
        }
    }
}

double dtw_distance(double *v1, double *v2, int size)
{
    double DTW[size + 1][size + 1];
    for ( int i = 0; i <= size; i++ )
    {
        for ( int j = 0; j <= size; j++ )
        {
            double cost = fabs(v1[i - 1] - v2[j - 1]);
            DTW[i][j] = cost + fmin(fmin(DTW[i - 1][j], DTW[i][j - 1]), DTW[i - 1][j - 1]);
        }
    }
    return DTW[size][size];
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

/**
 * @brief Destroys KMeans instance.
 * 
 * @param k Target KMeans instance.
*/
void kmeans_destroy(struct kmeans *k)
{
    /* Sanity check. */
    assert(k != NULL);
    free(k);
}