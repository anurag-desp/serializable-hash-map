
/**
 * @author: Anurag
 * @date: Sep 13, 2024
 * @details: This was a problem asked in the coding assessment of a company I had applied to
 * @timelimit: 4 hrs 
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#define PUT_NEWLINE                     printf("\n")
#define MAX_COMMAND_LENGTH              10
#define METADATA_SIZE                   18
#define SPECIAL_KEY_A                   1
#define SPECIAL_KEY_B                   0

typedef enum {
    PAGE_SIZE_OFFSET            =       0,
    NUM_OF_PAGES_OFFSET         =       4,
    SPECIAL_KEY_A_EXISTS_OFFSET =       8,
    SPECIAL_KEY_A_VALUE_OFFSET  =       9,
    SPECIAL_KEY_B_EXISTS_OFFSET =       13,
    SPECIAL_KEY_B_VALUE_OFFSET  =       14,
    PAGES_OFFSET                =       18
} MAP_OFFSETS;

typedef enum {
    KEY_SIZE                    =       4,
    VALUE_SIZE                  =       4,
    KEY_VALUE_PAIR_SIZE         =       KEY_SIZE + VALUE_SIZE,
    SPECIAL_KEY_SET_FLAG        =       1,
    SPECIAL_KEY_RESET_FLAG      =       0
} KEY_VALUE_METADATA;


typedef struct {
    uint8_t *arr;
    int page_size;
    int number_of_pages;
} HashMap;


// utils
/**
 * Encoding
 * @param data: the integer value to be converted
 * @param byte_array: array of bytes(size: 4) from any base address 
 *                    of the array of the hashmap*/
void int_to_bytes(const int data, uint8_t *byte_array) {
    byte_array[0] = (data >> 24) & (0xFF);
    byte_array[1] = (data >> 16) & (0xFF);
    byte_array[2] = (data >> 8)  & (0xFF);
    byte_array[3] = (data >> 0)  & (0xFF); 
}

/**
 * Decoding
 * @param byte_array: the array of bytes
 * @return: the integer value of 4 bytes of the array
 */
int bytes_to_int(const uint8_t *byte_array) {
    int data = ((byte_array[0] << 24) |
                (byte_array[1] << 16) |
                (byte_array[2] << 8)  |
                (byte_array[3] << 0));
    
    return data;
}


void delete_hashmap (HashMap **hashmap) {
    if (*hashmap != NULL) {
        free((*hashmap)->arr);
        free(*hashmap);
    }
}

void init_hashmap(HashMap **hashmap, int page_size, int number_of_pages) {
    delete_hashmap(hashmap);

    (*hashmap) = (HashMap *)(malloc(sizeof(HashMap)));
    if ((*hashmap) == NULL) {
        perror("Couldn't create the (*hashmap)");
        exit(EXIT_FAILURE);
    }

    (*hashmap)->page_size = page_size;
    (*hashmap)->number_of_pages = number_of_pages; 
    int byte_array_total_size = METADATA_SIZE + (page_size * number_of_pages);
    (*hashmap)->arr = (uint8_t *)(calloc(byte_array_total_size, sizeof(uint8_t)));
    if ((*hashmap)->arr == NULL) {
        perror("Couldn't create the hashmap byte array");
        exit(EXIT_FAILURE);
    }
    
    int_to_bytes((*hashmap)->page_size, (*hashmap)->arr + PAGE_SIZE_OFFSET);
    int_to_bytes(number_of_pages, (*hashmap)->arr + NUM_OF_PAGES_OFFSET);
}

static inline _Bool is_special_key(const int key) {
    return (key == SPECIAL_KEY_A) || (key == SPECIAL_KEY_B);
}

int get(const HashMap *hashmap, const int key) {
    if (hashmap == NULL) {
        perror("Please `init` the hashmap first");
        return -1;
    }

    // for special keys
    if (is_special_key(key)) {
        int key_index = (key == SPECIAL_KEY_A)? SPECIAL_KEY_A_EXISTS_OFFSET: SPECIAL_KEY_B_EXISTS_OFFSET;
        if (hashmap->arr[key_index] == SPECIAL_KEY_SET_FLAG) {
            return bytes_to_int(hashmap->arr + key_index + 1);
        }
        return 0;
    }

    // for other keys
    int page_offset = abs(key % hashmap->number_of_pages);
    int page_start = METADATA_SIZE + (page_offset * hashmap->page_size);
    
    for (uint32_t i = page_start; i < (page_start + hashmap->page_size); i += KEY_VALUE_PAIR_SIZE) {
        int stored_key = bytes_to_int(hashmap->arr + i);
        if (stored_key == key) {
            return bytes_to_int(hashmap->arr + i + KEY_SIZE);
        }
        if (stored_key == 0) break;
    } 
    return 0;
}

