#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <ctype.h>

void get_arguments(int argc, char **argv);

char* directoryName;
int numberOfThreads = 0;

int main(int argc, char **argv) {

    get_arguments(argc, argv);

    DIR *directory;
    directory = opendir(directoryName);

    if (directory == NULL){ //no directory
        fprintf(stderr,"Could not open given directory.\n");
        return 0;
    }

    struct dirent *ent;

    while ((ent = readdir (directory)) != NULL) {

        if(strstr(ent->d_name,".txt") != NULL){
            FILE *file;
            char wordFull[31];
            char word[31];
            char *path;

            if((path = malloc(strlen(ent->d_name)+strlen(directoryName)+2)) != NULL){
                path[0] = '\0';
                strcat(path,directoryName);
                strcat(path,"/");
                strcat(path,ent->d_name);
            }

            file = fopen(path,"r");

            while(fscanf(file,"%s",wordFull) != EOF){

                int i;
                int c = 0;

                for (i = 0; i < strlen(wordFull); i++) {

                    if (isalnum(wordFull[i])) {
                        word[c] = wordFull[i];
                        c++;
                    }
                }
                word[c] = '\0';

            }
        }
    }
}

void get_arguments(int argc, char **argv) {

    int option;

    while ((option = getopt(argc, argv,"d:n:")) != -1) {
        switch (option) {
            case 'd' : directoryName = optarg;
                break;
            case 'n' : numberOfThreads = atoi(optarg);
                break;
            default:
                exit(EXIT_FAILURE);
        }
    }
}