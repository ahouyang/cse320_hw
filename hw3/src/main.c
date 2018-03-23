#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <budmm.h>

int main(int argc, char const *argv[]) {


    //int i;

    bud_mem_init();
/*
    void* a = bud_malloc(20);

    void* b = bud_malloc(20);
    void* c = bud_malloc(32);
    void* d = bud_malloc(64);
    void* e = bud_malloc(128);
    void* f = bud_malloc(256);
    void* g = bud_malloc(512);
    void* h = bud_malloc(1024);
    void* i = bud_malloc(2048);
    void* j = bud_malloc(4096);
    void* k = bud_malloc(8196);
    for(int i = 0; i < NUM_FREE_LIST; i++){
        //assert_empty_free_list(i + ORDER_MIN);
    }
    bud_free(a);
    //bud_free(b);
    bud_free(c);
    bud_free(d);
    bud_free(e);
    bud_free(f);
    bud_free(g);
    bud_free(h);
    bud_free(i);
    bud_free(j);
    bud_free(k);
    for(int i = 0; i < NUM_FREE_LIST; i++){
        //int count = count_free_list(i + ORDER_MIN);
        //cr_assert(count == 1);
    }


    //bud_header *bhdr_b = PAYLOAD_TO_HEADER(b);

    struct s1 {
        int a;
        float b;
        char *c;
    };
    struct s2 {
        int a[100];
        char *b;
    };

    uint32_t size = MIN_BLOCK_SIZE - sizeof(bud_header);
    char* carr = bud_malloc(size);
    //cr_assert_not_null(carr, "bud_malloc returned null on the first call");
    for (int i = 0; i < size; i++) {
        carr[i] = 'a';
    }
    bud_free(carr);
    uint32_t sizeof_s1 = sizeof(struct s1);

    struct s1 *s_1 = bud_malloc(sizeof_s1);*/

    char* ptr1 = (char*)bud_malloc(10); // 64

    ptr1[0] = 'a';
    ptr1[1] = 'b';
    ptr1[2] = 'c';
    ptr1[3] = 'd';
    ptr1[4] = 'e';
    ptr1[5] = 'f';
    ptr1[6] = '\0';
    printf("ptr1 string: %s\n", ptr1);

    int* ptr2 = bud_malloc(sizeof(int) * 100); // 512
    for(int i = 0; i < 100; i++)
        ptr2[i] = i;

    void* ptr3 = bud_malloc(3000); // 4192
    printf("ptr3: %p\n", ptr3);

    bud_free(ptr1);

    ptr2 = bud_realloc(ptr2, 124); // 128

    ptr1 = bud_malloc(200); // 256
    ptr1 = bud_realloc(ptr1, 100); // 128

    // intentional error (errno = EINVAL)
    ptr3 = bud_malloc(20000);
    printf("errno: %d (%s)\n", errno, strerror(errno));



    bud_mem_fini();

    return EXIT_SUCCESS;
}
