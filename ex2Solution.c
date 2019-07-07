// PPLS Exercise 2 Solution File
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

pthread_mutex_t barrier;  /* mutex semaphore for the barrier */
pthread_cond_t go;        /* condition variable for leaving */
int numArrived = 0;       /* count of the number who have arrived */

// Here's a DIY barrier, it's OK to use the pthreads one too if you know
// about it.
void Barrier() {
  pthread_mutex_lock(&barrier);
  numArrived++;
  if (numArrived == NTHREADS) {
    numArrived = 0;
    pthread_cond_broadcast(&go);
  } else
    pthread_cond_wait(&go, &barrier);
  pthread_mutex_unlock(&barrier);
}


typedef struct myargs {
  int id;
  int *a;
  int n;
} my_args;

typedef my_args *argptr;


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

void prefixthread (int myid, int *data, int n) { // my thread number, the whole array, its size

  int mylo, myhi, mysize;  // section of the array I am responsible for
  int neighbour, i;        

  mysize = n/NTHREADS;
  mylo   =  mysize * myid;
  myhi   =  mylo + mysize -1;
  if (myid==NTHREADS-1) {
    mysize+= n%NTHREADS;
    myhi  += n%NTHREADS;
  }

  // do a sequential prefix on the local data
  sequentialprefixsum (&data[mylo], mysize);
  Barrier();

  // thread 0 prefixes the top items of each segment
  if (myid==0) {
    for (neighbour=1; neighbour<NTHREADS-1; neighbour++) {
      data[mysize*(neighbour+1)-1] += data[(mysize*neighbour)-1];
    }
    // special case for the final update to allow for size of last segment
    if (NTHREADS>1) data[NITEMS-1] += data[mysize*(NTHREADS-1)-1];
  }
  Barrier();

  // fold top result from left neighbours local prefix into my prefix
  // thread 0 inactive because it has no left neighbour
  if (myid!=0) {
    for (i=mylo; i<myhi; i++) {
      data[i] += data[mylo-1];
    }
  }
}

void *call_prefixthread (void *args) {

  prefixthread (((my_args *)args)->id, ((my_args *)args)->a, ((my_args *)args)->n);

  return NULL;

}


// YOU MUST WRITE THIS FUNCTION AND ANY ADDITIONAL FUNCTIONS YOU NEED
void parallelprefixsum (int *data, int n) {
  pthread_t *threads;
  my_args *threadargs;
  int i;

  // Set up arguments for the worker threads
  threads = (pthread_t *) malloc (NTHREADS * sizeof(pthread_t));
  threadargs     = (my_args *) malloc (NTHREADS * sizeof(my_args));

  for (i=0; i<NTHREADS; i++) {
    threadargs[i].id = i;
    threadargs[i].a = data;
    threadargs[i].n = NITEMS;
  }

  // Create the worker threads
  for (i=0; i<NTHREADS; i++) {
    pthread_create (&threads[i], PTHREAD_CREATE_JOINABLE, call_prefixthread, (void *) &threadargs[i]);
  }

  // Wait for the workers to finish
  for (i=0; i<NTHREADS; i++) {
    pthread_join(threads[i], NULL);
  }

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
