#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>

#define INITIALSIZE 8

struct mainBuffer{
    struct location* locationPtr;
    struct location* locationHead;
    char word[100];
};
typedef struct property{
    struct mainBuffer* bufferPtr;
    size_t used;
    size_t size;
}bufferProperty;
struct location{
    char fileName[100];
    char directoryName[100];
    struct location* locationPtr;
};
struct thread_parameters{
    char * fileName;
    char * directoryName;
    bufferProperty* bp;
    int *offsetPtr;
};

void get_arguments(int argc, char **argv);
int organize(int*,bufferProperty*);
void* work(void*);
void putWord(char*,struct thread_parameters*);
bufferProperty* generateBuffer(bufferProperty*);
void reallocation(bufferProperty*);
pthread_t thread[2];

//struct mainBuffer* generateBuffer();

char* directoryName;
int numberOfThreads = 0,ct=0;

int main(int argc, char **argv) {
    int offsetPtr=0;
    bufferProperty* bp=malloc(sizeof(bufferProperty));
    get_arguments(argc, argv);  //argumanları çekiyor, hata kontrolü yapabiliriz burada
    if(organize(&offsetPtr,generateBuffer(bp))==-1) exit(0);
}

bufferProperty* generateBuffer(bufferProperty* bp){
    bp->bufferPtr = malloc(INITIALSIZE*sizeof(struct mainBuffer));
    bp->used=-1;
    bp->size=INITIALSIZE;
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
int organize(int *offsetPtr,bufferProperty* bp){

    DIR *directory;
    directory = opendir(directoryName);

    if (directory == NULL){ //no directory
        fprintf(stderr,"Could not open given directory.\n");
        return -1;
    }
    int i=0;
    struct dirent *ent;
    while ((ent = readdir (directory)) != NULL) {
        struct thread_parameters parameters;

        parameters.directoryName=directoryName;
        parameters.fileName=ent->d_name;
        parameters.offsetPtr=offsetPtr;
        parameters.bp=bp;
        pthread_create  (&thread[i],  NULL,  &work,  &parameters);
        i++;
        if(i==2){
            pthread_exit(NULL);
        }
    }
}

void* work(void *parameters){
    /*Bu üç satırda threadlerin inital fonksiyonlarında kullanılması gereken argümanları
     * pass ettik, bu parametreleri bir struct'ın içine koymuştuk daha öncesinde.
     * struct mainBuffer* bizim üzerine ekleme yaptığımız buffer'ı poin ediyor.
     * */
    struct thread_parameters* tp=(struct thread_parameters*)parameters;
    char *fileName = tp->fileName;
    char *directoryName = tp->directoryName;

    if(strstr(fileName,".txt") != NULL) {     //.txt olan bütün dosyaları sırayla çek

        FILE *file;
        char wordFull[100];
        char word[100];
        char *path;

        if ((path = malloc(strlen(fileName) + strlen(directoryName) + 2)) != NULL) {   //dosya yolunu oluşturuyor
            path[0] = '\0';
            strcat(path, directoryName);
            strcat(path, "/");
            strcat(path, fileName);
        }

        file = fopen(path, "r");

        while (fscanf(file, "%s", wordFull) !=
               EOF) {   //kelimeyi çekiyor ve alpha numeric olmayan tüm karakterleri siliyor

            int i;
            int c = 0;

            for (i = 0; i < strlen(wordFull); i++) {

                if (isalnum(wordFull[i])) {
                    word[c] = wordFull[i];
                    c++;
                }
            }
            word[c] = '\0';
            //kelime 'word' elimizde


            tp->bp->used++;
            if(tp->bp->used==tp->bp->size){
                reallocation(tp->bp);
            }

            putWord(word,tp);

        }
    }
}
void putWord(char *word,struct thread_parameters* parameters){
    bufferProperty* bp=parameters->bp;
    strcpy((bp->bufferPtr)[bp->used].word,word);
    if(bp->bufferPtr->locationHead==NULL){
        bp->bufferPtr->locationPtr=malloc(sizeof(struct location));
        bp->bufferPtr->locationHead=bp->bufferPtr->locationPtr;
    }else{
        bp->bufferPtr->locationPtr->locationPtr=malloc(sizeof(struct location));
        bp->bufferPtr->locationPtr=bp->bufferPtr->locationPtr->locationPtr;
    }
    strcpy(bp->bufferPtr->locationPtr->fileName,parameters->fileName);
    strcpy(bp->bufferPtr->locationPtr->directoryName,parameters->directoryName);

    printf("Thread:%d--- %d word:%4s filename:%4s dname:%4s\n",(int)pthread_self(),bp->size,(bp->bufferPtr)[bp->used].word,bp->bufferPtr->locationPtr->fileName,bp->bufferPtr->locationPtr->directoryName);
    bp->bufferPtr->locationPtr->fileName;
}
void reallocation(bufferProperty *bp){
    bp->size*=2;
    bp->bufferPtr = (struct mainBuffer*)realloc(bp->bufferPtr, bp->size * sizeof(struct mainBuffer));
}

