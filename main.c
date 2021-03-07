#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h> // for random numbers

void *memoryStart;

struct HEAD {
    int size;
    struct HEAD *next;
};

void memory_init(void *ptr, unsigned int size) {
    memset(ptr, 0, size);

    // Create start of memory on pointer
    memoryStart = ptr;
    struct HEAD *memoryHead = (struct HEAD *) ptr;
    struct HEAD *firstBlock = (struct HEAD *) ((char *) ptr + sizeof(struct HEAD));
    memoryHead->next = firstBlock;

    // Set first block
    firstBlock->size = (((int) size) - sizeof(struct HEAD)) * -1; // Set block as free
    firstBlock->next = NULL;
}

// Find best free block using best fit
void *findBlock(int size) {
    // Start searching from the beginning of free block list
    struct HEAD *memoryHead = (struct HEAD *) memoryStart;
    struct HEAD *bestBlock = memoryHead->next;
    struct HEAD *actualBlock = memoryHead->next;
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
    struct HEAD *memoryHead = (struct HEAD *) memoryStart;
    if (memoryHead->next == NULL) {
        return NULL;
    }

    // Size of new block
    size += sizeof(struct HEAD);

    // Find free block
    struct HEAD *foundFreeBlock = (struct HEAD *) findBlock((int) size);

    // If free block wasn't found, end this function
    if (foundFreeBlock == NULL)
        return NULL;

    // Create new block in free block we found before
    struct HEAD *newFreeBlock;
    // If free space is bigger then we need, we separate it and create new smaller free block
    if (foundFreeBlock->size * -1 != size && (foundFreeBlock->size * -1) - size > sizeof(struct HEAD)) {
        void *pointer = foundFreeBlock; // Create pointer to free block
        pointer += size; // Move pointer to end of the block
        newFreeBlock = ((struct HEAD *) pointer); // Create new free block on end of the block we want to separate

        // Size of block we separate minus size of block we need to have
        newFreeBlock->size = (foundFreeBlock->size + (int) size);

        // If best free block we found is first block
        if (memoryHead->next == foundFreeBlock) {
            memoryHead->next = newFreeBlock;
        } else {
            // Else search that block
            struct HEAD *actualBlock = (struct HEAD *) memoryHead->next;
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
            // if not, we must find next free block to take foundFreeBlock out and dont break list
            struct HEAD *actualBlock = (struct HEAD *) memoryHead->next;
            while (actualBlock != foundFreeBlock && actualBlock->next != NULL) {
                if (actualBlock != NULL && actualBlock->next == foundFreeBlock) {
                    actualBlock->next = foundFreeBlock->next;
                }
                actualBlock = actualBlock->next;
            }
        }
    }

    // create new allocated block
    struct HEAD *allocatedBlock = foundFreeBlock;
    allocatedBlock->size = (int) allocateSize;
    allocatedBlock->next = NULL;

    // return pointer
    return (char *) allocatedBlock + sizeof(*allocatedBlock);
}

int memory_free(void *valid_ptr) {
    // Find block to free with pointer
    struct HEAD *blockToFree = (struct HEAD *) ((char *) valid_ptr - sizeof(struct HEAD));

    // Check if block is not already free
    if (blockToFree->size < 0) {
        return 1;
    }

    blockToFree->size *= -1; // Set size to negative number what means, block is now free

    // Check if next block is free. If block is free, merge it to this block
    struct HEAD *blockAfter = (struct HEAD *) ((char *) blockToFree + blockToFree->size * -1 + sizeof(struct HEAD));

    if (blockAfter->size < 0) { // if block is free
        blockToFree->next = blockAfter->next;
        blockToFree->size += blockAfter->size - sizeof(struct HEAD);

        // If blockAfter is first in memory.. We move blockToFree to beginning of memory
        if (((struct HEAD *) memoryStart)->next == blockAfter) {
            ((struct HEAD *) memoryStart)->next = blockToFree;
        }
    }


    // Check if prev block is free. If block is free, connect it to this block
    struct HEAD *blockBefore = NULL;
    struct HEAD *memoryHead = (struct HEAD *) memoryStart;
    struct HEAD *actualBlock = memoryHead->next;

    while (actualBlock != NULL && actualBlock->next != NULL && blockBefore == NULL) {
        // Predict if next blocks pointer is pointer we free now
        char *predict = ((char *) actualBlock + actualBlock->size * -1 + sizeof(struct HEAD));
        if (predict == blockToFree) {
            blockBefore = actualBlock;
        }
        actualBlock = actualBlock->next;
    }

    // If we found block before
    if (blockBefore != NULL && blockBefore->size < 0) {
        blockBefore->size += blockToFree->size - sizeof(struct HEAD);
        blockBefore->next = blockToFree->next;
        blockToFree = blockBefore;
    }

    // Edit list of free blocks
    if (((struct HEAD *) memoryStart)->next != blockToFree) {
        blockToFree->next = ((struct HEAD *) memoryStart)->next;
        ((struct HEAD *) memoryStart)->next = blockToFree;
    }

    return 0;
}

