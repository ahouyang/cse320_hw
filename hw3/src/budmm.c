/*
 * All functions you make for the assignment must be implemented in this file.
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <debug.h>
#include <budmm.h>

/*
 * You should store the heads of your free lists in these variables.
 * Doing so will make it accessible via the extern statement in budmm.h
 * which will allow you to pass the address to sf_snapshot in a different file.
 */
extern bud_free_block free_list_heads[NUM_FREE_LIST];


int required_block_size(uint32_t rsize);
bud_free_block* split_blocks(bud_free_block* block, int required_order, int cur_index);
void init_allocated_block(bud_free_block* free_block, int index, int rsize, int required);
int get_order(int required);
void print_free_list();



void *bud_malloc(uint32_t rsize) {
    if(rsize == 0 || rsize > MAX_BLOCK_SIZE - sizeof(bud_header)){
        errno = EINVAL;
        return NULL;
    }
    //check for -1
    int required = required_block_size(rsize);

    //int order = (int)(log(required) / log(2));
    int order = get_order(required);

    int index = order - ORDER_MIN;
    //check if desired order has free blocks
    if(free_list_heads[index].next != &free_list_heads[index]){
        bud_free_block* block = free_list_heads[index].next;
        init_allocated_block(block, index, rsize, required);
        void* free_area = (void*)(block) + sizeof(bud_header);
        return free_area;
    }
    //check higher orders if required order has no free blocks
    int i;
    for(i = index + 1; i < NUM_FREE_LIST; i++){
        if(free_list_heads[i].next == &free_list_heads[i])
            continue;
        bud_free_block* block = free_list_heads[i].next;
        bud_free_block* final_block = split_blocks(block, order, i);
        init_allocated_block(final_block, i, rsize, required);
        void* free_area = (void*)(final_block) + sizeof(bud_header);
        return free_area;
        //return final_block;

    }
    //only reaches here if no free blocks are available
    bud_free_block* new_chunk = bud_sbrk();
    if(new_chunk == (void*)-1){
        errno = ENOMEM;
        return NULL;
    }

    new_chunk->header.allocated = 0;
    new_chunk->header.order = ORDER_MAX - 1;
    /*
    new_chunk->next = &free_list_heads[NUM_FREE_LIST - 1];
    new_chunk->prev = &free_list_heads[NUM_FREE_LIST - 1]; // last sentinel may not be empty!!!!!
    free_list_heads[NUM_FREE_LIST - 1].next = new_chunk;
    free_list_heads[NUM_FREE_LIST - 1].prev = new_chunk;
    */
    new_chunk->next = free_list_heads[NUM_FREE_LIST - 1].next;
    new_chunk->prev = &free_list_heads[NUM_FREE_LIST - 1];
    if(free_list_heads[NUM_FREE_LIST - 1].next == &free_list_heads[NUM_FREE_LIST - 1])
        free_list_heads[NUM_FREE_LIST - 1].prev = new_chunk;
    free_list_heads[NUM_FREE_LIST - 1].next = new_chunk;
    //bud_free_block* final_block = split_blocks(new_chunk, ORDER_MAX - 1, NUM_FREE_LIST - 1);
    bud_free_block* final_block = split_blocks(new_chunk, order, NUM_FREE_LIST - 1);
    init_allocated_block(final_block, NUM_FREE_LIST - 1, rsize, required);
    void* free_area = (void*)(final_block) + sizeof(bud_header);
    return free_area;


    //return NULL;
}

int get_order(int required){
    int order = 0;
    while(required != (1 << order))
        order++;
    return order;
}

void init_allocated_block(bud_free_block* free_block, int index, int rsize, int required){
    free_list_heads[index].next = free_list_heads[index].next->next;
    free_list_heads[index].next->prev = &free_list_heads[index];
    //bud_header header = free_block->header;
    free_block->header.allocated = 1;
    //header.order = order;
    free_block->header.rsize = rsize;
    free_block->header.padded = (free_block->header.rsize + sizeof(bud_header)) == required? 0 : 1;
    //return block + sizeof(bud_header);
}

bud_free_block* split_blocks(bud_free_block* block, int required_order, int cur_index){
    if(block->header.order == required_order){
        return block;
    }
    block->header.order = block->header.order - 1;
    void* unaligned = (void*)(block) + (1 << block->header.order);
    bud_free_block* split_block = unaligned;
    //bud_free_block* split_block = (bud_free_block*)(block + (1 << block->header.order));
    split_block->header.order = block->header.order;
    split_block->header.allocated = 0;
    split_block->prev = &free_list_heads[cur_index - 1];// ????????
    split_block->next = free_list_heads[cur_index - 1].next;
    if(free_list_heads[cur_index - 1].next == &free_list_heads[cur_index - 1]) //if list is empty, set the prev pointer also to new block
        free_list_heads[cur_index - 1].prev = split_block;
    free_list_heads[cur_index - 1].next = split_block;
    return split_blocks(block, required_order, cur_index - 1);
    //int split_block_address = block + (2^block->header.order);

}

