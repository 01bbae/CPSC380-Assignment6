#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>

#define frameSize 256
#define pageSize 256
#define totalFrames 256
#define numInTLB 16
#define numInPageTable 256
#define totalPhysMem 65536

typedef struct bitfield{
    uint32_t offset:8;
    uint32_t pageno:8;
    uint32_t unused:16;
} logAddr;

typedef union {
    uint32_t ul;
    logAddr bf;
} u_logAddr;

typedef struct TLBentry{
    uint8_t pageno;
    uint8_t frameno;
} tlbent;

int main(int argc, char const *argv[])
{
    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    u_logAddr laddr;

    // for statistics
    int pageHits = 0;
    int pageMisses = 0;

    //initialize TLB
    tlbent* tlb[numInTLB];
    int tlbstart = 0; // for keeping track of fifo queue start
    int tlbend = 0; // for keeping track of end of queue to add element

    //initialize and declare physical memory
    char** physMem[totalFrames];

    //initialize backing store
    int fd = open("BACKING STORE.bin", O_RDONLY);
    char* backingStore = mmap(NULL, pageSize, PROT_READ, MAP_SHARED, fd, 0);

    //initialize page table
    uint8_t* pt[numInPageTable];
    // for(int i = 0; i<numInPageTable; ++i){
    //     bool repeat = true;
    //     u_int8_t frameno;
    //     // randomly order frame numbers in page table
    //     while(repeat){
    //         repeat = false;
    //         frameno = rand() % totalFrames;
    //         // check if it was used already
    //         for(int j = 0; j < i; ++j){
    //             if (pt[j] == frameno){
    //                 repeat = true;
    //             }
    //         }
    //     }
    //     pt[i] = frameno;
    // }

    fp = fopen("./addresses.txt", "r");
    if (fp == NULL){
        exit(EXIT_FAILURE);
    }
    while ((read = getline(&line, &len, fp)) != -1)
    {
        uint32_t addr = atoi(line);
        printf("number: %d\n", addr);

        char* output;

        laddr.ul = addr;
        printf("laddr bf pageno: %d\n", laddr.bf.pageno);
        printf("laddr bf offset: %d\n", laddr.bf.offset);
        if (laddr.bf.offset > 256){
            perror("Error: offset exceeds page size");
            exit(EXIT_FAILURE);
        }
        // search tlb for page hit
        uint8_t frameno; // translate pageno to frameno
        uint8_t offset = laddr.bf.offset; // offset is directly translated
        bool inTLB = false;
        for (int i = 0; i < tlbend; ++i){ // search until the end of tlb for pageno
            if (tlb[i]->pageno == laddr.bf.pageno){
                inTLB = true;
                frameno = tlb[i]->frameno;
                pageHits++;
            }
        }
        if (!inTLB){
            printf("frameno not in tlb\n");
            pageMisses++;
            // acquire from page table
            if (pt[laddr.bf.pageno] == NULL){
                printf("FRAME DOES NOT EXIST IN PAGE TABLE");
                // frame does not exist in page table
                // grab from backing store
                // bring to physical memory and add to page table
                physMem[laddr.bf.pageno] = backingStore[laddr.bf.pageno];
                //update page table and tlb
                frameno = laddr.bf.pageno;
                pt[laddr.bf.pageno] = frameno;
            }else{
                //frame in page table
                frameno = pt[laddr.bf.pageno];
            }

            // put page into tlb
            // check if tlb is full first
            bool tlbfull = true;
            for (int i = 0; i < numInTLB; ++i){
                if (tlb[i] == NULL){
                    tlbfull = false;
                }
            }
            // create new tlb entry
            tlbent newtlbent;
            newtlbent.frameno = frameno;
            newtlbent.pageno = laddr.bf.pageno;
            if(tlbfull){
                // replace one from tlb
                tlb[tlbstart] = &newtlbent; // free mem first?
                tlbstart = (tlbstart + 1) % numInTLB;
            }else{
                // add to tlb
                tlb[tlbend] = &newtlbent;
                tlbend = (tlbend + 1) % numInTLB; // not sure if necessary
            }
            printf("add to tlb complete\n");

            // create output
            printf("Logical Address: %d\n", addr);
            printf("Logical Address as Page Number and offset: %d + %d\n", laddr.bf.pageno, laddr.bf.offset);
            printf("Physical Address as Frame Number and offset: %d + %d\n", frameno, offset);
            printf("Signed value at Physical Address: %s\n", (*(physMem[frameno]))[offset]);
        }
    }
    printf("Statistics:\n");
    printf("Total Page Hits: %d\n",pageHits);
    printf("Total Page Misses: %d\n",pageMisses);


    fclose(fp);
    free(line);
    exit(EXIT_SUCCESS);
    return 0;
}
