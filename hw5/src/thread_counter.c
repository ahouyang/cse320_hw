#include "thread_counter.h"
#include "csapp.h"
#include <semaphore.h>


struct thread_counter{
    int counter;
    sem_t sem;
    pthread_mutex_t mutex;
};



/*
 * Initialize a new thread counter.
 */
THREAD_COUNTER *tcnt_init(){
    THREAD_COUNTER* tc = Malloc(sizeof(THREAD_COUNTER));
    tc->counter = 0;
    Sem_init(&(tc->sem), 0, 0);
    pthread_mutex_init(&(tc->mutex), NULL);
    return tc;
}


/*
 * Finalize a thread counter.
 */
void tcnt_fini(THREAD_COUNTER *tc){
    sem_destroy(&(tc->sem));
    pthread_mutex_destroy(&(tc->mutex));
    free(tc);
}

/*
 * Increment a thread counter.
 */
void tcnt_incr(THREAD_COUNTER *tc){
    pthread_mutex_lock(&(tc->mutex));
    tc->counter++;
    pthread_mutex_unlock(&(tc->mutex));
}

/*
 * Decrement a thread counter, alerting anybody waiting
 * if the thread count has dropped to zero.
 */
void tcnt_decr(THREAD_COUNTER *tc){
    pthread_mutex_lock(&(tc->mutex));
    if(tc->counter == 0){
        pthread_mutex_unlock(&(tc->mutex));
        return;
    }
    tc->counter--;
    if(tc->counter == 0)
        V(&(tc->sem));
    pthread_mutex_unlock(&(tc->mutex));
}

/*
 * A thread calling this function will block in the call until
 * the thread count has reached zero, at which point the
 * function will return.
 */
void tcnt_wait_for_zero(THREAD_COUNTER *tc){
    if(tc->counter == 0)
        return;
    P(&(tc->sem));
}