int required_block_size(uint32_t rsize){
    uint32_t overhead = rsize + sizeof(bud_header); // changed from bud_free_block to bud_header
    for(int i = ORDER_MIN; i < ORDER_MAX; i++){
        int size = 1 << i;
        if(overhead <= size)
            return size;
    }
    return -1;
}


void *bud_realloc(void *ptr, uint32_t rsize) {
    if(ptr == NULL){
        return bud_malloc(rsize);
    }
    if(rsize == 0){
        bud_free(ptr);
        return NULL;
    }
    if(rsize > MAX_BLOCK_SIZE - sizeof(bud_header)){
        errno = EINVAL;
        return NULL;
    }
    bud_header* block_header = (void*)ptr - sizeof(bud_header);
    int requested_block_size = required_block_size(rsize);
    int current_size = required_block_size(block_header->rsize);
    if(bud_heap_start() > ptr || bud_heap_end() <= ptr)
        abort();
    //shifting
    if((uintptr_t)ptr % 8 != 0)
        abort();
    if(block_header->order < ORDER_MIN || block_header->order >= ORDER_MAX)
        abort();
    if(block_header->allocated == 0)
        abort();
    if(block_header->padded == 0 && block_header->rsize + sizeof(bud_header) != current_size)
        abort();
    if(block_header->padded == 1 && block_header->rsize + sizeof(bud_header) == current_size)
        abort();
    if(1<<block_header->order != current_size)
        abort();

    if(requested_block_size == 1<<block_header->order)
        return ptr;
    else if(requested_block_size > 1<<block_header->order){
        void* new_block = bud_malloc(rsize);
        memcpy(new_block, ptr, (1<<block_header->order) - sizeof(bud_header));
        bud_free(ptr);
        return new_block;
    }
    else{
        //block_size < 1<<block_header->order
        int order = get_order(requested_block_size);
        int index = block_header->order - ORDER_MIN;
        //int index = order - ORDER_MIN;
        //de-allocate current block
        block_header->allocated = 0;
        bud_free_block* current_block = (bud_free_block*)block_header;
        current_block->prev = &free_list_heads[index];
        current_block->next = free_list_heads[index].next;
        if(free_list_heads[index].next == &free_list_heads[index])
            free_list_heads[index].prev = current_block;
        free_list_heads[index].next = current_block;


        bud_free_block* new_block = split_blocks(current_block, order, index);
        init_allocated_block(new_block, index, rsize, requested_block_size);
        return ((void*)new_block + sizeof(bud_header));

    }

    return NULL;
}

void bud_free(void *ptr) {
    bud_header* block_header = (void*)ptr - sizeof(bud_header);
    int block_size = required_block_size(block_header->rsize);
    if(bud_heap_start() > ptr || bud_heap_end() <= ptr)
        abort();
    //shifting
    if((uintptr_t)ptr % 8 != 0)
        abort();
    if(block_header->order < ORDER_MIN || block_header->order >= ORDER_MAX)
        abort();
    if(block_header->allocated == 0)
        abort();
    if(block_header->padded == 0 && block_header->rsize + sizeof(bud_header) != block_size)
        abort();
    if(block_header->padded == 1 && block_header->rsize + sizeof(bud_header) == block_size)
        abort();
    if(1<<block_header->order != block_size)
        abort();

    bud_free_block* buddy_block = (void*)((uintptr_t)block_header^block_size);
    int index = block_header->order - ORDER_MIN;
    bud_header* current = block_header;
    int found = 0;
    while(!found){
        if(buddy_block->header.allocated == 1 || buddy_block->header.order != current->order
            || index == NUM_FREE_LIST - 1){
            //add to free list
            bud_free_block* current_block = (void*)current;
            current_block->next = free_list_heads[index].next;
            current_block->prev = &free_list_heads[index];
            if(free_list_heads[index].next == &free_list_heads[index])
                free_list_heads[index].prev = current_block;
            free_list_heads[index].next = current_block;
            current_block->header.allocated = 0;

            found = 1;
            continue;
        }
        bud_header* lower_address;
        if((uintptr_t)buddy_block > (uintptr_t)current){
            lower_address = current;
        }
        else{
            lower_address = (bud_header*)buddy_block;
        }
        lower_address->order = lower_address->order + 1;
        current = lower_address;
        //remove buddy_block from free list
        buddy_block->next->prev = buddy_block->prev;
        buddy_block->prev->next = buddy_block->next;


        //int current_size = required_block_size(current->order);
        int current_size = 1<<current->order;
        buddy_block = (void*)((uintptr_t)current^current_size);
        index++;
    }
    return;
}




void print_free_list(){
    for(int i = 0; i < NUM_FREE_LIST; i++){
        bud_free_block* current = free_list_heads[i].next;
        int list_count = 1;
        while(current != &free_list_heads[i]){
            printf("Index: %d\n", i);
            printf("Size:%d\n", 1<<current->header.order);
            printf("Index In List%d\n", list_count);
            printf("Order:%d\n", current->header.order);
            printf("Next:%p\n", current->next);
            printf("Prev:%p\n", current->prev);
            printf("Address:%p\n", current);
            printf("\n");
            list_count++;
            current = current->next;

        }
    }
}
