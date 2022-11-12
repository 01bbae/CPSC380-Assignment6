#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>


int main(int argc, char const *argv[])
{
    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    fp = fopen("./addresses.txt", "r");
    if (fp == NULL){
        exit(EXIT_FAILURE);
    }
    while ((read = getline(&line, &len, fp)) != -1)
    {
        /* code */
        printf("%s", line);
    }

    fclose(fp);
    free(line);
    
    return 0;
}
