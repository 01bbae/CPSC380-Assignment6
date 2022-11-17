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

typedef struct ptEntry{
    uint8_t frameno;
    bool valid;
} ptent;

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
    FILE* bs = fopen("BACKING_STORE.bin", "rb");
    if (!bs){
        perror("Error reading backing file.");
        exit(EXIT_FAILURE);
    }
    char* backingStore = mmap(NULL, totalPhysMem, PROT_READ, MAP_SHARED, fileno(bs), 0);
    for (int i = 0; i < totalPhysMem; ++i){
        printf("%x",backingStore[i]);
    }
    printf("\n");
    printf("%d\n",backingStore[66]);

    //initialize page table
   ptent* pt[numInPageTable];
   for(int i = 0; i < numInPageTable; ++i){
      ptent newPTent = { .valid = false };
      pt[i] = &newPTent;
   }

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
	printf("%x\n", backingStore[66]);
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
            if (!pt[laddr.bf.pageno]->valid){
                printf("FRAME DOES NOT EXIST IN PAGE TABLE\n");
                // frame does not exist in page table
                // grab from backing store
                // bring to physical memory and add to page table
                // printf("backing store: %s\n", backingStore[laddr.bf.pageno*8]);
                char* page = "";
                printf("%x", backingStore[laddr.bf.pageno]);
                for (int i = laddr.bf.pageno; i < laddr.bf.pageno + laddr.bf.offset; ++i){
                    printf("%x", backingStore[i]);
                    strcat(page, &backingStore[i]);
                }
                printf("Page Contents: %s\n", page);
                physMem[laddr.bf.pageno] = &page;
                //update page table and tlb
                frameno = laddr.bf.pageno;
                pt[laddr.bf.pageno]->frameno = frameno;
                printf("FRAME IS CREATED IN PAGE TABLE\n");
            }else{
                //frame already in page table
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
            printf("Signed value at Physical Address: %c\n", (*(physMem[frameno]))[offset]);
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
