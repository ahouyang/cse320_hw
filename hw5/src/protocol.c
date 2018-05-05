#include <protocol.h>
#include <unistd.h>
#include <errno.h>

#include "csapp.h"

/*
typedef struct {
    uint8_t type;          // Type of the packet
    uint32_t payload_length;       // Length of payload
    uint32_t msgid;                // Unique ID of message to which packet pertains
    uint32_t timestamp_sec;        // Seconds field of time packet was sent
    uint32_t timestamp_nsec;       // Nanoseconds field of time packet was sent
} bvd_packet_header;
*/
/*
 * Send a packet with a specified header and payload.
 *   fd - file descriptor on which packet is to be sent
 *   hdr - the packet header, with multi-byte fields in host byte order
 *   payload - pointer to packet payload, or NULL, if none.
 *
 * Multi-byte fields in the header are converted to network byte order
 * before sending.  The header structure passed to this function may
 * be modified as a result.
 *
 * On success, 0 is returned.
 * On error, -1 is returned and errno is set.
 */
int proto_send_packet(int fd, bvd_packet_header *hdr, void *payload){
    //first convert header to network byte order
    int payload_length = hdr->payload_length;
    hdr->payload_length = htonl(hdr->payload_length);
    hdr->msgid = htonl(hdr->msgid);
    hdr->timestamp_sec = htonl(hdr->timestamp_sec);
    hdr->timestamp_nsec = htonl(hdr->timestamp_nsec);
    //send header
    int result = rio_writen(fd, hdr, sizeof(bvd_packet_header));
    if(result == 0){
        errno = EINTR;
        return -1;
    }
    //check if there is payload
    if(payload_length == 0){
        return 0;
    }
    else{
        //if there is, write the payload
        int payload_result = rio_writen(fd, payload, payload_length);
        if(payload_result == 0){
            errno = EINTR;
            return -1;
        }
        return 0;
    }

}

/*
 * Receive a packet, blocking until one is available.
 *  fd - file descriptor from which packet is to be received
 *  hdr - pointer to caller-supplied storage for packet header
 *  payload - variable into which to store payload pointer
 *
 * The returned header has its multi-byte fields in host byte order.
 *
 * If the returned payload pointer is non-NULL, then the caller
 * is responsible for freeing the storage.
 *
 * On success, 0 is returned.
 * On error, -1 is returned, payload and length are left unchanged,
 * and errno is set.
 */

int proto_recv_packet(int fd, bvd_packet_header *hdr, void **payload){//allocate this pointer
    //read header
    int result = rio_readn(fd, hdr, sizeof(bvd_packet_header));

    if(result == -1){
        return -1;
    }
    hdr->payload_length = ntohl(hdr->payload_length);
    hdr->msgid = ntohl(hdr->msgid);
    hdr->timestamp_sec = ntohl(hdr->timestamp_sec);
    hdr->timestamp_nsec = ntohl(hdr->timestamp_nsec);

    if(hdr->payload_length == 0){
        return 0;
    }
    //printf("payload not zero\n");
    //allocate pointer
    *payload = Malloc(hdr->payload_length);
    int payload_read = rio_readn(fd, *payload, hdr->payload_length);
    //printf("read payload\n");
    if(payload_read == -1){
        return -1;
    }
    //printf("returned\n");
    return 0;
}