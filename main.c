#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <semaphore.h>

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
    int i,available,critic;
    pthread_t threadid;
    struct thread_parameters* others;
};

void get_arguments(int argc, char **argv);
int organize(bufferProperty*);
void* work(void*);
void putWord(char*,struct thread_parameters*);
bufferProperty* generateBuffer(bufferProperty*);
void reallocation(bufferProperty*);

sem_t sem,sem2,sem3,re,assign;

//struct mainBuffer* generateBuffer();

char* directoryName;
int numberOfThreads = 0;
int main(int argc, char **argv) {
    int offsetPtr=0;

    bufferProperty* bp=malloc(sizeof(bufferProperty));
    get_arguments(argc, argv);  //argumanları çekiyor, hata kontrolü yapabiliriz burada

    sem_init(&sem, 0, 1);
    sem_init(&sem2, 0, (unsigned)numberOfThreads);
    sem_init(&sem3, 0, 1);
    sem_init(&re, 0, 1);
    sem_init(&assign, 0, 1);

    if(organize(generateBuffer(bp))==-1) exit(0);
}

bufferProperty* generateBuffer(bufferProperty* bp){
    bp->bufferPtr = calloc(3,INITIALSIZE*sizeof(struct mainBuffer));
    bp->used=-1;
    bp->size=INITIALSIZE;
}
void get_arguments(int argc, char **argv) {

    if(argc < 5) {
        fprintf(stderr, "ERROR: Arguments not enough!\nUSAGE: ./a.out -d <directoryName> -n <#ofThreads>\n");
        exit(-1);
    }else if(argc > 5) {
        fprintf(stderr, "ERROR: Too many arguments!\nUSAGE: ./a.out -d <directoryName> -n <#ofThreads>\n");
        exit(-1);
    }

    int option;
    while ((option = getopt(argc, argv,"d:n:")) != -1) {
        switch (option) {
            case 'd' :
                directoryName = optarg;
                break;

            case 'n' :
                if(!isdigit(optarg[0])) {
                    fprintf(stderr, "ERROR: Threads number not an integer!\nUSAGE: ./a.out -d <directoryName> -n <#ofThreads>\n");
                    exit(-1);
                }
                numberOfThreads = atoi(optarg);
                break;
            default:
                exit(EXIT_FAILURE);
        }
    }
}
int organize(bufferProperty* bp){

    DIR *directory;
    DIR *directory2;
    directory = opendir(directoryName);
    directory2 = opendir(directoryName);

    if (directory == NULL){ //no directory
        fprintf(stderr,"ERROR: Could not open given directory.\n");
        free(bp);
        exit(-1);
    }

    struct dirent *countent;

    int filecount = 0;

    while((countent = readdir(directory2)) != NULL){
        if(strstr(countent->d_name,".txt") != NULL){
            filecount++;
        }
    }

    if(filecount < numberOfThreads) {
        fprintf(stderr,"ERROR: Number of threads more than files'!\n");
        exit(-1);
    }

    printf("MAIN THREAD: Allocated initial array size to 8 pointers\n");

    int i;
    struct dirent *ent;
    struct thread_parameters parameters[numberOfThreads];
    for(i=0;i<numberOfThreads;i++){
        parameters[i].available=1;
        parameters[i].i=-1;
        parameters[i].others=parameters;
        parameters[i].critic=0;
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
                printf("MAIN THREAD: All done (successfully read %ld words with %d threads from %d files).\n",bp->used,numberOfThreads,bp->filecount);
                free(bp);
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
     * struct mainBuffer* bizim üzerine ekleme yaptığımız buffer'ı point ediyor.
     * */
    while(1) {
        struct thread_parameters *tp = (struct thread_parameters *) parameters;
        char *fileName = tp->fileName;
        char *directoryName = tp->directoryName;
        if (strstr(fileName, ".txt") != NULL) {     //.txt olan bütün dosyaları sırayla çek
            printf("MAIN THREAD: Assigned \"%s\" to worker thread %ld\n",tp->fileName,tp->threadid);
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
                    printf("THREAD %ld: Re-allocated array of %ld pointers\n",tp->threadid,tp->bp->size);
                    tp->bp->bufferPtr[tp->offsetPtr].locationHead = NULL;
                    sem_init(&sem2,0,(unsigned)numberOfThreads);
                }
                sem_post(&sem);
                sem_wait(&sem2);

                int ct;

                for(ct=0;ct<tp->bp->size;ct++){

                    if(tp->bp->bufferPtr[ct].word[0] != '\0' && strcmp(word,tp->bp->bufferPtr[ct].word)==0){

                        tp->offsetPtr=(size_t)ct;
                        tp->critic=1;
                        break;
                    }

                }
                for(ct=0;ct<numberOfThreads;ct++){

                    if(strcmp(word,tp->others[ct].word)==0){

                        tp->offsetPtr=tp->others[ct].offsetPtr;
                        tp->critic=1;
                        tp->others[ct].critic=1;

                    }
                }
                if(tp->critic==1) sem_wait(&re);
                putWord(word, tp);
                if(tp->critic==1){
                    sem_post(&re);
                    tp->critic=0;
                }
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
        printf("THREAD %ld: Added \"%s\" at index %ld\n",parameters->threadid,(bp->bufferPtr)[parameters->offsetPtr].word,parameters->offsetPtr);
    }else{
        bp->bufferPtr[parameters->offsetPtr].locationPtr->locationPtr=calloc(3,sizeof(struct location));
        bp->bufferPtr[parameters->offsetPtr].locationPtr=bp->bufferPtr[parameters->offsetPtr].locationPtr->locationPtr;
        printf("THREAD %ld: The word \"%s\" is already located at index %ld\n",parameters->threadid,(bp->bufferPtr)[parameters->offsetPtr].word,parameters->offsetPtr);
    }
    strcpy(bp->bufferPtr[parameters->offsetPtr].locationPtr->fileName,parameters->fileName);
    strcpy(bp->bufferPtr[parameters->offsetPtr].locationPtr->directoryName,parameters->directoryName);

}
void reallocation(bufferProperty *bp){
    bp->size*=2;
    bp->bufferPtr = (struct mainBuffer*)realloc(bp->bufferPtr, bp->size * sizeof(struct mainBuffer));
}