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
  printf("this msg should show up after phase 2.\n");
  int chunk_size = n / NTHREADS;
  // phase 3: local updates
  if (tid == 0) return NULL;
  int i;
  //printf("%d", tid); showdata("***thread debug", data, n);
  for (i=0; i<n-1; ++i)
  {
	printf("%d -> ",*(data+i));
  	*(data+i) += *(data-1);
	printf("%d\n",*(data+i));
  }
  showdata("***thread debug", data, n);
  return NULL;
}

// thread 0 perform sequential prefix sum at highest indices of each chunk 
void *parent_sequentialprefixsum (int *data, int n) {
  // perform NTHREADS-1 times sum, from 2nd chunk to the last one
  int chunk_size = n / NTHREADS; // minimum # of items
  int i;
  for (i=1; i<NTHREADS-1; i++) { // 2nd chunk to 2nd last chunk
	/* printf("data[%d] = %d + %d\n", */
	/* 	   (i+1)*chunk_size-1, data[(i+1)*chunk_size-1], data[i*chunk_size-1]); */
    data[(i+1)*chunk_size-1] = data[(i+1)*chunk_size-1] + data[i*chunk_size-1];
	//showdata("***debug", data, NITEMS);
  }
  /* printf("data[%d] = %d + %d\n", */
  /* 		 NITEMS-1, data[NITEMS-1], data[(NTHREADS-1)*chunk_size-1]); */
  data[NITEMS-1] += data[(NTHREADS-1)*chunk_size-1];
  //showdata("***debug", data, NITEMS);
  return NULL;
}

// YOU MUST WRITE THIS FUNCTION AND ANY ADDITIONAL FUNCTIONS YOU NEED
void parallelprefixsum (int *data, int n) {
  printf("call parallel prefix sum\n");
  pthread_t *threads;
  arg_pack *threadargs;
  int i;
  threads = (pthread_t *) malloc (NTHREADS * sizeof(pthread_t));
  threadargs = (arg_pack *) malloc (NTHREADS * sizeof(arg_pack));
  if (pthread_barrier_init(&barrier, NULL, NTHREADS+1)) { // create a barrier
	printf("fail to init barrier\n");
	return;
  }
  // phase 2: thread 0 performs sequential prefix sum to the last item of each chunk
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
  pthread_barrier_wait(&barrier); // snyc
  printf("synchronization is done.\n");
  pthread_barrier_destroy(&barrier);
  
  parent_sequentialprefixsum(data, n);
  showdata("***debug: phase 2 done: ", data, NITEMS);
  pthread_barrier_wait(&barrier2);
  pthread_barrier_destroy(&barrier2);
  
  // phase 3: child threads do local update

  // join and exit
  for (i=0; i<NTHREADS; i++)
  	pthread_join(threads[i], NULL);
  showdata("***debug", data, NITEMS);
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
