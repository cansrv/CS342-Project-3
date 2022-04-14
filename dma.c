#include "dma.h"
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

void* p;

// Method that inserts 1 into the bitmap
void insert_1(char *bitmap, int grand_index, int index) {
    char mask = *(bitmap + grand_index);
    mask = mask | (1 << index);
    *(bitmap + grand_index) = mask;
}

// Method that inserts 0 into the bitmap
void insert_0(char *bitmap, int grand_index, int index) {
    char mask = *(bitmap + grand_index);
    char tmp = 255;
    tmp -= 1 << index;
    mask = mask & tmp;
    *(bitmap + grand_index) = mask;
}

// Method that initialized 2^n bytes of contiguous memory using mmap() system call
int dma_init(int n) {
    int size;
    size = (int) pow(2, n);
    p = mmap (NULL, (size_t) size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    printf  ("%lx\n", (long) p); // print start address
    if(p == MAP_FAILED) {
        perror("mmap");
        exit(-1);
    }
    // Filling the first 2^17 bits of the memory with 1
    int i;
    for(i = 0; i < (int) pow(2, 17); i++) {
        insert_1((char *) p, i / 8, i % 8);
    }

    // As we filled the first 2^10 words, we need to set the bitmap segment referring to be full
    insert_0((int *) p, 0, 0);
    insert_1((int *) p, 0, 1);
    for(int i = 0; i < (int) pow(2, 10) -2; i++) {
        insert_0((int *) p, (i+2) / 8,(i + 2) % 8);
    }
    return (0);
}


// Method that allocates size amount from mapped memory using mmap() system call 
void *dma_alloc(int size) {
    int modulo = size % 16;
    int mult = size / 16;
    int totalArea = mult * 16;
    if(modulo != 0)
        totalArea += 16;

    // Finding the indice big enough to store the requested size
    int i;
    for(i = 0; i < (int) pow(2, 17); i += 2) {
        // Checking if the two indices are 00 or 01
        if(*((int)* p + i) == 0 && *((int)* p + i + 1) == 0) {
            // Checking if the area is big enough to store the requested size
            if(totalArea <= *((int *) p + i + 2)) {
                // Setting the bitmap to 1
                for(int j = 0; j < totalArea; j++) {
                    insert_1((int *) p, i + j);
                }
                // Returning the start address of the allocated area
                return (p + i);
            }
        }
    }
    // Allocating the area in the memory

}