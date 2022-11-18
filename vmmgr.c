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
#define TLBSize 16
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
    int rowCount = 0;

    int pageHitpageNo[10][3];

    //initialize TLB
    tlbent tlb[TLBSize];
    bool TLBFull = false;
    int tlbstart = -1; // for keeping track of fifo queue start
    int tlbend = -1; // for keeping track of end of queue to add element

    //initialize and declare physical memory
    char* physMem[totalFrames];
    int nextPhysMem = 0;

    //initialize backing store
    FILE* bs = fopen("BACKING_STORE.bin", "rb");
    if (!bs){
        perror("Error reading backing file.");
        exit(EXIT_FAILURE);
    }
    char* backingStore = mmap(NULL, totalPhysMem, PROT_READ, MAP_SHARED, fileno(bs), 0);
    if (backingStore == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
    // int count = 0;
    // for (int i = 0; i < totalPhysMem; ++i){
    //     printf("%x",backingStore[i]);
    //     count++;
    // }
    // printf("\n%d\n", count);

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
        // printf("number: %d\n", addr);
        rowCount++;

        char* output;

        laddr.ul = addr;
        // printf("laddr bf pageno: %d\n", laddr.bf.pageno);
        // printf("laddr bf offset: %d\n", laddr.bf.offset);
        if (laddr.bf.offset > 256){
            perror("Error: offset exceeds page size");
            exit(EXIT_FAILURE);
        }
        // search tlb for page hit
        uint8_t frameno; // translate pageno to frameno
        uint8_t offset = laddr.bf.offset; // offset is directly translated
        bool inTLB = false;
        for (int i = tlbstart; i != tlbend; i = (i+1)%TLBSize){ // search until the end of tlb for pageno
            // printf("%d pageno: %d, frameno: %d\n", i, tlb[i].pageno, tlb[i].frameno);
            if (tlb[i].pageno == laddr.bf.pageno){
                inTLB = true;
                frameno = tlb[i].frameno; //if its in TLB get frameno

            }
        }
        if (!inTLB){
            // printf("frameno not in tlb\n");
            pageMisses++;
            // acquire from page table
            if (!pt[laddr.bf.pageno]->valid){
                // frame does not exist in page table
                // grab from backing store
                // bring to physical memory and add to page table

                char page[pageSize];
                for (int i = 0; i < pageSize; ++i){
                    page[i] = backingStore[i + (laddr.bf.pageno*pageSize)];
                }
                if (sizeof(page) != 256){
                    exit(EXIT_FAILURE);
                }
                // put page into physical memory
                physMem[nextPhysMem] = page;
                //update page table and tlb
                frameno = nextPhysMem;
                nextPhysMem++;
                pt[laddr.bf.pageno]->frameno = frameno;
            }else{
                //frame already in page table
            }

            // put page into tlb
            tlbent newtlbent;
            newtlbent.frameno = frameno;
            newtlbent.pageno = laddr.bf.pageno;
            TLBFull = (tlbstart == tlbend + 1) || (tlbstart == 0 && tlbend == TLBSize - 1) ? true : false;
            if(TLBFull){ //chek if tlb full
                // replace one from tlb
                tlb[tlbstart] = newtlbent; // free mem first?
                tlbstart = (tlbstart + 1) % TLBSize;
                tlbend = (tlbend + 1) % TLBSize;
            }else{
                // add to tlb
                if(tlbstart == -1){
                    tlbstart = 0;
                }
                tlbend = (tlbend + 1) % TLBSize;
                tlb[tlbend] = newtlbent;
            }
        }else{
            pageHits++;
        }

        // create output
        printf("Logical Address: %d\n", addr);
        printf("Logical Address as Page Number and offset: %d + %d\n", laddr.bf.pageno, offset);
        printf("Physical Address as Frame Number and offset: %d + %d\n", frameno, offset);
        printf("Hex value at Physical Address: %x\n", (char)(physMem[frameno][offset]));
        printf("\n");
    }

    printf("Statistics:\n");
    printf("Total Page Hits: %d\n",pageHits);
    printf("Total Page Misses: %d\n",pageMisses);
    printf("Row Count: %d\n",rowCount);

    printf("Page Hits Percentage: %f%%\n",(float)pageHits/rowCount * 100);
    printf("Page Misses Percentage: %f%%\n",(float)pageMisses/rowCount * 100);


    fclose(fp);
    free(line);
    exit(EXIT_SUCCESS);
    return 0;
}
