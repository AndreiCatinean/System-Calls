#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "a2_helper.h"
#include <time.h>
#include <stdlib.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <semaphore.h>
#include <fcntl.h>

#define SEM_NAMEin "/sempahore6in"
#define SEM_NAMEout "/sempahore6out"


typedef struct {
    int id;
    sem_t *logSem_6in, *logSem_6out;
    pthread_mutex_t *lock;
    pthread_cond_t *cond;
} TH_STRUCT6;

typedef struct {
    int id;
    sem_t *logSem;
    pthread_mutex_t *lock;
    pthread_mutex_t *lock2;
    pthread_cond_t *cond;
    sem_t *barrier;
} TH_STRUCT4;

typedef struct {
    int id;
    sem_t *logSem_6in, *logSem_6out;

} TH_STRUCT5;

int p6 = 0;
int p6end = 0;
int p4 = 0;
int t15 = 0;
int t15f = 0;
int p4processed = 0;


void *thread_function6(void* arg)
{
    TH_STRUCT6 *s = (TH_STRUCT6*)arg;


    if (s->id == 2 || s->id == 3)
    {
        pthread_mutex_lock(s->lock);

        if (s->id == 2) {
            while (p6 == 0) {
                pthread_cond_wait(s->cond, s->lock);
            }
            info(BEGIN, 6, s->id);
            info(END, 6, s->id);
            p6end = 1;
            pthread_cond_signal(s->cond);
        }

        else
        {
            info(BEGIN, 6, s->id);
            p6 = 1;
            pthread_cond_signal(s->cond);
            while (p6end == 0) {
                pthread_cond_wait(s->cond, s->lock);
            }
            info(END, 6, s->id);

        }

        pthread_mutex_unlock(s->lock);
    }
    else
    {

        if (s->id == 5)
        {

            sem_wait(s->logSem_6in);

        }

        info(BEGIN, 6, s->id);




        info(END, 6, s->id);
        if (s->id == 5)
        {

            sem_post(s->logSem_6out);
        }

    }


    return NULL;
}

void *thread_function5(void* arg)
{
    TH_STRUCT5 *s = (TH_STRUCT5*)arg;


    if (s->id == 5)
    {

        sem_wait(s->logSem_6out);

    }
    info(BEGIN, 5, s->id);



    info(END, 5, s->id);
    if (s->id == 2)
    {

        sem_post(s->logSem_6in);

    }


    return NULL;
}

void *thread_function4(void* arg)
{
    TH_STRUCT4 *s = (TH_STRUCT4*)arg;

    sem_wait(s->logSem);


    info(BEGIN, 4, s->id);

    pthread_mutex_lock(s->lock2);
    p4processed++;
    pthread_mutex_unlock(s->lock2);

    if ( s->id != 15 && ( t15 == 1 || (44 - p4processed < 4 && t15f == 0 )) )
    {

        pthread_mutex_lock(s->lock);
        p4++;

        if (p4 == 3) {

            sem_post(s->barrier);
        }
        pthread_cond_wait(s->cond, s->lock);
        p4--;
        pthread_mutex_unlock(s->lock);
    }


    if (s->id == 15)
    {
        t15 = 1;
        sem_wait(s->barrier);
    }


    info(END, 4, s->id);

    if (s->id == 15)
    {
        pthread_cond_broadcast(s->cond);
        t15 = 0;
        t15f = 1;

    }


    sem_post(s->logSem);

    return NULL;
}


