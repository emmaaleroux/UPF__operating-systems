#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include "parsePGM.h"

#define MAX_BUFF_SIZE 1024

typedef struct {        // From Hint 2
    char* path; 
    int offset;         // Offset from the beginning of the file (including header) 
    int bytesToRead; 
    unsigned int* histogram;
    pthread_mutex_t* lock;
} ThreadInfo;

void* thread(void* st) {
    
    // Cast parameter to the correct struct type 
    ThreadInfo* info = (ThreadInfo*) st;
    // Compute histogram. This is what you will need to do in threads.
    unsigned char* buffer = malloc(MAX_BUFF_SIZE);
    int fd = open(info->path, O_RDONLY);
    if (fd < 0) {return NULL;}
    lseek(fd, info->offset, SEEK_SET); // Move the cursor to the start of the data segment

    int remaining = info->bytesToRead;

    while (remaining > 0) {
        // Read either the max buffer size or what's left for this thread
        int toRead = (remaining > MAX_BUFF_SIZE) ? MAX_BUFF_SIZE : remaining;
        int nRead = read(fd, buffer, toRead);
        if (nRead <= 0) break;

        // PROTECT SHARED DATA: Lock before updating histogram
        pthread_mutex_lock(info->lock);
        for (int i = 0; i < nRead; i++) {
            info->histogram[buffer[i]]++;
        }
        pthread_mutex_unlock(info->lock);

        remaining -= nRead;
    }

    close(fd);
    return NULL;
}


int main(int argc, char* argv[]) {

    if (argc != 4) {
        printf("Incorrect usage. Args needed: histogram pathToImage pathToHistogramOut nThreads\n");
        _exit(1);
    }

    int nThreads = atoi(argv[3]);
    pthread_t threads[nThreads];
    ThreadInfo thread_data[nThreads];

    // Read the header
    int width, height;
    int maxval;
    int nBytesHeader = parse_pgm_header(argv[1], &width, &height, &maxval);
    if (maxval > 255){
        perror("Expecting 1 byte ints\n");
        _exit(1);
    }
    int nPixels = width * height;

    int bytesToRead = nPixels / nThreads;
    // last thread: nPixels - (bytesToRead * (nThreads - 1))

    unsigned int* histogram = malloc(maxval * sizeof(unsigned int));
    for (int i = 0; i < maxval; i++) {
        histogram[i] = 0;
    }

    pthread_mutex_t lock;
    pthread_mutex_init(&lock, NULL);

    // Thread creation loop
    for (int i = 0; i < nThreads; i++) {
        thread_data[i].histogram = histogram;
        thread_data[i].lock = &lock;
        // offset for thread i: offset = nBytesHeader + (i * bytesToRead)
        thread_data[i].path = argv[1];
        thread_data[i].offset = nBytesHeader + (i * bytesToRead);
        // Distribute pixels (handle the remainder for the last thread)
        if (i == nThreads - 1) {
            thread_data[i].bytesToRead = nPixels - (i * bytesToRead);
        } else {
            thread_data[i].bytesToRead = bytesToRead;
        }
        pthread_create(&threads[i], NULL, thread, &thread_data[i]);
    }

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
    close(fd_out);

    /*
    The main reads the image metadata, 
    determines the size of the data segment (excluding the header), 
    and creates the threads.
    */


    // Hint1: Because file descriptors maintain a shared file pointer, using a single shared descriptor across threads may 
    // lead to unintended interference. For this reason, it is recommended that each thread opens the file independently. 



    return 0;
}