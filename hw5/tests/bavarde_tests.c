
#include "thread_counter.h"
#include "debug.h"
#include <pthread.h>

THREAD_COUNTER* tc;

void* inc();
void* dec();
void* wait_for_zero();

int main(int argc, char** argv){
    tc = tcnt_init();
    pthread_t tid1, tid2, tid3;
    pthread_create(&tid1, NULL, inc, NULL);
    pthread_create(&tid2, NULL, dec, NULL);
    pthread_create(&tid3, NULL, wait_for_zero, NULL);
    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);
    pthread_join(tid3, NULL);

}

void* inc(){
    for(int i = 0; i < 1000; i++){
        tcnt_incr(tc);
    }
    debug("thread 1 done");
    return NULL;
}

void* dec(){
    for(int i = 0; i < 1000; i++){
        tcnt_decr(tc);
    }
    debug("thread 2 done");
    return NULL;
}

void* wait_for_zero(){
    tcnt_wait_for_zero(tc);
    debug("thread 3 done");
    return NULL;
}