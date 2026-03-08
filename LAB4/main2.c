// OPERATING SYSTEMS P101 - LAB 5
// EMMA LEROUX 304174 & GUILLEM ARÉVALO 306124

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "parsePGM.h"

#define BLOCK_SIZE (1024 * 16)
#define HIST_SIZE 256

// BUFFER
unsigned char *buffer[1024]; // circular buffer
int head = 0, tail = 0, count = 0;
pthread_mutex_t buffer_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t not_full = PTHREAD_COND_INITIALIZER;
pthread_cond_t not_empty = PTHREAD_COND_INITIALIZER;

// HISTOGRAM 
int nPixels = width * height;
int bytesToRead = nPixels / nThreads;
unsigned int* histogram = malloc((maxval) * sizeof(unsigned int));
for (int i = 0; i < maxval; i++) {
    histogram[i] = 0;
}
pthread_mutex_t histLock;
pthread_mutex_init(&histLock, NULL);

// ----------------- File reading -----------------
int readPos = 0; // starts after header
pthread_mutex_t lock_read;
pthread_mutex_init(&histLock, NULL);
int active_producers = 0;
FILE *inputFile;
long fileSize;

// ----------------- Producer -----------------
void *Producer(void *arg)
{
    char *path = (char *)arg;
    int fd = open(path, O_RDONLY);
    if (fd < 0)
    {
        perror("open");
        return NULL;
    }

    while (1)
    {
        int readPosLocal;

        // 1️⃣ Reserve a block from file
        pthread_mutex_lock(&lock_read);
        readPosLocal = readPos;
        readPos += BLOCK_SIZE;
        pthread_mutex_unlock(&lock_read);

        // 2️⃣ Read the block
        unsigned char *block = malloc(BLOCK_SIZE);
        int nBytesRead = pread(fd, block, BLOCK_SIZE, readPosLocal);
        if (nBytesRead <= 0)
        {
            free(block);
            break;
        }

        // 3️⃣ Insert block into buffer
        pthread_mutex_lock(&buffer_mutex);
        while (count == 1024)
            pthread_cond_wait(&not_full, &buffer_mutex);

        buffer[tail] = block;
        tail = (tail + 1) % 1024;
        count++;

        pthread_cond_signal(&not_empty);
        pthread_mutex_unlock(&buffer_mutex);
    }

    close(fd);

    // 4️⃣ Signal consumers if this was the last producer
    pthread_mutex_lock(&buffer_mutex);
    active_producers--;
    if (active_producers == 0)
        pthread_cond_broadcast(&not_empty);
    pthread_mutex_unlock(&buffer_mutex);

    return NULL;
}

// ----------------- Consumer -----------------
void *Consumer(void *arg)
{
    (void)arg;
    while (1)
    {
        unsigned char *block;

        pthread_mutex_lock(&buffer_mutex);
        while (count == 0 && active_producers > 0)
            pthread_cond_wait(&not_empty, &buffer_mutex);

        if (count == 0 && active_producers == 0)
        {
            pthread_mutex_unlock(&buffer_mutex);
            break; // nothing more to consume
        }

        block = buffer[head];
        head = (head + 1) % 1024;
        count--;

        pthread_cond_signal(&not_full);
        pthread_mutex_unlock(&buffer_mutex);

        // 2️⃣ Accumulate histogram
        pthread_mutex_lock(&hist_mutex);
        for (int i = 0; i < BLOCK_SIZE; i++)
            histogram[block[i]]++;
        pthread_mutex_unlock(&hist_mutex);

        free(block);
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc != 6)
    {
        printf("Incorrect usage. Args needed: computeHistogram pathToImage pathToHistogramOut nProducers nConsumers sizeBuffer\n");
        exit(1);
    }

    char *inputPath = argv[1];
    char *outputPath = argv[2];
    int nProd = atoi(argv[3]);
    int nCons = atoi(argv[4]);
    int bufferSize = atoi(argv[5]);

    // Open file and skip PGM header
    int width, height;
    int maxval;
    int nBytesHeader = parse_pgm_header(argv[1], &width, &height, &maxval);
    if (maxval > 255)
    {
        perror("Expecting 1 byte ints\n");
        exit(1);
    }

    inputFile = fopen(inputPath, "rb");
    if (!inputFile)
    {
        perror("fopen");
        exit(1);
    }
    fseek(inputFile, 0, SEEK_END);
    fileSize = ftell(inputFile);
    readPos = nBytesHeader;
    fseek(inputFile, nBytesHeader, SEEK_SET);

    active_producers = nProd;

    pthread_t prodThreads[nProd];
    pthread_t consThreads[nCons];

    // Start producer threads
    for (int i = 0; i < nProd; i++)
        pthread_create(&prodThreads[i], NULL, Producer, inputPath);

    // Start consumer threads
    for (int i = 0; i < nCons; i++)
        pthread_create(&consThreads[i], NULL, Consumer, NULL);

    // Join threads
    for (int i = 0; i < nProd; i++)
        pthread_join(prodThreads[i], NULL);
    for (int i = 0; i < nCons; i++)
        pthread_join(consThreads[i], NULL);

    // Write histogram to file
    FILE *out = fopen(outputPath, "w");
    if (!out)
    {
        perror("fopen");
        exit(1);
    }
    for (int i = 0; i < HIST_SIZE; i++)
        fprintf(out, "%d,%d\n", i, histogram[i]);
    fclose(out);

    fclose(inputFile);
    return 0;
}