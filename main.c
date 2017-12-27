#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <semaphore.h>
#include <zconf.h>


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
    int finished,filecount;
}bufferProperty;
struct location{
    char fileName[256];
    char directoryName[100];
    struct location* locationPtr;
};
struct thread_parameters{
    char fileName[256];
    char directoryName[100];
    char word[100];
    bufferProperty* bp;
    size_t offsetPtr;
    int i,available;
    pthread_t threadid;

    struct thread_parameters* others;
};

void get_arguments(int argc, char **argv);
int organize(int*,bufferProperty*);
void* work(void*);
void putWord(char*,struct thread_parameters*);
bufferProperty* generateBuffer(bufferProperty*);
void reallocation(bufferProperty*);

sem_t sem,sem2,sem3,re,assign;

//struct mainBuffer* generateBuffer();

char* directoryName;
int numberOfThreads = 0,ct=0;
int main(int argc, char **argv) {
    int offsetPtr=0;

    bufferProperty* bp=malloc(sizeof(bufferProperty));
    get_arguments(argc, argv);  //argumanları çekiyor, hata kontrolü yapabiliriz burada

    sem_init(&sem, 0, 1);
    sem_init(&sem2, 0, numberOfThreads);
    sem_init(&sem3, 0, 1);
    sem_init(&re, 0, 1);
    sem_init(&assign, 0, 1);

    if(organize(&offsetPtr,generateBuffer(bp))==-1) exit(0);
}

bufferProperty* generateBuffer(bufferProperty* bp){
    bp->bufferPtr = calloc(3,INITIALSIZE*sizeof(struct mainBuffer));
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
    printf("MAIN THREAD: Allocated initial array size to 8 pointers\n");
    DIR *directory;
    directory = opendir(directoryName);

    if (directory == NULL){ //no directory
        fprintf(stderr,"Could not open given directory.\n");
        return -1;
    }


    int i;
    struct dirent *ent;
    struct thread_parameters parameters[numberOfThreads];
    for(i=0;i<numberOfThreads;i++){
        parameters[i].available=1;
        parameters[i].i=-1;
        parameters[i].others=parameters;
    }
    i=0;
    pthread_t thread[numberOfThreads];
    bp->finished=0;
    bp->filecount=0;
    while (1) {
        if(i==numberOfThreads) i=0;
        if(parameters[i].available==1) {
            ent = readdir (directory);
            if(ent==NULL){
                bp->finished=1;
                int ct;
                for(ct=0;ct<numberOfThreads;ct++){
                    pthread_join(parameters[ct].threadid,NULL);
                }
                printf("MAIN THREAD: All done (successfully read %d words with %d threads from %d files).\n",bp->used,numberOfThreads,bp->filecount);
                exit(0);
            }
            strcpy((parameters[i].directoryName), directoryName);
            strcpy((parameters[i].fileName), ent->d_name);

            parameters[i].bp = bp;

            parameters[i].available=0;
            if(parameters[i].i==-1) pthread_create(&thread[i], NULL, &work, &parameters[i]);

            parameters[i].i = i;
            parameters[i].threadid = thread[i];


        }
        i++;
    }
}
void* work(void *parameters) {
    /*Bu üç satırda threadlerin inital fonksiyonlarında kullanılması gereken argümanları
     * pass ettik, bu parametreleri bir struct'ın içine koymuştuk daha öncesinde.
     * struct mainBuffer* bizim üzerine ekleme yaptığımız buffer'ı poin ediyor.
     * */
    while(1) {
        struct thread_parameters *tp = (struct thread_parameters *) parameters;
        char *fileName = tp->fileName;
        char *directoryName = tp->directoryName;
        if (strstr(fileName, ".txt") != NULL) {     //.txt olan bütün dosyaları sırayla çek
            printf("MAIN THREAD: Assigned \"%s\" to worker thread %u\n",tp->fileName,tp->threadid);
            sem_wait(&sem3);
                tp->bp->filecount+=1;
            sem_post(&sem3);
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

                sem_wait(&sem);
                tp->bp->used++;
                tp->offsetPtr = tp->bp->used;
                strcpy(tp->word,word);

                int s2val=0;
                sem_getvalue(&sem2,&s2val);
                while(s2val!=numberOfThreads){
                    sem_getvalue(&sem2,&s2val);
                }
                sem_init(&sem2,0,0);
                int ct;
                for(ct=0;ct<tp->bp->size;ct++){
                    if(strcmp(word,tp->bp->bufferPtr[ct].word)==0){
                        tp->offsetPtr=(size_t)ct;
                        break;
                    }
                }
                for(ct=0;ct<numberOfThreads;ct++){
                    if(strcmp(word,tp->others[ct].word)==0){
                        tp->offsetPtr=tp->others[ct].offsetPtr;
                        break;
                    }
                }
                sem_init(&sem2,0,numberOfThreads);
                if (tp->offsetPtr == tp->bp->size){
                    int sem2val=0;
                    sem_getvalue(&sem2,&sem2val);
                    if(sem2val>0){
                        while(sem2val!=numberOfThreads){
                            sem_getvalue(&sem2,&sem2val);
                        }
                    }
                    sem_init(&sem2,0,0);
                    reallocation(tp->bp);
                    printf("THREAD %u: Re-allocated array of %d pointers\n",tp->threadid,tp->bp->size);
                    tp->bp->bufferPtr[tp->offsetPtr].locationHead = NULL;
                    sem_init(&sem2,0,numberOfThreads);
                }
                sem_post(&sem);
                sem_wait(&sem2);
                putWord(word, tp);
                sem_post(&sem2);
            }
        }
        tp->available=1;
        while(tp->available==1){
            if(tp->bp->finished==1) {
                return NULL;
            }
        }
    }
}
void putWord(char *word,struct thread_parameters* parameters){
    bufferProperty* bp=parameters->bp;
    strcpy((bp->bufferPtr)[parameters->offsetPtr].word,word);
    if(bp->bufferPtr[parameters->offsetPtr].locationHead==NULL){
        bp->bufferPtr[parameters->offsetPtr].locationPtr=calloc(3,sizeof(struct location));
        bp->bufferPtr[parameters->offsetPtr].locationHead=bp->bufferPtr->locationPtr;
    }else{
        printf("HERE\n");
        bp->bufferPtr[parameters->offsetPtr].locationPtr->locationPtr=calloc(3,sizeof(struct location));
        bp->bufferPtr[parameters->offsetPtr].locationPtr=bp->bufferPtr->locationPtr->locationPtr;
    }
    strcpy(bp->bufferPtr[parameters->offsetPtr].locationPtr->fileName,parameters->fileName);
    strcpy(bp->bufferPtr[parameters->offsetPtr].locationPtr->directoryName,parameters->directoryName);
    bp->bufferPtr[parameters->offsetPtr].locationPtr->fileName;
    printf("THREAD %u: Added \"%s\" at index %d\n",parameters->threadid,(bp->bufferPtr)[parameters->offsetPtr].word,parameters->offsetPtr);
}
void reallocation(bufferProperty *bp){
    bp->size*=2;
    bp->bufferPtr = (struct mainBuffer*)realloc(bp->bufferPtr, bp->size * sizeof(struct mainBuffer));
}

