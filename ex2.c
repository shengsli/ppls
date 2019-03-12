/*
- In the first phase, parent thread (thread 0) initializes each child thread id, calculate its chunk 
size and prepare items which will be allocated to each child thread. Thread 0 calls ~pthread_create~ 
to create threads, passes start routine and arguments (thread id, chunk size, items). 
- In the second phase, I used ~pthread_barrier~ to synchronize all the threads before thread 0 start 
to perform sequential prefix sum at highest indices. I put this barrier before thread 0 start prefix 
sum at highest indices, and I put this barrier right after child threads finish the first phase. 
Sequential prefix sum at highest indices is implemented in ~parent_sequentialprefixsum()~. Thread 1 
has finished the calculation on the first chunk. Thread 0 only needs to calculate from the second 
chunk. The last item of each chunk adds the value of last item in the previous chunk to itself. Other 
threads wait for thread 0 until it finishes this highest-indexed prefix sum. To achieve this, I used 
a second barrier after the code block of this highest-indexed prefix sum in thread 0, and I put this 
second barrier right after the first barrier, before child threads do any further computation. 
- In the third phase, each child thread does local updates to its own data chunk. The last item of 
each chunk has completed its calculation in phase 2. In this phase, each item (except the last one) 
add the value of last item in the previous chunk to itself. At this point, all threads finish 
calculation and terminate by return. Thread 0 joins all threads. 
*/

// PPLS Exercise 2 Starter File
//
// See the exercise sheet for details
//
// Note that NITEMS, NTHREADS and SHOWDATA should
// be defined at compile time with -D options to gcc.
// They are the array length to use, number of threads to use
// and whether or not to printout array contents (which is
// useful for debugging, but not a good idea for large arrays).

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

pthread_barrier_t barrier;
pthread_barrier_t barrier2;

// Print a helpful message followed by the contents of an array
// Controlled by the value of SHOWDATA, which should be defined
// at compile time. Useful for debugging.
void showdata (char *message,  int *data,  int n) {
  int i; 

  if (SHOWDATA) {
    printf (message);
    for (i=0; i<n; i++ ){
     printf (" %d", data[i]);
    }
    printf("\n");
  }
}

// Check that the contents of two integer arrays of the same length are equal
// and return a C-style boolean
int checkresult (int* correctresult,  int *data,  int n) {
  int i; 

  for (i=0; i<n; i++ ){
    if (data[i] != correctresult[i]) return 0;
  }
  return 1;
}

// Compute the prefix sum of an array **in place** sequentially
void sequentialprefixsum (int *data, int n) {
  int i;

  for (i=1; i<n; i++ ) {
    data[i] = data[i] + data[i-1];
  }
}

// MY THREAD CODE
typedef struct arg_pack {
  int tid;
  int *data;
  int n;
} arg_pack;

typedef arg_pack *argptr;

void *thread_sequentialprefixsum (void *args) {
  int tid;
  int *data;
  int n; // allocated chunk size, varies among threads
  // retrieve the arguments
  tid = ((arg_pack *)args)->tid;
  data = ((arg_pack *)args)->data;
  n = ((arg_pack *)args)->n;

  printf ("Hello from thread %d, i have %d items.\n", tid, n);
  sequentialprefixsum(data, n);
  pthread_barrier_wait(&barrier);
  pthread_barrier_wait(&barrier2);
  int chunk_size = n / NTHREADS;
  // phase 3: local updates
  if (tid == 0) return NULL;
  int i;
  for (i=0; i<n-1; ++i)
  	*(data+i) += *(data-1);
  return NULL;
}

// thread 0 perform sequential prefix sum at highest indices of each chunk 
void *parent_sequentialprefixsum (int *data, int n) {
  // perform NTHREADS-1 times sum, from 2nd chunk to the last one
  int chunk_size = n / NTHREADS; // minimum # of items
  int i;
  for (i=1; i<NTHREADS-1; i++) // 2nd chunk to 2nd last chunk
    data[(i+1)*chunk_size-1] = data[(i+1)*chunk_size-1] + data[i*chunk_size-1];
  data[NITEMS-1] += data[(NTHREADS-1)*chunk_size-1];
  return NULL;
}

// YOU MUST WRITE THIS FUNCTION AND ANY ADDITIONAL FUNCTIONS YOU NEED
void parallelprefixsum (int *data, int n) {
  pthread_t *threads;
  arg_pack *threadargs;
  int i;
  threads = (pthread_t *) malloc (NTHREADS * sizeof(pthread_t));
  threadargs = (arg_pack *) malloc (NTHREADS * sizeof(arg_pack));
  if (pthread_barrier_init(&barrier, NULL, NTHREADS+1)) { // create a barrier
	printf("fail to init barrier\n");
	return;
  }
  if (pthread_barrier_init(&barrier2, NULL, NTHREADS+1)) { // create a barrier
	printf("fail to init barrier2\n");
	return;
  }
  
  // phase 1:
  int chunk_size = n / NTHREADS;
  for (i=0; i<NTHREADS; i++)   // allocate items to threads
  {
	threadargs[i].tid = i;
	threadargs[i].data = data + (chunk_size * i);
	threadargs[i].n = chunk_size;
  }
  threadargs[NTHREADS-1].n += n - chunk_size * NTHREADS; // give rest items to the last thread
  for (i=0; i<NTHREADS; i++) // create threads
	pthread_create(&threads[i], NULL, thread_sequentialprefixsum, (void *) &threadargs[i]);
  pthread_barrier_wait(&barrier); // snychronize before phase 2
  pthread_barrier_destroy(&barrier);
  
  // phase 2: thread 0 performs sequential prefix sum to the last item of each chunk
  parent_sequentialprefixsum(data, n);
  pthread_barrier_wait(&barrier2);
  pthread_barrier_destroy(&barrier2);
  
  // phase 3: child threads do local update, see thread_sequentialprefixsum()

  // join and exit
  for (i=0; i<NTHREADS; i++)
  	pthread_join(threads[i], NULL);
}


int main (int argc, char* argv[]) {

  int *arr1, *arr2, i;

  // Check that the compile time constants are sensible for this exercise
  if ((NITEMS>10000000) || (NTHREADS>32)) {
    printf ("So much data or so many threads may not be a good idea! .... exiting\n");
    exit(EXIT_FAILURE);
  }

  // Create two copies of some random data
  arr1 = (int *) malloc(NITEMS*sizeof(int));
  arr2 = (int *) malloc(NITEMS*sizeof(int));
  srand((int)time(NULL));
  for (i=0; i<NITEMS; i++) {
     arr1[i] = arr2[i] = rand()%5;
  }
  showdata ("initial data          : ", arr1, NITEMS);

  // Calculate prefix sum sequentially, to check against later
  sequentialprefixsum (arr1, NITEMS);
  showdata ("sequential prefix sum : ", arr1, NITEMS);

  // Calculate prefix sum in parallel on the other copy of the original data
  parallelprefixsum (arr2, NITEMS);
  showdata ("parallel prefix sum   : ", arr2, NITEMS);

  // Check that the sequential and parallel results match
  if (checkresult(arr1, arr2, NITEMS))  {
    printf("Well done, the sequential and parallel prefix sum arrays match.\n");
  } else {
    printf("Error: The sequential and parallel prefix sum arrays don't match.\n");
  }

  free(arr1); free(arr2);
  return 0;
}
