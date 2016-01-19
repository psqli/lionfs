/* link with -lpthread */
#include <pthread.h>

#ifdef LOG
#include <stdio.h>

#define log(x) puts(x)
#endif

#include "cache.h"

/* Do not change this!
 * Maximum size of Cache Requests that array support
 *  * Defined by MAX_CACHEREQUESTS in cache.h
 */
#define ARRAY_SIZE MAX_CACHEREQUESTS
#define ARRAY_LAST_INDEX (ARRAY_SIZE - 1)

/*
 * How this array works?
 *   It works like a FIFO, where the first CacheR structure that in is the first
 *   that out.
 *   To manage our environment as Cache Manager needs, we create two
 *   different functions in API:
 *     array_add(): Adds a CacheR structure at the end of array. Case array is
 *                  full, it blocks until occur a call to array_get().
 *     array_get(): Return the first CacheR structure in array, remove this and
 *                  bring all other members one space less to align the array.
 *                  If there isn't any structure in array, it blocks until occur
 *                  a call to array_add().
 *   Both functions are thread-safe, which means that if any executes at the
 *   same time our array will not be corrupted.
 */

/* Array used to process Cache Requests. */
static CacheR *array[ARRAY_SIZE];

/* next NULL space and last occupied space, respectively */
static int next = 0, last = -1;

static pthread_mutex_t ctlmutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t condinc = PTHREAD_COND_INITIALIZER;
static pthread_cond_t conddec = PTHREAD_COND_INITIALIZER;

static void
align_array(void)
{
	int i;

	for(i = 0; i < last; i++)
		array[i] = array[i + 1];

	array[last] = NULL; /* old last */

	last--;
	next--;

	return;
}

void
array_clear(void)
{
	int i;

	for(i = 0; i < ARRAY_SIZE; i++)
		array[i] = NULL;

	return;
}

int
array_add(CacheR *cacher)
{
#ifdef LOG
	int waitd = 0;
#endif
	pthread_mutex_lock(&ctlmutex);

	/* If the Cache Requests take a long time to be processed and
	 * our array becomes full ... */
	if(next > ARRAY_LAST_INDEX)
	{
#ifdef LOG
		waitd = 1;
#endif
		pthread_cond_wait(&conddec, &ctlmutex);
	}

	/* We put in array with only one instruction */
	array[next] = cacher;

	last++;
	next++;

	pthread_mutex_unlock(&ctlmutex);

	pthread_cond_signal(&condinc);
#ifdef LOG
	if(waitd)
		log("array full, waited");
#endif
	return 0;
}

CacheR*
array_get(void)
{
#ifdef LOG
	int waitd = 0;
#endif
	NtfIR *cacher;

	pthread_mutex_lock(&ctlmutex);

	/* If there isn't an Cache Request in array ... */
	if((cacher = array[0]) == NULL)
	{
#ifdef LOG
		waitd = 1;
#endif
		pthread_cond_wait(&condinc, &ctlmutex);
	}

	cacher = array[0];

	align_array();

	pthread_mutex_unlock(&ctlmutex);

	pthread_cond_signal(&conddec);
#ifdef LOG
	if(waitd)
		log("array void, waited");
#endif
	return cacher;
}
