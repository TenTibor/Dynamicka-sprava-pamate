#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

void *memoryStart;

struct BLOCK_HEAD {
    int size;
    struct BLOCK_HEAD *next;
};

void memory_init(void *ptr, unsigned int size) {
    memset(ptr, 0, size);

    // Find first free block
    memoryStart = ptr;
    struct BLOCK_HEAD *memory_head = (struct BLOCK_HEAD *) ptr;
    struct BLOCK_HEAD *firstBlock = (struct BLOCK_HEAD *) ((char *) ptr + sizeof(struct BLOCK_HEAD));
    memory_head->next = firstBlock;

    // Set first block
    int freeSize = ((int) size) - sizeof(struct BLOCK_HEAD);
    firstBlock->size = freeSize * -1; // Set block as free
    firstBlock->next = NULL;
}

// Find best free block using best fit
void *find_free_block(int size) {

    // Start searching from the beginning of free block list
    struct BLOCK_HEAD *memoryHead = (struct BLOCK_HEAD *) memoryStart;

    struct BLOCK_HEAD *bestBlock = memoryHead->next;
    struct BLOCK_HEAD *actualBlock = memoryHead->next;
    int bestBlockSize = 0;

    while (actualBlock != NULL) {
        //Is this smallest good block?
        if (actualBlock->size < 0) { // If block is free
            int thisSize = actualBlock->size * -1;
            if (thisSize >= size && (thisSize < bestBlockSize || bestBlockSize == 0)) {
                // Yes, it is best block for now
                bestBlock = actualBlock;
                bestBlockSize = thisSize;
            }
        }
        // Go to next block
        actualBlock = actualBlock->next;
    }

    // Last check if smallest block size is big enough for size we need
    if (bestBlockSize >= size)
        return bestBlock;
    else
        return NULL;
}

void *memory_alloc(unsigned int size) {
    unsigned int allocateSize = size;

    // Return null if free block does not exist
    struct BLOCK_HEAD *memoryHead = (struct BLOCK_HEAD *) memoryStart;
    if (memoryHead->next == NULL) {
        return NULL;
    }

    // Size of new block
    size += sizeof(struct BLOCK_HEAD);

    // Find free block
    struct BLOCK_HEAD *foundFreeBlock = (struct BLOCK_HEAD *) find_free_block((int) size);

    // If free block wasn't found, end this function
    if (foundFreeBlock == NULL)
        return NULL;

    // Create new block in free block we found before
    struct BLOCK_HEAD *newFreeBlock;
    // If free space is bigger then we need, we separate it and create new smaller free block
    if (size != foundFreeBlock->size * -1 && (foundFreeBlock->size * -1) - size > sizeof(struct BLOCK_HEAD)) {
        void *pointer = foundFreeBlock; // Create pointer to free block
        pointer += size; // Move pointer to end of the block
        newFreeBlock = ((struct BLOCK_HEAD *) pointer); // Create new free block on end of the block we want to separate

        // Size of block we separate minus size of block we need to have
        newFreeBlock->size = (foundFreeBlock->size + (int) size);

        // If best free block we found is first block
        if (memoryHead->next == foundFreeBlock) {
            memoryHead->next = newFreeBlock;
        } else {
            // Else search that block
            struct BLOCK_HEAD *actualBlock = (struct BLOCK_HEAD *) memoryHead->next;
            while (actualBlock != foundFreeBlock && actualBlock->next != NULL) {
                if (actualBlock != NULL && actualBlock->next == foundFreeBlock) {
                    // When we find prev block of found block, we connect new free block to list of free blocks
                    actualBlock->next = newFreeBlock;
                }
                actualBlock = actualBlock->next;
            }
        }
    } else {
        // Else if free block have same size like we need

        // If best free block we found is first block
        if (memoryHead->next == foundFreeBlock) {
            memoryHead->next = foundFreeBlock->next;
        } else {
            struct BLOCK_HEAD *actual_block = (struct BLOCK_HEAD *) memoryHead->next;
            while (actual_block != foundFreeBlock && actual_block->next != NULL) {
                if (actual_block != NULL && actual_block->next == foundFreeBlock) {
                    actual_block->next = foundFreeBlock->next;
                }
                actual_block = actual_block->next;
            }
        }
    }

    // create new allocated block
    struct BLOCK_HEAD *allocatedBlock = foundFreeBlock;
    allocatedBlock->size = (int) allocateSize;
    allocatedBlock->next = NULL;

    // return pointer
    return (char *) allocatedBlock + sizeof(*allocatedBlock);
}

