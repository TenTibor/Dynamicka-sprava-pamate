#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>


void *memory;

struct BLOCK_HEAD {
    int size;
    struct BLOCK_HEAD *next;
};


struct MAIN_HEAD {
    struct BLOCK_HEAD *free_block;
};

void memory_init(void *ptr, unsigned int size) {
    memset(ptr, 0, size);

    memory = ptr;
    struct MAIN_HEAD *memory_head = (struct MAIN_HEAD *) ptr;
//    memory_head->size = size;

    struct BLOCK_HEAD *firstFreeBlock = (struct BLOCK_HEAD *) ((char *) ptr + sizeof(struct MAIN_HEAD));
    memory_head->free_block = firstFreeBlock;

    int freeSize = ((int) size) - sizeof(struct MAIN_HEAD);
    firstFreeBlock->size = freeSize * -1;
    firstFreeBlock->next = NULL;
}

// Find best free block using best fit
void *find_free_block(int size) {

    // Start searching from the beginning
    struct MAIN_HEAD *memory_head = (struct MAIN_HEAD *) memory;

    struct BLOCK_HEAD *bestBlock = memory_head->free_block;
    struct BLOCK_HEAD *actualBlock = memory_head->free_block;
    unsigned int bestBlockSize = 0;

    while (actualBlock != NULL) {
        //Is this smallest good block?
        if (actualBlock->size < 0) { // If block is free
            int thisSize = actualBlock->size * -1;
//            printf("Block %d have size %d\n", actualBlock, thisSize);
            if (thisSize >= size && (thisSize < bestBlockSize || bestBlockSize == 0)) {
//                printf(" %d < %d for %d\n", thisSize, bestBlockSize, size);
                // Yes, it is best block for now
                bestBlock = actualBlock;
                bestBlockSize = thisSize;
//                printf("Best block is %d with size %d\n", bestBlock, bestBlockSize);
            }
        }
        // Go to next block
        actualBlock = actualBlock->next;
    }

    // If smallest block size is big enough for size we need
    if (bestBlockSize >= size) {
        return bestBlock;
    } else {
        return NULL;
    }

}

void *memory_alloc(unsigned int size) {
    unsigned int allocateSize = size;
    // Create new block
    struct MAIN_HEAD *memory_head = (struct MAIN_HEAD *) memory;

    // Return null if free block does not exist
    if (memory_head->free_block == NULL) {
        return NULL;
    }
    // Size of new block
    size += sizeof(struct BLOCK_HEAD);
//    size &= ~(1); // 1 means that this block will not be free


    // Find best free block with best fit
    struct BLOCK_HEAD *free_block = (struct BLOCK_HEAD *) find_free_block(size);

    // If free block wasn't found, end this function
    if (free_block == NULL)
        return NULL;

    // Create new block in free block we found before
    struct BLOCK_HEAD *new_free_block;
    // If free space is bigger then we need, we separate it and create new smaller free block
    if (size != free_block->size * -1 && free_block->size + size > sizeof(int) + sizeof(struct BLOCK_HEAD)) {
        void *pointer = free_block; // Create pointer to free block
        pointer += size; // Move pointer to end of the block
        new_free_block = ((struct BLOCK_HEAD *) pointer); // Create new free block on end of the block we want to separate

        // Size of block we separate minus size of block we need to have
        new_free_block->size = (free_block->size + size);

        //// Edit connection between blocks

        // If best free block we found is first block
        if (memory_head->free_block == free_block) {
            memory_head->free_block = new_free_block;
        } else {
            // Else search that block to end while we dont find that free block
            struct BLOCK_HEAD *actual_block = (struct BLOCK_HEAD *) memory_head->free_block;
            while (actual_block != free_block && actual_block->next != NULL) {
                if (actual_block != NULL && actual_block->next == free_block) {
                    // If  we found that new free block, we set pointer of actual block to that new and we change linked list to what we need
                    actual_block->next = new_free_block;
                }
                actual_block = actual_block->next;
            }

        }
    } else {
        // Else if free block have same size like we need

        // If best free block we found is first block
        if (memory_head->free_block == free_block) {
            memory_head->free_block = free_block->next;
        } else {
            struct BLOCK_HEAD *actual_block = (struct BLOCK_HEAD *) memory_head->free_block;
            while (actual_block != free_block && actual_block->next != NULL) {
                if (actual_block != NULL && actual_block->next == free_block) {
                    actual_block->next = free_block->next;
                }
                actual_block = actual_block->next;
            }
        }
    }

    // create new allocated block
    struct BLOCK_HEAD *allocated_block = free_block;
    allocated_block->size = allocateSize;
    allocated_block->next = NULL;
//    int *footer = (int *) ((char *) allocated_block + size);
//    *footer = size | 1;

//    printf("allocoval som velkost %d na %d adrese\n", size, allocated_block);

    // return pointer
    return (char *) allocated_block + sizeof(*allocated_block);


//    printf("this is first pointer %d\n", *ptr);
//
//    ptr->next = (ptr + size + sizeof(HEAD));
//    printf("Next adresa [%d]\n");
//
//    return ptr + sizeof(HEAD);
    //ked alokujes, tak vracias adresu alokacie.. Vracias adresu na zaciatok alokacie, nie hlavicky
}

