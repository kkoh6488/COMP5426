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
int n, base;

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
void mark_multiples(int k) {
	int start = k * 2;
	for (int i = start; i < n; i += k) {
		set_bit(primes, i);
		//printf("%d ", i);
	}
	//printf("\n");
}

void *worker(int start) {
	printf("Start is %d, n is %d\n", start, n);
	for (int i = start; i < n; i += 2) {
		if (get_bit(primes, i) == 0) {
			//set_bit(primes, i);
			mark_multiples(i);
		}
	}
	printf("Thread done\n");
	pthread_exit(NULL);
}


int main (int argc, char **argv) {
	n 				= strtol(argv[1], NULL, 10);
	int numthreads 	= strtol(argv[2], NULL, 10);
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
		rc = pthread_create(&threads[t], NULL, worker, (void *) base);
		if (rc) {
			printf("ERROR; return code from pthread_create() is %d \n", rc);
			exit(-1);
		}
	}
	for (int i = 0; i < numthreads; i++) {
		rc = pthread_join(threads[i], &status);
	}

	// Include 2 manually, since we don't check even numbers
	printf("Primes less than %d are: 2 ", n);
	for (int i = 3; i < n; i += 2) {
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