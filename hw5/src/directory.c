
#include "directory.h"
#include "csapp.h"

#include <string.h>
#include <sys/socket.h>
/*
 * The directory maintains a mapping from handles (i.e. user names)
 * to client info, which includes the client mailbox and the socket
 * file descriptor over which commands from the client are received.
 * The directory is used when sending a message, in order to find
 * the destination mailbox that corresponds to a particular recipient
 * specified by their handle.
 */

pthread_mutex_t lock;

typedef struct dirNode NODE;

MAILBOX* lookup_helper(char* helper);

struct dirNode{
    MAILBOX* mb;
    int fd;
    char* handle;
    NODE* next;
    NODE* prev;
};


typedef struct directory DIRECTORY;
struct directory{
    NODE* head;
    NODE* tail;
    int defunct;
};

DIRECTORY* dir;




/*
 * Initialize the directory.
 */
void dir_init(void){
    pthread_mutex_init(&lock, NULL);
    dir = Malloc(sizeof(DIRECTORY));
    dir->head = NULL;
    dir->tail = NULL;
    dir->defunct = 0;
}

/*
 * Shut down the directory.
 * This marks the directory as "defunct" and shuts down all the client sockets,
 * which triggers the eventual termination of all the server threads.
 */
void dir_shutdown(void){
    pthread_mutex_lock(&lock);
    NODE* cur = dir->head;
    while(cur != NULL){
        shutdown(cur->fd, SHUT_RDWR);
        cur = cur->next;
    }
    dir->defunct = 1;
    pthread_mutex_unlock(&lock);
}

/*
 * Finalize the directory.
 *
 * Precondition: the directory must previously have been shut down
 * by a call to dir_shutdown().
 */
void dir_fini(void){
    pthread_mutex_lock(&lock);
    NODE* cur = dir->head;
    while(cur != NULL){
        NODE* next = cur->next;
        free(cur->handle);
        mb_unref(cur->mb);
        mb_shutdown(cur->mb);
        free(cur);
        cur = next;
    }
    free(dir);
    pthread_mutex_unlock(&lock);
    pthread_mutex_destroy(&lock);
}

/*
 * Register a handle in the directory.
 *   handle - the handle to register
 *   sockfd - file descriptor of client socket
 *
 * Returns a new mailbox, if handle was not previously registered.
 * Returns NULL if handle was already registered or if the directory is defunct.
 */
MAILBOX *dir_register(char *handle, int sockfd){
    pthread_mutex_lock(&lock);
    if(dir->defunct){
        pthread_mutex_unlock(&lock);
        return NULL;
    }
    if(lookup_helper(handle) != NULL){
        pthread_mutex_unlock(&lock);
        return NULL;
    }
    NODE* node = Malloc(sizeof(NODE));
    node->mb = mb_init(handle);
    mb_ref(node->mb);
    node->fd = sockfd;
    node->handle = strdup(handle);
    if(dir->head == NULL){
        dir->head = node;
        dir->tail = node;
        node->next = NULL;
        node->prev = NULL;
    }
    else{
        dir->tail->next = node;
        node->prev = dir->tail;
        node->next = NULL;
        dir->tail = node;
    }
    pthread_mutex_unlock(&lock);
    return node->mb;
}

/*
 * Unregister a handle in the directory.
 * The associated mailbox is removed from the directory and shut down.
 */
void dir_unregister(char *handle){
    pthread_mutex_lock(&lock);
    NODE* cur = dir->head;
    while(cur != NULL){
        if(strcmp(handle, cur->handle) == 0){
            if(dir->tail == cur && dir->head == cur){
                dir->head = NULL;
            }
            else if(dir->tail == cur){
                cur->prev->next = NULL;
                dir->tail = cur->prev;
            }
            else if(dir->head == cur){
                cur->next->prev = NULL;
                dir->head = cur->next;
            }
            else{
                cur->next->prev = cur->prev;
                cur->prev->next = cur->next;
            }
            free(cur->handle);
            mb_unref(cur->mb);
            mb_shutdown(cur->mb);
            break;
        }
        cur = cur->next;
    }
    pthread_mutex_unlock(&lock);
}

/*
 * Query the directory for a specified handle.
 * If the handle is not registered, NULL is returned.
 * If the handle is registered, the corresponding mailbox is returned.
 * The reference count of the mailbox is increased to account for the
 * pointer that is being returned.  It is the caller's responsibility
 * to decrease the reference count when the pointer is ultimately discarded.
 */
MAILBOX *dir_lookup(char *handle){
    pthread_mutex_lock(&lock);
    MAILBOX* mb = lookup_helper(handle);
    pthread_mutex_unlock(&lock);
    return mb;

}

MAILBOX* lookup_helper(char* handle){
    NODE* cur = dir->head;
    while(cur != NULL){
        if(strcmp(handle, cur->handle) == 0){
            mb_ref(cur->mb);
            return cur->mb;
        }
        cur = cur->next;
    }
    return NULL;
}

/*
 * Obtain a list of all handles currently registered in the directory.
 * Returns a NULL-terminated array of strings.
 * It is the caller's responsibility to free the array and all the strings
 * that it contains.
 */
char **dir_all_handles(void){
    pthread_mutex_lock(&lock);
    NODE* cur = dir->head;
    char** handles = Malloc(sizeof(char*));
    int num = 0;
    if(cur == NULL)
        return handles;
    handles[num] = strdup(cur->handle);
    num++;
    cur = cur->next;
    while(cur != NULL){
        num++;
        handles = Realloc(handles, num * sizeof(char*));
        handles[num-1] = strdup(cur->handle);
        cur = cur->next;
    }
    num++;
    handles = Realloc(handles, num * sizeof(char*));
    handles[num - 1] = NULL;
    pthread_mutex_unlock(&lock);
    return handles;
}