int memory_free(void *valid_ptr) {

    // Find block to free
    struct BLOCK_HEAD *block = (struct BLOCK_HEAD *) ((char *) valid_ptr - sizeof(struct BLOCK_HEAD));
    if (block->size < 0) {
        printf("Block is already free");
        return 1;
    }
    block->size = block->size * -1;

//        int *end_header = (char *) block + block->size - sizeof(unsigned int);
//    *end_header = block->size;

    // Find block after this block
    struct BLOCK_HEAD *block_after_head = (struct BLOCK_HEAD *) ((int) block + block->size * -1 +
                                                                 sizeof(struct BLOCK_HEAD));
    // If block is free connect it to this block
    if (block_after_head->size < 0) {
        block->next = block_after_head->next;
        block->size += block_after_head->size - sizeof(struct BLOCK_HEAD);

        if (((struct MAIN_HEAD *) memory)->free_block == block_after_head) {
            ((struct MAIN_HEAD *) memory)->free_block = block;
        }
    }


    // Find block before this block
    struct BLOCK_HEAD *block_before_head = NULL;
    struct MAIN_HEAD *memory_head = (struct MAIN_HEAD *) memory;
    struct BLOCK_HEAD *check_block = memory_head->free_block;
//    printf("First free block is %d and his next is %d\n", check_block, check_block->next);


    while (check_block != NULL && check_block->next != NULL && block_before_head == NULL) {
        // Predict if next block is block we free now
        int predict = ((int) check_block + check_block->size * -1 + sizeof(struct BLOCK_HEAD));
        if (predict == block) {
            block_before_head = check_block;
        }
        check_block = check_block->next;
    }
//    printf("I found block %d\n", block_before_head);

    // If we found block before
    if (block_before_head != NULL && block_before_head->size < 0) {
//        printf("This block is free with size %d\n", block_before_head->size);
        block_before_head->size += block->size - sizeof(struct BLOCK_HEAD);
        block_before_head->next = block->next;
        block = block_before_head;

//        end_header = (char *) block_before_head + block_before_head->size - sizeof(unsigned int);
//        *end_header = block->size;
    }

    // Edit list of free blocks
    if (((struct MAIN_HEAD *) memory)->free_block != block) {
        block->next = ((struct MAIN_HEAD *) memory)->free_block;
        ((struct MAIN_HEAD *) memory)->free_block = block;
    }

    return 0;
}

// If pointer is valid return 1
int memory_check(void *ptr) {
    struct MAIN_HEAD *memory_head = (struct MAIN_HEAD *) memory;
    struct BLOCK_HEAD *actual_block = memory_head->free_block;

    // Check every free block and return 0 if we found that pointer
    while (actual_block != NULL) {
        if ((int) actual_block + sizeof(struct BLOCK_HEAD) == ptr)
            return 0;
        actual_block = actual_block->next;
    }
    return 1;
}

void test1() {
    int memory_size = 600;
    char region[memory_size];
    memory_init(&region, memory_size);
    printf("header size is %d\n", sizeof(struct BLOCK_HEAD));
    printf("starter poinet is %d\n", memory);

    char *pointer1 = (char *) memory_alloc(15);
    char *pointer2 = (char *) memory_alloc(40);
    printf("Pointer 2 is %d\n", pointer2);
    char *pointer3 = (char *) memory_alloc(20);
    char *pointer4 = (char *) memory_alloc(15);
    char *pointer5 = (char *) memory_alloc(15);
    char *pointer6 = (char *) memory_alloc(15);
    char *pointer7 = (char *) memory_alloc(15);
    memory_free(pointer3);
    memory_free(pointer2);
    memory_free(pointer4);
    memory_free(pointer5);
    char *pointer8 = (char *) memory_alloc(50);
}

void z1_testovac(char *region, char **pointer, int minBlock, int maxBlock, int minMemory, int maxMemory,
                 int testFragDefrag) {
    srand(time(0));
    unsigned int allocated = 0;
    unsigned int mallocated = 0;
    unsigned int allocated_count = 0;
    unsigned int mallocated_count = 0;
    unsigned int i = 0;
    int random_memory = 0;
    int random = 0;
    memset(region, 0, 100000);
    random_memory = (rand() % (maxMemory - minMemory + 1)) + minMemory;
    memory_init(region + 500, random_memory);
    if (testFragDefrag) {
        do {
            pointer[i] = memory_alloc(8);
            if (pointer[i])
                i++;
        } while (pointer[i]);
        for (int j = 0; j < i; j++) {
            if (memory_check(pointer[j])) {
                memory_free(pointer[j]);
            } else {
                printf("Error: Wrong memory check.\n");
            }
        }
    }
    i = 0;
    while (allocated <= random_memory - minBlock) {
        random = (rand() % (maxBlock - minBlock + 1)) + minBlock;
        if (allocated + random > random_memory)
            continue;
        allocated += random;
        allocated_count++;
        pointer[i] = memory_alloc(random);
        if (pointer[i]) {
            i++;
            mallocated_count++;
            mallocated += random;
        }
    }
    for (int j = 0; j < i; j++) {
        if (memory_check(pointer[j])) {
            memory_free(pointer[j]);
        } else {
            printf("Error: Wrong memory check.\n");
        }
    }
    memset(region + 500, 0, random_memory);
    for (int j = 0; j < 100000; j++) {
        if (region[j] != 0) {
            region[j] = 0;
            printf("Error: Modified memory outside the managed region. index: %d\n", j - 500);
        }
    }
    float result = ((float) mallocated_count / allocated_count) * 100;
    float result_bytes = ((float) mallocated / allocated) * 100;
    printf("Memory size of %d bytes: allocated %.2f%% blocks (%.2f%% bytes).\n", random_memory, result, result_bytes);
}

void test4() {
    char region[100000];
    char *pointer[13000];
    z1_testovac(region, pointer, 8, 24, 50, 100, 1);
    z1_testovac(region, pointer, 8, 1000, 10000, 20000, 0);
    z1_testovac(region, pointer, 8, 35000, 50000, 99000, 0);
}

int main() {
    test4();
    return 0;
}
