// OPERATING SYSTEMS P101 - LAB 2
// EMMA LEROUX 304174 & GUILLEM ARÉVALO 306124

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include "parsePGM.h"

#define BUFF_SIZE 1024

// Thread information (parameter)
typedef struct {  
    char* path; 
    int offset; // Offset from the beginning of the file (including header) 
    int bytesToRead; 
    unsigned int* histogram;
    pthread_mutex_t* lock;
} ThreadInfo;


void* thread(void* st) {
    
    // Cast parameter to correct struct type 
    ThreadInfo* info = (ThreadInfo*) st;
    // Compute histogram
    unsigned char buffer[BUFF_SIZE];
    int fd = open(info->path, O_RDONLY); // Each thread opens the file independently. 
    if (fd < 0) {return NULL;}
    lseek(fd, info->offset, SEEK_SET); // Move the cursor to the start of the data segment

    int nBytesRead;
    for (int i = 0; i < info->bytesToRead; i += BUFF_SIZE) {
        // Compute what is left to read
        int toRead;
        if (info->bytesToRead - i < BUFF_SIZE) {
            // If at the end, only read the remaining bytes
            toRead = info->bytesToRead - i;
        } else { toRead = BUFF_SIZE; }
        
        nBytesRead = read(fd, buffer, toRead);
        if (nBytesRead <= 0) break;

        // Mutex to avoid race conditions 
        pthread_mutex_lock(info->lock);
        for (int j = 0; j < nBytesRead; j++) {
            info->histogram[buffer[j]]++;
        }
        pthread_mutex_unlock(info->lock);
    }

    close(fd);
    return NULL;
}


int main(int argc, char* argv[]) {

    if (argc != 4) {
        printf("Incorrect usage. Args needed: histogram pathToImage pathToHistogramOut nThreads\n");
        _exit(1);
    }

    // We initialize the array of threads and of threads parameters (structs)
    int nThreads = atoi(argv[3]);
    pthread_t threads[nThreads];
    ThreadInfo thread_data[nThreads];

    // We read the header
    int width, height;
    int maxval;
    int nBytesHeader = parse_pgm_header(argv[1], &width, &height, &maxval);
    if (maxval > 255){
        perror("Expecting 1 byte ints\n");
        _exit(1);
    }
    // We create the histogram
    int nPixels = width * height;
    int bytesToRead = nPixels / nThreads;
    unsigned int* histogram = malloc((maxval) * sizeof(unsigned int));
    for (int i = 0; i < maxval; i++) {
        histogram[i] = 0;
    }

    // We initialize the lock
    pthread_mutex_t lock;
    pthread_mutex_init(&lock, NULL);

    // Thread creation loop
    for (int i = 0; i < nThreads; i++) {
        thread_data[i].histogram = histogram;
        thread_data[i].lock = &lock;
        thread_data[i].path = argv[1];
        thread_data[i].offset = nBytesHeader + (i * bytesToRead);
        // Hhandle remainder pixels for the last thread
        if (i == nThreads - 1) {
            thread_data[i].bytesToRead = nPixels - (i * bytesToRead);
        } else {
            thread_data[i].bytesToRead = bytesToRead;
        }
        pthread_create(&threads[i], NULL, thread, &thread_data[i]);
    }
    // We wait for all threads to finish
    for (int i = 0; i < nThreads; i++) {
        pthread_join(threads[i], NULL);
    }

    // Write histogram, so it can be loaded in python with np.loadtxt
    int fd_out = open(argv[2], O_WRONLY | O_CREAT, 0644);
    for (int i = 0; i < maxval; i++) {
        char s[80];
        sprintf(s, "%d,%d\n", i, histogram[i]);
        write(fd_out, s, strlen(s));
    }

    pthread_mutex_destroy(&lock);
    free(histogram);
    close(fd_out);

    return 0;
}