void put(HashMap *hashmap, const int key, const int value) {
    if (hashmap == NULL) {
        perror("Please `init` the hashmap first");
        return;
    }

    // for special key
    if (is_special_key(key)) {
        int key_index = (key == SPECIAL_KEY_A)? SPECIAL_KEY_A_EXISTS_OFFSET: SPECIAL_KEY_B_EXISTS_OFFSET;
        hashmap->arr[key_index] = SPECIAL_KEY_SET_FLAG;
        int_to_bytes(value, hashmap->arr + key_index + 1);
        return;
    }

    // for other keys
    int page_offset = abs(key % hashmap->number_of_pages);
    int page_start = METADATA_SIZE + (page_offset * hashmap->page_size);

    for (uint32_t i = page_start; i < (page_start + hashmap->page_size); i += KEY_VALUE_PAIR_SIZE) {
        int stored_key = bytes_to_int(hashmap->arr + i);
        if ((stored_key == key) || (stored_key == 0) || (stored_key == -1)) {
            // set key
            int_to_bytes(key, hashmap->arr + i);
            // set value
            int_to_bytes(value, hashmap->arr + i + KEY_SIZE);
            return;
        }
    }
    perror("Page ran out of storage");
}

void delete(HashMap *hashmap, int key) {
    if (hashmap == NULL) {
        perror("Please `init` the hashmap first");
        return;
    }

    // for special keys
    if (is_special_key(key)) {
        int key_index = (key == SPECIAL_KEY_A)? SPECIAL_KEY_A_EXISTS_OFFSET: SPECIAL_KEY_B_EXISTS_OFFSET;
        hashmap->arr[key_index] = SPECIAL_KEY_RESET_FLAG;
        int_to_bytes(0, hashmap->arr + key_index + 1);
        return;
    }

    // for other keys
    int page_offset = abs(key % hashmap->number_of_pages);
    int page_start = METADATA_SIZE + (page_offset * hashmap->page_size);

    for (uint32_t i = page_start; i < (page_start + hashmap->page_size); i += KEY_VALUE_PAIR_SIZE) {
        int stored_key = bytes_to_int(hashmap->arr + i);
        if (stored_key == key) {
            // set key to -1
            int_to_bytes(-1, hashmap->arr + i);
            // set value to 0
            int_to_bytes( 0, hashmap->arr + i + KEY_SIZE);
            return;
        }
        if (stored_key == 0) return;
    }
}

void dump(const HashMap *hashmap) {
    if (hashmap == NULL) {
        perror("Hashmap is not initialized yet");
        return;
    }

    int byte_array_total_size = METADATA_SIZE + (hashmap->page_size * hashmap->number_of_pages);

    for (uint32_t i = 0; i < byte_array_total_size; i++) {
        printf("%02x", hashmap->arr[i]);
        if ((i == NUM_OF_PAGES_OFFSET - 1)          ||
            (i == SPECIAL_KEY_A_EXISTS_OFFSET - 1)  ||
            (i == SPECIAL_KEY_A_VALUE_OFFSET - 1)   ||
            (i == SPECIAL_KEY_B_EXISTS_OFFSET - 1)  ||
            (i == SPECIAL_KEY_B_VALUE_OFFSET - 1)  ||
            (i == PAGES_OFFSET - 1)) {
                printf(" ");
        }

        if (i == PAGES_OFFSET - 1) {
            for (int page = 0; page < hashmap->number_of_pages; page++) {
                printf("[");
                for (int j = 0; j < hashmap->page_size;  j++) {
                    printf("%02x", hashmap->arr[METADATA_SIZE + (page * hashmap->page_size) + j]);
                    if (((j + 1) % KEY_VALUE_PAIR_SIZE) == 0) {
                        printf(",");
                    }
                    if ((((j + 1) % KEY_SIZE) == 0) && (((j + 1) % KEY_VALUE_PAIR_SIZE) != 0)) {
                        printf(":");
                    }
                }
                printf("]");
            }
            break;
        }
    }
    printf("\n");
}




int main (int argc, char **argv) {
    
    char command[MAX_COMMAND_LENGTH];
    HashMap *hashmap = NULL;
    uint32_t page_size = 0, number_of_pages = 0;
    int key = 0, value = 0;

    while (scanf(" %s", command) != EOF) {
        if (strncmp(command, "init", MAX_COMMAND_LENGTH) == 0) {
            scanf("%u %u", &page_size, &number_of_pages);
            init_hashmap(&hashmap, page_size, number_of_pages);
        } else if (strncmp(command, "get", MAX_COMMAND_LENGTH) == 0) {
            scanf("%d", &key);
            printf("%d\n", get(hashmap, key));
        } else if (strncmp(command, "put", MAX_COMMAND_LENGTH) == 0) {
            scanf("%d %d", &key, &value);
            put(hashmap, key, value);
        } else if (strncmp(command, "delete", MAX_COMMAND_LENGTH) == 0) {
            scanf("%d", &key);
            delete(hashmap, key);
        } else if (strncmp(command, "dump", MAX_COMMAND_LENGTH) == 0) {
            dump(hashmap);
        } else {
            perror("Invalid input\n");
        }
    }

    delete_hashmap(&hashmap);
    return 0;
}