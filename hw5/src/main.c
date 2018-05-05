#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "debug.h"
#include "server.h"
#include "directory.h"
#include "thread_counter.h"
#include "csapp.h"

#include <getopt.h>
#include <csapp.h>
#include <signal.h>
#include <pthread.h>
static void terminate();

THREAD_COUNTER *thread_counter;

void SIGHUP_handler(int signum){
    terminate();
}

int main(int argc, char* argv[]) {
    // Option processing should be performed here.
    // Option '-p <port>' is required in order to specify the port number
    // on which the server should listen.
    int option;
    int socket;
    while((option = getopt(argc, argv, "p:")) != -1){
        switch(option){
            case 'p':;
                char* port_num = optarg;
                socket = Open_listenfd(port_num);
        }
    }


    // Perform required initializations of the thread counter and directory.
    thread_counter = tcnt_init();
    dir_init();

    // TODO: Set up the server socket and enter a loop to accept connections
    // on this socket.  For each connection, a thread should be started to
    // run function bvd_client_service().  In addition, you should install
    // a SIGHUP handler, so that receipt of SIGHUP will perform a clean
    // shutdown of the server.
    /*
    struct sigaction sa;
    sa.sa_handler = SIGHUP_handler;

    sigaction(SIGHUP, &sa, NULL);*/
    Signal(SIGHUP, SIGHUP_handler);

    while(1){
        struct sockaddr client_addr;
        socklen_t addr_len;
        int* conn_fd = malloc(sizeof(int));
        *conn_fd = Accept(socket, &client_addr, &addr_len);
        //Accept(socket, &client_addr, &addr_len);
        pthread_t tid;
        pthread_create(&tid,NULL,bvd_client_service,conn_fd);

    }



    fprintf(stderr, "You have to finish implementing main() "
	    "before the Bavarde server will function.\n");

    terminate();
}

/*
 * Function called to cleanly shut down the server.
 */
void terminate(int sig) {
    // Shut down the directory.
    // This will trigger the eventual termination of service threads.
    dir_shutdown();

    debug("Waiting for service threads to terminate...");
    tcnt_wait_for_zero(thread_counter);
    debug("All service threads terminated.");

    tcnt_fini(thread_counter);
    dir_fini();
    exit(EXIT_SUCCESS);
}
