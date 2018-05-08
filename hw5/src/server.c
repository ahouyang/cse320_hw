#include "server.h"
#include "protocol.h"
#include "csapp.h"
#include "directory.h"
#include "thread_counter.h"

void parse_header(int fd, bvd_packet_header* hdr, void** payload);
void remove_newline(char* str);
void send_response(int fd, bvd_packet_header* hdr, bvd_packet_type type, int payload_length, void* payload);
void send_mailbox_notice(int fd, MAILBOX* mb);


typedef struct fd_and_mb FD_MB;
typedef struct node CLIENT_NODE;
struct node{
    FD_MB* info;
    CLIENT_NODE* next;
    CLIENT_NODE* prev;
};

CLIENT_NODE* head = NULL;
CLIENT_NODE* tail = NULL;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
THREAD_COUNTER *thread_counter;
//pthread_mutex_init(&mutex, NULL);

void insert_client_node(CLIENT_NODE* node){
    pthread_mutex_lock(&mutex);
    if(head == NULL){
        head = node;
        tail = node;
        pthread_mutex_unlock(&mutex);
        return;
    }
    tail->next = node;
    node->prev = tail;
    tail = node;
    pthread_mutex_unlock(&mutex);
}

CLIENT_NODE* remove_node(int fd){
    pthread_mutex_lock(&mutex);
    CLIENT_NODE* cur = head;
    while(cur!= NULL){
        if(cur->info->fd == fd){
            if(tail == cur && head == cur){
                head = NULL;
            }
            else if(tail == cur){
                cur->prev->next = NULL;
                tail = cur->prev;
            }
            else if(head == cur){
                cur->next->prev = NULL;
                head = cur->next;
            }
            else{
                cur->next->prev = cur->prev;
                cur->prev->next = cur->next;
            }
            pthread_mutex_unlock(&mutex);
            return cur;
        }
        cur = cur->next;
    }
    pthread_mutex_unlock(&mutex);
    return NULL;
}

MAILBOX* list_lookup(int fd){
    pthread_mutex_lock(&mutex);
    CLIENT_NODE* cur = head;
    while(cur!= NULL){
        if((cur->info->fd) == fd){
            pthread_mutex_unlock(&mutex);
            return cur->info->mb;
        }
        cur = cur->next;
    }
    pthread_mutex_unlock(&mutex);
    return NULL;

}
/*
 * Thread counter that should be used to track the number of
 * service threads that have been created.
 */
//extern THREAD_COUNTER *thread_counter;

/*
 * Thread function for the thread that handles client requests.
 *
 * The arg pointer point to the file descriptor of client connection.
 * This pointer must be freed after the file descriptor has been
 * retrieved.
 */
void *bvd_client_service(void *arg){
    int* fd_pointer = (int*)arg;
    int fd = *fd_pointer;
    free(arg);
    while(1){
        bvd_packet_header* hdr = Malloc(sizeof(bvd_packet_header));
        void** payload = Malloc(sizeof(void*));
        int response = proto_recv_packet(fd, hdr, payload);
        if(response == -1)
            break;
        parse_header(fd, hdr, payload);
        //free(*payload);
        free(payload);
        free(hdr);
    }
    return NULL;
}


