#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include <time.h>

void *PrintHello(void *threadid) {
	long tid;
	tid = (long) threadid;
	printf("Hello world, from thread %ld\n", tid);
	pthread_exit(NULL);
}

int* primes;
int n, base, numthreads;

void set_bit(int* n, int index) {
	int arrind = index / 32;
	int pos = index % 32;
	unsigned int flag = 1;
	flag = flag << pos;				// Shift the 1 bit left by pos spaces
	n[arrind] = n[arrind] | flag;
	//printf("Set bit at %d, %d \n", arrind, pos);
}

void clear_bit(int* n, int index) {
	int arrind = index / 32;
	int pos = index % 32;
	unsigned int flag = 1;
	flag = flag << pos;
	flag = ~flag;
	n[arrind] = n[arrind] & flag;
}

int get_bit(int* n, int index) {
	int arrind = index / 32;
	int pos = index % 32;
	unsigned int flag = 1;
	flag = flag << pos;				// Shift the 1 bit left by pos spaces
	if (n[arrind] & flag) {
		return 1;
	} else {
		return 0;
	}
}


/* Mark all odd multiples of base */
void mark_multiples(int k, int arraystart, int arrayend) {
	int startnum = arraystart / k;
	startnum = startnum * k;

	// If this thread is marking the first occurrence (eg 5, 7), start from k * k
	// Both conditions are for checking if this is the first occurrence.
	if (k == startnum || startnum == 0) {
		startnum = k * k;
	}
	
	// Don't need to check if k * k is out of our range or larger than n,
	// as we know previous primes have been checked.
	if (startnum > arrayend || k * k > n) {
		return;
	}

	printf("Startnum and k are: %d, %d \n", startnum, k);
	for (int i = startnum; i <= arrayend; i += k) {
		set_bit(primes, i);
		printf("%d ", i);
	}
	printf("\n");
}

void *worker(void* t) {
	int tid = (int) t;
	int start = 3;

	int remaining = n % ((n / numthreads) * numthreads);
	printf("Remainder = %d \n", remaining);

	// Get the array bounds this thread is responsible for
	int arraystart = n / numthreads * tid;
	if (arraystart < 3) {
		arraystart = 3;
	}
	int arrayend = n / numthreads * (tid + 1);
	if (arrayend > n) {
		arrayend = n;
	}
	// Give the last thread the remainder numbers
	if (tid == numthreads - 1) {
		arrayend += remaining;
	}
	printf("Start and End is %d, %d. t is %d\n", arraystart, arrayend, t);
	for (int i = start; i <= n; i += 2) {
		if (i > arrayend) {
			break;
		}
		if (get_bit(primes, i) == 0) {
			mark_multiples(i, arraystart, arrayend);
		}
	}
	printf("Thread %d done\n", tid);
	pthread_exit(NULL);
}


int main (int argc, char **argv) {
	n 				= strtol(argv[1], NULL, 10);
	numthreads 	= strtol(argv[2], NULL, 10);
	clock_t start, end;
	double elapsed;	

	// Find number of ints needed to store n in bits
	//int onlyoddn		= n / 2;
	int intsforbitarray = (n / 32) + 1;
	primes = malloc (intsforbitarray * sizeof(int));
	base = 3;

	printf("Need %d for n= %d\n", intsforbitarray, n);

	// Start timer 
	start = clock();

	pthread_t threads[numthreads];
	int rc;
	long t;
	void *status;
	for (t = 0; t < numthreads; t++) {
		printf("Started thread %d \n", t);
		rc = pthread_create(&threads[t], NULL, worker, (void *) t);
		if (rc) {
			printf("ERROR; return code from pthread_create() is %d \n", rc);
			exit(-1);
		}
	}
	for (int i = 0; i < numthreads; i++) {
		rc = pthread_join(threads[i], &status);
	}

	// Include 2 manually, since we don't check even numbers. By initialising at 3
	// and incrementing 2, we can halve the number of checks.
	printf("Primes less than %d are: 2 ", n);
	for (int i = 3; i <= n; i += 2) {
		if (get_bit(primes, i) == 0) {
			printf("%d ", i);
		}
	}
	printf("\n");

	//End timer
	end = clock();
	elapsed = (double)(end - start) / CLOCKS_PER_SEC;
	printf("Execution time: %f\n", elapsed);

	pthread_exit(NULL);
}