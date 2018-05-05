#include "mailbox.h"
#include "csapp.h"
#include "debug.h"



//node for linked list
typedef struct entry_node ENTRY_NODE;

struct entry_node{
    ENTRY_NODE* next;
    //ENTRY_NODE* prev;
    MAILBOX_ENTRY* entry;
};



struct mailbox{
    int defunct;
    char* handle;
    int ref_count;
    ENTRY_NODE* head;
    ENTRY_NODE* tail;
    MAILBOX_DISCARD_HOOK* hook;
    sem_t semaphore;
    pthread_mutex_t mutex;
};



void insert_node(MAILBOX* mb, ENTRY_NODE* node);

void discard_hook(MAILBOX_ENTRY* entry){
    if(entry->type == MESSAGE_ENTRY_TYPE){
        MAILBOX* sender = entry->content.message.from;
        if(sender == NULL)
            return;
        //create
        mb_add_notice(sender, BOUNCE_NOTICE_TYPE, entry->content.message.msgid, NULL, 0);
    }
}



/*
 * Create a new mailbox for a given handle.
 * The mailbox is returned with a reference count of 1.
 */
MAILBOX *mb_init(char *handle){
    MAILBOX* mb = Malloc(sizeof(MAILBOX));
    mb->handle = strdup(handle);
    mb->head = NULL;
    mb->tail = NULL;
    mb->ref_count = 0;
    mb_set_discard_hook(mb, discard_hook);
    Sem_init(&(mb->semaphore), 0, 0);
    pthread_mutex_init(&(mb->mutex), NULL);
    return mb;
}

/*
 * Set the discard hook for a mailbox.
 */
void mb_set_discard_hook(MAILBOX *mb, MAILBOX_DISCARD_HOOK * hook){
    mb->hook = hook;
}

/*
 * Increase the reference count on a mailbox.
 * This must be called whenever a pointer to a mailbox is copied,
 * so that the reference count always matches the number of pointers
 * that exist to the mailbox.
 */
void mb_ref(MAILBOX *mb){
    pthread_mutex_lock(&(mb->mutex));
    debug("ref count increased");
    mb->ref_count = mb->ref_count + 1;
    pthread_mutex_unlock(&(mb->mutex));
}

/*
 * Decrease the reference count on a mailbox.
 * This must be called whenever a pointer to a mailbox is discarded,
 * so that the reference count always matches the number of pointers
 * that exist to the mailbox.  When the reference count reaches zero,
 * the mailbox will be finalized.
 */
void mb_unref(MAILBOX *mb){
    pthread_mutex_lock(&(mb->mutex));

    mb->ref_count = mb->ref_count - 1;
    debug("ref count decreased");
    if(mb->ref_count == 0){
        //finalize mailbox
        ENTRY_NODE* cur = mb->head;
        while(cur != NULL){
            ENTRY_NODE* next = cur->next;
            free(cur->entry->body);
            if(cur->entry->type == MESSAGE_ENTRY_TYPE){
                mb_unref(cur->entry->content.message.from);
            }
            free(&(cur->entry->content));
            free(cur->entry);
            free(cur);
            cur = next;
        }
        free(mb->handle);
        sem_destroy(&(mb->semaphore));
        pthread_mutex_unlock(&(mb->mutex));
        pthread_mutex_destroy(&(mb->mutex));
        free(mb);
    }
}

/*
 * Shut down this mailbox.
 * The mailbox is set to the "defunct" state.  A defunct mailbox should
 * not be used to send any more messages or notices, but it continues
 * to exist until the last outstanding reference to it has been
 * discarded.  At that point, the mailbox will be finalized, and any
 * entries that remain in it will be discarded.
 */
void mb_shutdown(MAILBOX *mb){
    pthread_mutex_lock(&(mb->mutex));
    mb->defunct = 1;
    V(&(mb->semaphore));
    pthread_mutex_unlock(&(mb->mutex));
}

/*
 * Get the handle associated with a mailbox.
 */
char *mb_get_handle(MAILBOX *mb){
    pthread_mutex_lock(&(mb->mutex));
    char* handle = mb->handle;
    pthread_mutex_unlock(&(mb->mutex));
    return handle;
}

