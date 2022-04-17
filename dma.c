#include "dma.h"
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

void *starting_pointer;
int segment_size;
int internal_fragmentation;
// Method that inserts 1 into the bitmap
void insert_1(char *bitmap, long grand_index, long index)
{
    char mask = *(bitmap + grand_index);
    mask = mask | (1 << (7 - index));
    *(bitmap + grand_index) = mask;
}

// Method that inserts 0 into the bitmap
void insert_0(char *bitmap, long grand_index, long index)
{
    char mask = *(bitmap + grand_index);
    char tmp = 255;
    tmp -= 1 << (7 - index);
    mask = mask & tmp;
    *(bitmap + grand_index) = mask;
}

// Method that initialized 2^n bytes of contiguous memory using mmap() system call
int dma_init(int n)
{
    segment_size = n;
    internal_fragmentation = 0;
    int size;
    size = (int) pow(2, n);
    starting_pointer = mmap(NULL, (size_t)size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    // printf("%lx\n", (long)starting_pointer); // print start address
    if (starting_pointer == MAP_FAILED)
    {
        perror("mmap");
        exit(-1);
    }
    // Filling the first 2^segment_size/8 bits of the memory with 1
    int i;
    for (i = 0; i < (int) pow(2, segment_size - 3); i++)
    {
        insert_1((char *)starting_pointer, i / 8, i % 8);
    }
    // As we filled the first 2^10 words, we need to set the bitmap segment referring to be full
    insert_0((char *)starting_pointer, 0, 0);
    insert_1((char *)starting_pointer, 0, 1);
    for (int i = 2; i < (int)pow(2, segment_size - 9) + 32 /*reserved spaces' bits*/; i++)
    {
        insert_0((char *)starting_pointer, (i) / 8, (i) % 8);
    }

    return (0);
}

// Method that allocates size amount from mapped memory using mmap() system call
void *dma_alloc(int size)
{

    // Calculating the size needed to be allocated
    int modulo = size % 16;
    int num_of_pages = size / 16;
    // If the size is not a multiple of 16, we need to allocate more memory
    if (modulo != 0) {
        internal_fragmentation += (16 - modulo);
        num_of_pages++;
    }
    // We need to check if number of allocated pages are multiple of 2
    if (num_of_pages % 2 != 0) {
        internal_fragmentation += 16;
        num_of_pages++;
    }
    // Searching for empty sequences in bitmap
    int area_found = 0;
    for (int i = (int)pow(2, segment_size -9) /*bitmap*/ + 32 /*reserved*/; i < (int)pow(2, segment_size - 3); i += 2)
    {

        char current_char = *((char *)starting_pointer + i / 8);
        int in_char_index = i % 8;
        unsigned int b0 = (unsigned int) (current_char >> (7 - in_char_index)) % 2;
        unsigned int b1 = (unsigned int) (current_char >> (6 - in_char_index)) % 2;
        
        if (b0 && b1) {
            area_found +=2;
        }
        else
            area_found = 0;
        // printf("i: %d, current_char: %hhu, b0: %d, b1: %hhu , area_found:%d\n", i, current_char, b0, b1, area_found); 
        if(area_found == num_of_pages) {
            long starting_index = (long) (i - area_found + 2);

            insert_0((char *) starting_pointer, starting_index / 8, starting_index % 8); 
            insert_1((char *) starting_pointer, (starting_index + 1) / 8, (starting_index + 1) % 8);
            for(long j = starting_index + 2; j < i + 2; j++) {
                insert_0((char *) starting_pointer, j / 8, j % 8);
                //printf("j: %ld\n", j);
            }
            return (char*) (starting_pointer + (i - area_found) * 8);
        }
    }
    // If we didn't find a sequence of empty pages, we return NULL
    return NULL;
}

void dma_free(void *p)
{
    
    // Calculating the location of the pointer in the bitmap
    unsigned long tmp = (unsigned long)p - (unsigned long)starting_pointer;
    unsigned long index = tmp/8 + 2;
    
    unsigned long end_index = index;
    // Searching for full sequences in bitmap
    int area_found = 0;

    for(int i = (int) index; i < (int)pow(2, segment_size - 3); i += 2) {
        char current_char = *((char *)starting_pointer + i / 8);
        int in_char_index = i % 8;
        unsigned int b0 = (unsigned int) (current_char >> (7 - in_char_index)) % 2;
        unsigned int b1 = (unsigned int) (current_char >> (6 - in_char_index)) % 2;

        // printf("i: %d, current_char: %hhu, b0: %d, b1: %hhu , area_found:%d\n", i, current_char, b0, b1, area_found); 
        if((!b0 && !b1) || (!b0 && b1)) {
            area_found += 2;
        }
        else {
            end_index = i;
            break;
        }
    }

    
    for(int i = index; i < end_index; i++) {
        insert_1((char *)starting_pointer, i / 8, i % 8);
    }

    
}

void dma_print_bitmap()
{
    for(int i = 0; i < (int)pow(2, segment_size - 6); i++) {
        char current_char = *((char *)starting_pointer + i);
        // Changing current_char to binary
        unsigned int b0 = (unsigned int) (current_char >> 7) % 2;
        unsigned int b1 = (unsigned int) (current_char >> 6) % 2;
        unsigned int b2 = (unsigned int) (current_char >> 5) % 2;
        unsigned int b3 = (unsigned int) (current_char >> 4) % 2;
        unsigned int b4 = (unsigned int) (current_char >> 3) % 2;
        unsigned int b5 = (unsigned int) (current_char >> 2) % 2;
        unsigned int b6 = (unsigned int) (current_char >> 1) % 2;
        unsigned int b7 = (unsigned int) (current_char >> 0) % 2;
        printf("%hhu%hhu%hhu%hhu%hhu%hhu%hhu%hhu ", b0, b1, b2, b3, b4, b5, b6, b7);
        if(i % 8 == 7)
            printf("\n");
    }
    printf("\n");
}

char hex_converter(int i) {
    switch (i)
    {
    case 0:
        return '0';
    case 1:
        return '1';
    case 2:
        return '2';
    case 3:
        return '3';
    case 4:
        return '4';
    case 5:
        return '5';
    case 6: 
        return '6';
    case 7:
        return '7';
    case 8:
        return '8';
    case 9:
        return '9';
    case 10:
        return 'A';
    case 11:
        return 'B';
    case 12:   
        return 'C';
    case 13:   
        return 'D';
    case 14:    
        return 'E';
    case 15:
        return 'F';
    default:
        return '0';
    }
}

void dma_print_page(int pno) {
    // Each page is 4096 bytes long
    char *p = (char *)starting_pointer;
    p = p + pno * 4096;
    for(int i = 0; i < 4096; i+=2) {
        unsigned int b0 = (unsigned int) *(p + i);
        b0 = b0 >> 4;
        b0 %= 16;
        unsigned int b1 = (unsigned int)*(p + i);
        b1 %= 16;
        printf("%c%c", hex_converter(b0), hex_converter(b1));
        if(i % 64 == 63)
            printf("\n");
    }
    printf("\n");

}

void dma_print_blocks() {
    for(int i = 0; i < (int)pow(2, segment_size - 3); i+= 2) {
        char current_char = *((char *)starting_pointer + i);
        // Changing current_char to binary
        unsigned int b0 = (unsigned int) (current_char >> 7) % 2;
        unsigned int b1 = (unsigned int) (current_char >> 6) % 2;
        
        if(!b0 && b1) {
             for(int j = 0; (int) pow(2, segment_size -3) - i; j += 2) {
                
                char current_char = *((char *)starting_pointer + j + i);
                // Changing current_char to binary
                unsigned int b0 = (unsigned int) (current_char >> 7) % 2;
                unsigned int b1 = (unsigned int) (current_char >> 6) % 2;
                if(b0 || b1) {
                    printf("A, 0x%lx,\t0x%x\t0x (%d)\n", (unsigned long)starting_pointer + i, (j + 1) * 16, (j + 1) * 16);
                    break;
                }
                i+=2;
                
            }
        }
        if(b0 && b1) {
            for(int j = 0; (int) pow(2, segment_size -3) - i; j += 2) {
                
                char current_char = *((char *)starting_pointer + j + i);
                // Changing current_char to binary
                unsigned int b0 = (unsigned int) (current_char >> 7) % 2;
                unsigned int b1 = (unsigned int) (current_char >> 6) % 2;
                if(!b0 || !b1) {
                    printf("F, 0x%lx,\t0x%x\t0x (%d)\n", (unsigned long)starting_pointer + i, (j + 1) * 16, (j + 1) * 16);
                    break;
                }
                i+=2;
                
            }
        }
        
    }
    printf("\n");
}

int dma_give_intfrag() {
    return internal_fragmentation;
}

int main()
{
    dma_init(14);
    //dma_print_bitmap();
    void *p = dma_alloc(106);
    dma_print_bitmap();
    dma_print_blocks();
    //dma_print_bitmap();
    if(p == NULL) {
        printf("NULL\n");
    }
    else {
        printf("%lx\n", (long)p);
        dma_free(p);
    }
    //dma_print_bitmap();
    return 0;
}