// If pointer is valid return 1
int memory_check(void *ptr) {
    if (ptr == 0) return 0;

    struct HEAD *memoryHead = (struct HEAD *) memoryStart;
    struct HEAD *actualBlock = memoryHead->next;

    // Check every free block and return 0 if we found that pointer
    while (actualBlock <= ptr) {
        int firstBit = (char *) actualBlock;
        int lastBit = ((char *) actualBlock + sizeof(struct HEAD) + (actualBlock->size * -1));
        // If pointer is in free block
        if (actualBlock->size < 0 && ptr >= firstBit && ptr < lastBit)
            return 0;

        int positiveSize = actualBlock->size;
        if (positiveSize < 0) positiveSize *= -1;

        actualBlock = (struct HEAD *) ((char *) actualBlock + sizeof(struct HEAD) + positiveSize);
    }
    return 1;
}

//
// TESTING
//

void startTestVisually(int name) {
    printf("=============TEST%d=============\n", name);
}

void endTestVisually() {
    printf("\n");
}

void printBlockUsage(int memory_size, float mallocated_count, float allocated_count) {
    float blockUsage = ((float) mallocated_count / (float) allocated_count) * 100;
    printf("Allocated blocks: %.2f%%\n", blockUsage);
    endTestVisually();
}

void isPointerValid(void *ptr) {
    if (memory_check(ptr))
        printf("Pointer is valid\n");
    else
        printf("Pointer is not valid\n");
}


// Pridavanie rovnkakych blokov malej velkosti
void test1() {
    startTestVisually(1);
    printf("> Adding small blocks with same size\n");
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

// Pridavanie nahodne velkych blokov malej velkosti
void test2() {
    startTestVisually(2);
    printf("> Adding small blocks with random size\n");
    srand(time(0));
    int memorySize = 300;
    char region[memorySize];
    char *pointers[1000];
    float allocatedPart = 0;
    float mallocatedPart = 0;
    memory_init(&region, memorySize);

    for (int i = 0; i < 11; ++i) {
        allocatedPart++;
        int randomSize = (rand() % (24 - 8 + 1)) + 8;;
        pointers[i] = memory_alloc(randomSize);
        if (pointers[i]) {
            mallocatedPart++;
        }
    }

    printBlockUsage(memorySize, mallocatedPart, allocatedPart);
}

// Pridavanie nahodne velkych blokov vacsej velkosti do vacsej pamate
void test3() {
    startTestVisually(3);
    printf("> Adding blocks with random size\n");
    srand(time(0));
    int memorySize = 30000;
    char region[memorySize];
    char *pointers[1000];
    float allocatedPart = 0;
    float mallocatedPart = 0;
    float allocatedMemory = 0;
    memory_init(&region, memorySize);
    printf("Memory size: %d\n", memorySize);
    printf("Size of block is between 100 to 1000\n");
    int i = 0;
    while (allocatedMemory <= memorySize - 100) {
        allocatedPart++;
        int randomSize = (rand() % (1000 - 100 + 1)) + 100;;
        if (allocatedMemory + randomSize > memorySize)
            continue;
        allocatedMemory += randomSize;
        pointers[i] = memory_alloc(randomSize);
        if (pointers[i]) {
            mallocatedPart++;
            printf("Block with size %d was allocated on address %d\n", randomSize, pointers[i]);
        }
        i++;
    }

    printBlockUsage(memorySize, mallocatedPart, allocatedPart);
}

// Pridavanie a uvolnovanie blokov
void test4() {
    startTestVisually(4);
    printf("> Adding & freeing blocks\n");
    int memorySize = 400;
    char region[memorySize];
    memory_init(&region, memorySize);

    char *pointer1 = (char *) memory_alloc(15);
    char *pointer2 = (char *) memory_alloc(40);
    char *pointer3 = (char *) memory_alloc(20);
    printf("Pointer3 have address %d\n", pointer3);
    char *pointer4 = (char *) memory_alloc(20);
    printf("Pointer4 have address %d\n", pointer4);
    char *pointer5 = (char *) memory_alloc(15);
    printf("Pointer5 have address %d\n", pointer5);

    if (memory_check(pointer3)) {
        memory_free(pointer3);
        if (!memory_check(pointer3)) printf("Block 3 is now free\n");
    }
    if (memory_check(pointer4)) {
        memory_free(pointer4);
        if (memory_check(pointer4)) printf("Block 4 doesn't exist\n");
    }
    char *pointer6 = (char *) memory_alloc(25);
    printf("Pointer6 have address %d\n", pointer6);
    endTestVisually();
}

void test5() {
    startTestVisually(5);
    printf("> Testing memory checks\n");
    int memorySize = 400;
    char region[memorySize];
    memory_init(&region, memorySize);

    char *pointer = (char *) memory_alloc(25);
    isPointerValid(pointer);
    isPointerValid(pointer + 5);

    memory_free(pointer);
    isPointerValid(pointer);
    isPointerValid(pointer + 5);
    endTestVisually();
}

int main() {
    test1();
    test2();
    test3();
    test4();
    test5();
    return 0;
}