/*
 * Add a message to the end of the mailbox queue.
 *   msgid - the message ID
 *   from - the sender's mailbox
 *   body - the body of the message, which can be arbitrary data, or NULL
 *   length - number of bytes of data in the body
 *
 * The message body must have been allocated on the heap,
 * but the caller is relieved of the responsibility of ultimately
 * freeing this storage, as it will become the responsibility of
 * whomever removes this message from the mailbox.
 *
 * The reference to the sender's mailbox ("from") is conceptually
 * "transferred" from the caller to the new message, so no increase in
 * the reference count is performed.  However, after the call the
 * caller must discard this pointer which it no longer "owns".
 */
void mb_add_message(MAILBOX *mb, int msgid, MAILBOX *from, void *body, int length){
    pthread_mutex_lock(&(mb->mutex));
    MESSAGE* message = Malloc(sizeof(MESSAGE));
    message->from = from;
    message->msgid = msgid;
    MAILBOX_ENTRY* me = Malloc(sizeof(MAILBOX_ENTRY));
    me->type = MESSAGE_ENTRY_TYPE;
    me->body = body;
    me->length = length;
    me->content.message = *message;
    ENTRY_NODE* node = Malloc(sizeof(ENTRY_NODE));
    node->entry = me;
    node->next = NULL;
    insert_node(mb, node);
    V(&(mb->semaphore));
    debug("message added");
    pthread_mutex_unlock(&(mb->mutex));
}


void insert_node(MAILBOX* mb, ENTRY_NODE* node){
    if(mb->head == NULL){
        mb->head = node;
        mb->tail = node;
    }
    else{
        mb->tail->next = node;
        mb->tail = node;
    }
}

/*
 * Add a notice to the end of the mailbox queue.
 *   ntype - the notice type
 *   msgid - the ID of the message to which the notice pertains
 *   body - the body of the notice, which can be arbitrary data, or NULL
 *   length - number of bytes of data in the body
 *
 * The notice body must have been allocated on the heap, but the
 * caller is relieved of the responsibility of ultimately freeing this
 * storage, as it will become the responsibility of whomever removes
 * this notice from the mailbox.
 */
void mb_add_notice(MAILBOX *mb, NOTICE_TYPE ntype, int msgid, void *body, int length){
    pthread_mutex_lock(&(mb->mutex));
    NOTICE* notice = Malloc(sizeof(NOTICE));
    notice->type = ntype;
    notice->msgid = msgid;
    MAILBOX_ENTRY*me = Malloc(sizeof(MAILBOX_ENTRY));
    me->type =NOTICE_ENTRY_TYPE;
    me->body = body;
    me->length = length;
    me->content.notice = *notice;
    ENTRY_NODE* node = Malloc(sizeof(ENTRY_NODE));
    node->next = NULL;
    node->entry = me;
    insert_node(mb, node);
    V(&(mb->semaphore));
    debug("notice added");
    pthread_mutex_unlock(&(mb->mutex));

}

/*
 * Remove the first entry from the mailbox, blocking until there is
 * one.  The caller assumes the responsibility of freeing the entry
 * and its body, if present.  In addition, if it is a message entry,
 * the caller must decrease the reference count on the "from" mailbox
 * to account for the destruction of the pointer.
 *
 * This function will return NULL in case the mailbox is defunct.
 * The thread servicing the mailbox should use this as an indication
 * that service should be terminated.
 */
MAILBOX_ENTRY *mb_next_entry(MAILBOX *mb){
    pthread_mutex_lock(&(mb->mutex));
    if(mb->defunct){
        pthread_mutex_unlock(&(mb->mutex));
        return NULL;
    }
    /*
    if(mb->head == NULL){
        //block
        P(&(mb->semaphore));

        pthread_mutex_unlock(&(mb->mutex));
    }
    */
    P(&(mb->semaphore));
    ENTRY_NODE* node = mb->head;
    mb->head = mb->head->next;
    pthread_mutex_unlock(&(mb->mutex));
    return node->entry;
}