void parse_header(int fd, bvd_packet_header* hdr, void** payload){
    pthread_t tid;
    bvd_packet_type type = hdr->type;
    if(type == BVD_LOGIN_PKT){
        char** char_payload = (char**)(payload);
        char* header = *char_payload;
        //remove_newline(header);
        MAILBOX* mb = dir_register(header, fd);
        if(mb == NULL){
            send_response(fd, hdr, BVD_NACK_PKT, 0, NULL);
        }
        else{
            //add node to list
            CLIENT_NODE* node = Malloc(sizeof(CLIENT_NODE));
            node->next = NULL;
            node->prev = NULL;
            FD_MB* fd_mb = Malloc(sizeof(FD_MB));
            fd_mb->fd = fd;
            fd_mb->mb = mb;
            node->info = fd_mb;
            FD_MB* copy = Malloc(sizeof(FD_MB));
            memcpy(copy, fd_mb, sizeof(FD_MB));
            insert_client_node(node);
            mb_add_notice(mb, ACK_NOTICE_TYPE, hdr->msgid, NULL, 0);
            pthread_create(&tid, NULL, bvd_mailbox_service, copy);
            //send_response(fd, hdr, BVD_ACK_PKT, 0, NULL);
        }
        //return 1;
    }
    else if(type == BVD_LOGOUT_PKT){
        CLIENT_NODE* removed = remove_node(fd);
        if(removed == NULL){
            shutdown(fd, SHUT_RDWR);
            close(fd);
        }
        else{
            char* handle = mb_get_handle(removed->info->mb);
            dir_unregister(handle);
            free(removed->info);
            free(removed);
            shutdown(fd, SHUT_RDWR);
            close(fd);
        }
    }
    else if(type == BVD_USERS_PKT){
        char** handles = dir_all_handles();
        int total_length = 0;
        char** cur = handles;
        int counter = 0;
        while(*cur != NULL){
            total_length = total_length + strlen(*cur);
            cur++;
            counter++;
        }
        char* users = Malloc(total_length);
        for(int i = 0; i < counter; i++){
            strcat(users, handles[i]);
        }
        MAILBOX* lookup = list_lookup(fd);
        if(lookup == NULL){
            send_response(fd, hdr, BVD_ACK_PKT, counter, users);
        }
        else{
            mb_add_notice(lookup, ACK_NOTICE_TYPE, hdr->msgid, users, total_length);
        }
    }
    else if(type == BVD_SEND_PKT){
        char* new_line_pointer = strchr(*payload, '\n');
        int new_line_index = new_line_pointer - (char*)*payload;
        char* copy = Malloc(new_line_index + 2); //need one more extra spot for the null terminator, and another for \n as it's zero based
        //strncpy(copy, *payload, new_line_index + 1);
        memcpy(copy, *payload, new_line_index + 1);
        copy[new_line_index + 1] = '\0';
        char* end_of_payload = ((char*)*payload) + hdr->payload_length;
        *end_of_payload = '\0';
        MAILBOX* receiver = dir_lookup(copy);
        //free(copy);
        MAILBOX* sender = list_lookup(fd);
        char* handle = mb_get_handle(sender);
        int payload_length = strlen(new_line_pointer + 1);
        int total_length = payload_length + strlen(handle);
        char* full_message = Malloc(total_length);
        strcat(full_message, handle);
        strcat(full_message, new_line_pointer + 1);

        if(sender == NULL || receiver == NULL){
            //free(full_message);
            send_response(fd, hdr, BVD_NACK_PKT, 0, NULL);
        }
        else{
            mb_add_message(receiver, hdr->msgid, sender, full_message,total_length);
            //free(full_message);
            send_response(fd, hdr, BVD_ACK_PKT, 0, NULL);
        }
    }
    /*
    switch(hdr->type){
        case BVD_LOGIN_PKT:;
            //remove \r\n from end of handle
            char** char_payload = (char**)(payload);
            char* header = *char_payload;
            remove_newline(header);
            MAILBOX* mb = dir_register(header, fd);
            if(mb == NULL){
                send_response(fd, hdr, BVD_NACK_PKT, 0);
            }
            else{
                send_response(fd, hdr, BVD_ACK_PKT, 0);
            }
            break;

        case BVD_LOGOUT_PKT:;
            char** char_payload = (char**)(payload);
            char* header = *char_payload;
            remove_newline(header);
        case BVD_USERS_PKT:;
        case BVD_SEND_PKT:;

    }*/
}

void remove_newline(char* str){
    int i = 0;
    while(str[i] != '\r')
        i++;
    str[i] = '\0';
}

void send_response(int fd, bvd_packet_header* hdr, bvd_packet_type type, int payload_length, void* payload){
    bvd_packet_header* response = Malloc(sizeof(bvd_packet_header));
    response->type = type;
    response->msgid = hdr->msgid;
    response->payload_length = payload_length;
    proto_send_packet(fd, response, payload);

}




/*
 * Thread function for the thread that handles client requests.
 *
 * The arg pointer points to the file descriptor of client connection.
 * This pointer must be freed after the file descriptor has been
 * retrieved.
 */
/*
struct fd_and_mb {
    int fd;
    MAILBOX *mb;
};*/
/*
 * Once the file descriptor and mailbox have been retrieved,
 * this structure must be freed.
 */
void *bvd_mailbox_service(void *arg){
    tcnt_incr(thread_counter);
    FD_MB* info = (FD_MB*)arg;
    int fd = info->fd;
    MAILBOX* mb = info->mb;
    free(arg);
    while(1){
        MAILBOX_ENTRY* entry = mb_next_entry(mb);
        if(entry == NULL){
            tcnt_decr(thread_counter);
            pthread_exit(NULL);
        }
        else{
            if(entry->type == MESSAGE_ENTRY_TYPE){
                //send DLVR to recipient
                //send receipt to sender
                MESSAGE message = entry->content.message;
                mb_add_notice(message.from, RRCPT_NOTICE_TYPE,
                    message.msgid, NULL, 0);
               // mb_add_notice(message.from, ACK_NOTICE_TYPE,
                 //   message.msgid, NULL, 0);
                bvd_packet_header* hdr = Malloc(sizeof(bvd_packet_header));
                hdr->type = BVD_DLVR_PKT;
                hdr->payload_length = entry->length;
                hdr->msgid = message.msgid;
                proto_send_packet(fd, hdr, entry->body);
                free(hdr);
                //free entry
                //free(&(entry->content.message));
                free(entry);
            }
            else if(entry->type == NOTICE_ENTRY_TYPE){
                NOTICE notice = entry->content.notice;
                bvd_packet_header* hdr = Malloc(sizeof(bvd_packet_header));
                //hdr->type = notice.type;
                switch(notice.type){
                    case ACK_NOTICE_TYPE:;
                        hdr->type = BVD_ACK_PKT;
                        break;
                    case NACK_NOTICE_TYPE:;
                        hdr->type = BVD_NACK_PKT;
                        break;
                    case BOUNCE_NOTICE_TYPE:;
                        hdr->type = BVD_BOUNCE_PKT;
                        break;
                    case RRCPT_NOTICE_TYPE:;
                        hdr->type =BVD_RRCPT_PKT;
                        break;
                }

                hdr->payload_length = entry->length;
                hdr->msgid = notice.msgid;
                proto_send_packet(fd, hdr, entry->body);
                free(hdr);
                //free(&(entry->content.notice));
                free(entry);
            }


        }

    }
}
