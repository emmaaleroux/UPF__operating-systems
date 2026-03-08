// OPERATING SYSTEMS P101 - LAB 4
// EMMA LEROUX 304174 & GUILLEM ARÉVALO 306124

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include "parsePGM.h"

#define BLOCK_SIZE 1024*16


// CIRCULAR BUFFER
typedef struct {
    unsigned char **blocks; // array of pointers to blocks
    int *sizes; // size of each block (for EOF handling)
    int capacity; // number of slots
    int count; // current number of full blocks in buffer
    int head; // consumer reads from head
    int tail; // producer writes to tail
    pthread_mutex_t lock;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} CircularBuffer;


// GLOBAL VARIABLES
CircularBuffer cb;  
char* inputPath; 
int maxval;
unsigned int* histogram;
pthread_mutex_t histLock; // Protects histogram
int offset;
pthread_mutex_t offsetLock; // Protects offset
// TERMINATION
int active_producers = 0; // Producer counter
int producers_finished = 0; // Flag
pthread_mutex_t termLock; // Protects termination variables


// PRODUCER THREADS
void *producer(void *arg) {
    (void)arg; // We use global variables instead of parameters

    int fd = open(inputPath, O_RDONLY);
    if(fd < 0){
        perror("open");
        return NULL;
    }
    
    while (1) {
        unsigned char *block = malloc(BLOCK_SIZE);

        pthread_mutex_lock(&offsetLock);
        lseek(fd, offset, SEEK_SET);
        int n = read(fd, block, BLOCK_SIZE);
        if (n > 0) {
            offset += n;
        }
        pthread_mutex_unlock(&offsetLock);
        if (n <= 0) {
            free(block);
            break; // Producers finish when EOF (n <= 0)
        }
        // Insert into buffer
        pthread_mutex_lock(&cb.lock);
        while (cb.count == cb.capacity) {
            // We avoid starvation because pthread_cond_wait() puts to sleep
            pthread_cond_wait(&cb.not_full, &cb.lock);
        }
        cb.blocks[cb.tail] = block;
        cb.sizes[cb.tail] = n; // n = block unless EOF
        cb.tail = (cb.tail + 1) % cb.capacity;
        cb.count++;

        pthread_cond_signal(&cb.not_empty);
        pthread_mutex_unlock(&cb.lock);
    }

    // Decrement active producer count
    pthread_mutex_lock(&termLock);
    active_producers--;
    // The last producer wakes everyone up to finish
    if (active_producers == 0) {
        producers_finished = 1;
        pthread_mutex_lock(&cb.lock);
        pthread_cond_broadcast(&cb.not_empty); // We wake up all consumers
        pthread_mutex_unlock(&cb.lock);
    }
    pthread_mutex_unlock(&termLock);

    close(fd);
    return NULL;
}

// CONSUMER THREADS
void *consumer(void *arg) {
    (void)arg; // We use global variables instead of passing parameters

    // Local histogram
    unsigned int* local_hist = calloc(maxval + 1, sizeof(unsigned int));

    while(1){
        unsigned char* block = NULL;
        int blockSize = 0;

        pthread_mutex_lock(&cb.lock);
        // Wait if buffer is empty AND producers are still working
        while(cb.count == 0) {
            pthread_mutex_lock(&termLock);
            int finished = producers_finished;
            pthread_mutex_unlock(&termLock);

            if (finished) break; 
            pthread_cond_wait(&cb.not_empty, &cb.lock);
        }

        // Final check: if empty and no one is producing, exit
        if(cb.count == 0) {
            pthread_mutex_unlock(&cb.lock);
            break;
        }

        block = cb.blocks[cb.head]; // Take corresponding block
        blockSize = cb.sizes[cb.head]; // Check its size
        cb.head = (cb.head + 1) % cb.capacity; // Increment block index
        cb.count--; // Decrement number of full blocks

        pthread_cond_signal(&cb.not_full);
        pthread_mutex_unlock(&cb.lock);

        // Update local histogram
        for(int i = 0; i < blockSize; i++) { local_hist[block[i]]++; }

        free(block);
    }

    // Merge local result into global histogram
    pthread_mutex_lock(&histLock);
    for(int i = 0; i <= maxval; i++) {
        histogram[i] += local_hist[i];
    }
    pthread_mutex_unlock(&histLock);

    free(local_hist);
    return NULL;
}


int main(int argc, char* argv[]) {

    if (argc != 6) {
        printf("Incorrect usage. Args needed: computeHistogram pathToImage pathToHistogramOut nProducers nConsumers sizeBuffer\n");
        return 1;
    }

    // Save arguments
    inputPath = argv[1];
    char* outPath = argv[2];
    int nProd = atoi(argv[3]);
    int nCons = atoi(argv[4]);
    int buffSize = atoi(argv[5]);

    // We read the header, from sequential
    int width, height;
    int nBytesHeader = parse_pgm_header(inputPath, &width, &height, &maxval);
    if (maxval > 255){
        perror("Expecting 1 byte ints\n");
        return 1;
    }
    offset = nBytesHeader; 

    // We initialize the circular buffer data
    cb.blocks = malloc(buffSize * sizeof(unsigned char*));
    cb.sizes  = malloc(buffSize * sizeof(int));
    cb.capacity = buffSize;
    cb.count = 0; cb.head = 0; cb.tail = 0;
    pthread_mutex_init(&cb.lock, NULL);
    pthread_cond_init(&cb.not_empty, NULL);
    pthread_cond_init(&cb.not_full, NULL);

    // We initialize the histogram
    histogram = calloc(maxval, sizeof(unsigned int));
    // We initialize the locks
    pthread_mutex_init(&histLock, NULL);  
    pthread_mutex_init(&offsetLock, NULL);
    pthread_mutex_init(&termLock, NULL);

    active_producers = nProd;

    // We initialize the arrays of threads and we create the threads
    pthread_t prodThreads[nProd];
    for (int i = 0; i < nProd; i++) {
        pthread_create(&prodThreads[i], NULL, producer, NULL);
    }
    pthread_t consThreads[nCons];
    for(int i = 0; i < nCons; i++){
        pthread_create(&consThreads[i], NULL, consumer, NULL);
    }

    // We wait for all threads to finish
    for(int i = 0; i < nProd; i++) { pthread_join(prodThreads[i], NULL); }
    for(int i = 0; i < nCons; i++) { pthread_join(consThreads[i], NULL); }

    // We write into the output file
    int fd_out = open(outPath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    char s[80];
    for (int i = 0; i < maxval; i++) {
        sprintf(s, "%d,%u\n", i, histogram[i]);
        write(fd_out, s, strlen(s));
    }

    // We close and free everything
    close(fd_out);
    free(histogram);
    free(cb.blocks);
    free(cb.sizes);
    return 0;

}