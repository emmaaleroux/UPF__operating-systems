// OPERATING SYSTEMS P101 - LAB 4
// EMMA LEROUX 304174 & GUILLEM ARÉVALO 306124

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include "parsePGM.h"


int main(int argc, char* argv[]) {

    if (argc != 6) {
        printf("Incorrect usage. Args needed: computeHistogram pathToImage pathToHistogramOut nProducers nConsumers sizeBuffer\n");
        _exit(1);
    }


    return 0;
}