int memory_free(void *valid_ptr) {
    // Find block to free with pointer
    struct BLOCK_HEAD *blockToFree = (struct BLOCK_HEAD *) ((char *) valid_ptr - sizeof(struct BLOCK_HEAD));

    // Check if block is not already free
    if (blockToFree->size < 0) {
        return 1;
    }

    blockToFree->size *= -1; // Set size to negative number what means, block is now free

    // Check if next block is free. If block is free, connect it to this block
    struct BLOCK_HEAD *blockAfter = (struct BLOCK_HEAD *) ((char *) blockToFree + blockToFree->size * -1 +
                                                           sizeof(struct BLOCK_HEAD));

    if (blockAfter->size < 0) { // if block is free
        blockToFree->next = blockAfter->next;
        blockToFree->size += blockAfter->size - sizeof(struct BLOCK_HEAD);

        // If blockAfter first.. Move blockToFree to first place
        if (((struct BLOCK_HEAD *) memoryStart)->next == blockAfter) {
            ((struct BLOCK_HEAD *) memoryStart)->next = blockToFree;
        }
    }


    // Check if prev block is free. If block is free, connect it to this block
    struct BLOCK_HEAD *blockBefore = NULL;
    struct BLOCK_HEAD *memoryHead = (struct BLOCK_HEAD *) memoryStart;
    struct BLOCK_HEAD *actualBlock = memoryHead->next;

    while (actualBlock != NULL && actualBlock->next != NULL && blockBefore == NULL) {
        // Predict if next blocks pointer is pointer we free now
        char *predict = ((char *) actualBlock + actualBlock->size * -1 + sizeof(struct BLOCK_HEAD));
        if (predict == blockToFree) {
            blockBefore = actualBlock;
        }
        actualBlock = actualBlock->next;
    }

    // If we found block before
    if (blockBefore != NULL && blockBefore->size < 0) {
        blockBefore->size += blockToFree->size - sizeof(struct BLOCK_HEAD);
        blockBefore->next = blockToFree->next;
        blockToFree = blockBefore;
    }

    // Edit list of free blocks
    if (((struct BLOCK_HEAD *) memoryStart)->next != blockToFree) {
        blockToFree->next = ((struct BLOCK_HEAD *) memoryStart)->next;
        ((struct BLOCK_HEAD *) memoryStart)->next = blockToFree;
    }

    return 0;
}

// If pointer is valid return 1
int memory_check(void *ptr) {
    if (ptr == 0) return 0;

    struct BLOCK_HEAD *memory_head = (struct BLOCK_HEAD *) memoryStart;
    struct BLOCK_HEAD *actual_block = memory_head->next;

    // Check every free block and return 0 if we found that pointer
    while (actual_block != NULL) {
        if ((char *) actual_block + sizeof(struct BLOCK_HEAD) == ptr)
            return 0;
        actual_block = actual_block->next;
    }
    return 1;
}

//void test1() {
//    int memory_size = 600;
//    char region[memory_size];
//    memory_init(&region, memory_size);
//    printf("header size is %d\n", sizeof(struct BLOCK_HEAD));
//    printf("starter poinet is %d\n", memoryStart);
//
//    char *pointer1 = (char *) memory_alloc(15);
//    char *pointer2 = (char *) memory_alloc(40);
////    printf("Pointer 2 is %d\n", pointer2);
//    char *pointer3 = (char *) memory_alloc(20);
//    char *pointer4 = (char *) memory_alloc(15);
//    char *pointer5 = (char *) memory_alloc(15);
//    char *pointer6 = (char *) memory_alloc(15);
//    char *pointer7 = (char *) memory_alloc(15);
//    memory_free(pointer3);
//    memory_free(pointer2);
//    memory_free(pointer4);
//    memory_free(pointer5);
//    char *pointer8 = (char *) memory_alloc(50);
//}
//
//void test2() {
//    int memory_size = 600;
//    char region[memory_size];
//    memory_init(&region, memory_size);
//    char *pointer1 = (char *) memory_alloc(15);
//    char *pointer2 = (char *) memory_alloc(40);
//    char *pointer3 = (char *) memory_alloc(20);
//    char *pointer4 = (char *) memory_alloc(15);
//    if (memory_check(pointer3)) {
//        printf("TRUE\n");
//    } else printf("False\n");
//
//    memory_free(pointer3);
//    if (memory_check(pointer3)) {
//        printf("TRUE\n");
//    } else printf("False\n");
//}
//
//void test3() {
//    int memory_size = 600;
//    char region[memory_size];
//    memory_init(&region, memory_size);
//
//    char *pointer1 = (char *) memory_alloc(15);
//    printf("%d\n", pointer1);
//    memory_free(pointer1);
//}
//

void printBlockUsage(int memory_size, float mallocated_count, float allocated_count) {
    float blockUsage = ((float) mallocated_count / (float) allocated_count) * 100;
    printf("Size of memory: %d bytes\nAllocated blocks: %.2f%%\n", memory_size, blockUsage);
}

// Pridavanie rovnkakych blokov malej velkosti
void test1() {
    int memorySize = 200;
    char region[memorySize];
    char *pointers[1000];
    float allocatedPart = 0;
    float mallocatedPart = 0;
    memory_init(&region, memorySize);

    for (int i = 0; i < 6; ++i) {
        allocatedPart++;
        pointers[i] = memory_alloc(16);
        if (pointers[i]) {
            mallocatedPart++;
        }
    }

    printBlockUsage(memorySize, mallocatedPart, allocatedPart);
}

void test2() {
    srand(time(0));
    int memorySize = 200;
    char region[memorySize];
    char *pointers[1000];
    float allocatedPart = 0;
    float mallocatedPart = 0;
    memory_init(&region, memorySize);

    for (int i = 0; i < 12; ++i) {
        allocatedPart++;
        int randomSize = (rand() % (24 - 8 + 1)) + 8;;
        pointers[i] = memory_alloc(randomSize);
        if (pointers[i]) {
            mallocatedPart++;
        }
    }

    printBlockUsage(memorySize, mallocatedPart, allocatedPart);
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
//
//void test4() {
//    char region[100000];
//    char *pointer[13000];
//    z1_testovac(region, pointer, 8, 24, 50, 100, 1);
//    z1_testovac(region, pointer, 8, 1000, 10000, 20000, 0);
//    z1_testovac(region, pointer, 8, 35000, 50000, 99000, 0);
//}

// Testovace staci skusat na 1000bitov

int main() {
    test2();
    return 0;
}
