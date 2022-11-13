#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#define frame_size 256
#define page_size 256
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

    //initialize TLB
    tlbent tlb[numInTLB];

    //initialize and declare page table
    u_int8_t pt[numInPageTable];
    for(int i = 0; i<numInPageTable; ++i){
        bool repeat = true;
        u_int8_t frameno;
        while(repeat){
            repeat = false;
            frameno = rand() % totalFrames;

            // check if it wasnt used already
            for(int j = 0; j < i; ++j){
                if (pt[j] == frameno){
                    repeat = true;
                }
            }
        }
        pt[i] = frameno;
    }

    fp = fopen("./addresses.txt", "r");
    if (fp == NULL){
        exit(EXIT_FAILURE);
    }
    while ((read = getline(&line, &len, fp)) != -1)
    {
        /* code */
        uint32_t addr = atoi(line);
        printf("number: %d\n", addr);

        laddr.ul = addr;
        printf("laddr bf pageno: %d\n", laddr.bf.pageno);
        printf("laddr bf offset: %d\n", laddr.bf.offset);
        if (laddr.bf.offset > 256){
            perror("Error: offset exceeds page size");
            exit(EXIT_FAILURE);
        }
        

    }



    fclose(fp);
    free(line);
    exit(EXIT_SUCCESS);
    return 0;
}