int main() {
    init();

    pid_t pid2 = -1, pid3 = -1, pid4 = -1, pid5 = -1, pid6 = -1, pid7 = -1, pid8 = -1;
    srand(time(NULL));

    info(BEGIN, 1, 0);

    pid2 = fork();
    if (pid2 == 0)
    {
        info(BEGIN, 2, 0);

        pid3 = fork();

        if (pid3 == 0)
        {
            info(BEGIN, 3, 0);

            pid7 = fork();
            if (pid7 == 0)
            {
                info(BEGIN, 7, 0);

                pid8 = fork();
                if (pid8 == 0)
                {
                    info(BEGIN, 8, 0);
                    info(END, 8, 0);
                }
                else
                {
                    waitpid(pid8, NULL, 0);
                    info(END, 7, 0);
                }
            }
            else
            {
                waitpid(pid7, NULL, 0);
                info(END, 3, 0);
            }
        }
        else  {

            pid4 = fork();

            if (pid4 == 0)
            {
                info(BEGIN, 4, 0);

                pid5 = fork();
                if (pid5 == 0)
                {
                    info(BEGIN, 5, 0);

                    sem_t * logSem_6in  ;
                    sem_t * logSem_6out ;

                    logSem_6in = sem_open(SEM_NAMEin, O_CREAT, 0644, 0);
                    if (logSem_6in == SEM_FAILED) {
                        perror("sem_open failed");
                        exit(EXIT_FAILURE);
                    }

                    logSem_6out = sem_open(SEM_NAMEout, O_CREAT, 0644, 0);
                    if (logSem_6out == SEM_FAILED) {
                        perror("sem_open failed");
                        exit(EXIT_FAILURE);
                    }

                    int i;
                    TH_STRUCT5 params[6];
                    pthread_t tids[6];


                    for (i = 1; i <= 5; i++) {
                        params[i].id = i;

                        params[i].logSem_6in = logSem_6in;
                        params[i].logSem_6out = logSem_6out;
                        pthread_create(&tids[i], NULL, thread_function5, &params[i]);


                    }
                    for (i = 1; i <= 5; i++) {
                        pthread_join(tids[i], NULL);
                    }



                    info(END, 5, 0);
                    sem_close(logSem_6in);
                    sem_close(logSem_6out);
                    sem_unlink(SEM_NAMEin);
                    sem_unlink(SEM_NAMEout);
                }
                else
                {
                    pid6 = fork();
                    if (pid6 == 0)
                    {
                        info(BEGIN, 6, 0);

                        sem_t * logSem_6in  ;
                        sem_t * logSem_6out ;

                        logSem_6in = sem_open(SEM_NAMEin, O_CREAT, 0644, 0);
                        if (logSem_6in == SEM_FAILED) {
                            perror("sem_open failed");
                            exit(EXIT_FAILURE);
                        }

                        logSem_6out = sem_open(SEM_NAMEout, O_CREAT, 0644, 0);
                        if (logSem_6out == SEM_FAILED) {
                            perror("sem_open failed");
                            exit(EXIT_FAILURE);
                        }

                        int i;
                        TH_STRUCT6 params[6];
                        pthread_t tids[6];
                        pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
                        pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

                        for (i = 1; i <= 5; i++) {
                            params[i].id = i;
                            params[i].lock = &lock;
                            params[i].cond = &cond;

                            params[i].logSem_6in = logSem_6in;
                            params[i].logSem_6out = logSem_6out;
                            pthread_create(&tids[i], NULL, thread_function6, &params[i]);


                        }
                        for (i = 1; i <= 5; i++) {
                            pthread_join(tids[i], NULL);
                        }
                        pthread_cond_destroy(&cond);
                        pthread_mutex_destroy(&lock);



                        info(END, 6, 0);

                        sem_close(logSem_6in);
                        sem_close(logSem_6out);
                        sem_unlink(SEM_NAMEin);
                        sem_unlink(SEM_NAMEout);
                    }
                    else {

                        sem_t logSem, barrier;
                        int i;
                        TH_STRUCT4 params[44];
                        pthread_t tids[44];
                        pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
                        pthread_mutex_t lock2 = PTHREAD_MUTEX_INITIALIZER;
                        pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

                        //initialize random number generator
                        srand(time(NULL));
                        //initialize the semaphore
                        if (sem_init(&logSem, 0, 4) != 0) {
                            perror("Could not init the semaphore");
                            return -1;
                        }
                        if (sem_init(&barrier, 0, 0) != 0) {
                            perror("Could not init the semaphore");
                            return -1;
                        }

                        //create the threads
                        for (i = 1; i <= 44; i++) {
                            params[i].id = i;
                            params[i].logSem = &logSem;
                            params[i].lock = &lock;
                            params[i].lock2 = &lock2;
                            params[i].barrier = &barrier;
                            params[i].cond = &cond;
                            pthread_create(&tids[i], NULL, thread_function4, &params[i]);
                        }
                        //wait for the threads to finish
                        for (i = 1; i <= 44; i++) {
                            pthread_join(tids[i], NULL);
                        }
                        //destroy the semaphore
                        sem_destroy(&logSem);
                        sem_destroy(&barrier);
                        pthread_mutex_destroy(&lock);

                        waitpid(pid5, NULL, 0);
                        waitpid(pid6, NULL, 0);
                        info(END, 4, 0);


                    }
                }
            }
            else {

                waitpid(pid3, NULL, 0);
                waitpid(pid4, NULL, 0);
                info(END, 2, 0);
            }
        }

    }
    else
    {
        waitpid(pid2, NULL, 0);
        info(END, 1, 0);
    }


    return 0;
}