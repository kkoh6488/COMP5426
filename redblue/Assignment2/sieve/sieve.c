#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include <time.h>

int* primes;
int n, base, numthreads;

// Sets the bit for the specified index in the bit array
void set_bit(int* n, int index) {
	int arrind = index / 32;
	int pos = index % 32;
	unsigned int flag = 1;
	flag = flag << pos;				// Shift the 1 bit left by pos spaces
	n[arrind] = n[arrind] | flag;
}

// Clears the bit for the specified index in the bit array
void clear_bit(int* n, int index) {
	int arrind = index / 32;
	int pos = index % 32;
	unsigned int flag = 1;
	flag = flag << pos;
	flag = ~flag;
	n[arrind] = n[arrind] & flag;
}

// Gets the value of the bit at the specified index in the bit array
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
	// Optimisation: start from the prime's square
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

	for (int i = startnum; i <= arrayend; i += k) {
		set_bit(primes, i);
	}
}

void *worker(void* t) {
	int tid = (int) t;
	int start = 3;

	int remaining = n % ((n / numthreads) * numthreads);

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
	for (int i = start; i <= n; i += 2) {
		if (i > arrayend) {
			break;
		}
		if (get_bit(primes, i) == 0) {
			mark_multiples(i, arraystart, arrayend);
		}
	}
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

	// Start timer 
	start = clock();

	pthread_t threads[numthreads];
	int rc;
	long t;
	void *status;
	for (t = 0; t < numthreads; t++) {
		rc = pthread_create(&threads[t], NULL, worker, (void *) t);
		if (rc) {
			printf("ERROR; return code from pthread_create() is %d \n", rc);
			exit(-1);
		}
	}
	// Wait for all threads to finish
	for (int i = 0; i < numthreads; i++) {
		rc = pthread_join(threads[i], &status);
	}

	// Include 2 manually, since we don't check even numbers. By initialising at 3
	// and incrementing by 2, we can halve the number of checks.
	FILE *f = fopen("primes.txt", "w");
	if (f == NULL) {
		printf("Couldn't open file.\n");
		exit(1);
	}
	fprintf(f, "Primes less than %d are: \n2\n", n);
	for (int i = 3; i <= n; i += 2) {
		if (get_bit(primes, i) == 0) {
			fprintf(f, "%d\n", i);
		}
	}
	fclose(f);

	//End timer
	end = clock();
	elapsed = (double)(end - start) / CLOCKS_PER_SEC;
	printf("Execution time: %f\n", elapsed);

	pthread_exit(NULL);
}