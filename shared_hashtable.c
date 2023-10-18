/* 
 *  File:        hashtable.c
 *  Author:      Haoyu Shi
 *  Student ID:  100143687
 *  Version:     2.0
 *  Date:        2021.2.21
 *  Purpose:     Implementation of hashtable using semaphore lock and
 *               shared memory. 
 * 
 *  Note:        The program will allocate a piece of memory with exzact 
 *               needed size and hash pair<keys(name), value> into the 
 *               hashtable.
 */

#ifndef _SHARED_HASH_TABLE_H_
#define _SHARED_HASH_TABLE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <pthread.h>
#include "utility_macros.h"
#include "shared_hashtable.h"

/* Structure to represnt the header of hash table. */
typedef struct hash_table_struct {
    sem_t lock;                    /* Semaphore lock. */
    size_t max_element_size;        /* max_element_size. */
    int num_elements;               /* max number of elements. */
    size_t memory_size;             /* Memory bytes allocate for hash_table. */
    int n_items;                    /* Number of elements in current table. */

    void *keys;                    /* Keys(names). */
    void *value;                   /* Values(binary date). */
    int *flag;                      /* 0 for free, 1 for used. */
    int *real_size;                 /* Real size for current value. */
}hash_table;


/*  
* Name:         make_hashtable
* Argument:     int, int
* Return:       void*
* Purpose:      Allocate a piece of memory and return a void 
*               pointer of the hashtable. 
* Note:         none
*/
void* make_hashtable(int num_elements, int max_element_size){
    size_t memory_size, temp_size;
    void *allocated;
    hash_table *hash_table_ptr;
    int status;

    /* Allocate memory for the hashtable. */
    memory_size = sizeof(hash_table) + num_elements*120 +
                  num_elements*max_element_size         +
                  num_elements*sizeof(int) + num_elements*(sizeof(int));
    
    allocated = mmap(NULL, memory_size, PROT_READ | PROT_WRITE,
                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    RETURN_ON_VALUE(allocated, NULL, "Cannot allocate memory, return.\n", 
        NULL);

    /* Cast first part of memory to the hashtable for return. */
    hash_table_ptr = (hash_table*)allocated;

    hash_table_ptr->memory_size = memory_size;
    hash_table_ptr->max_element_size = max_element_size;
    hash_table_ptr->num_elements = num_elements;
    hash_table_ptr->n_items = 0;

    /* Initialize the semaphore lock to binary. */
    status = sem_init(&(hash_table_ptr->lock), 1, 1);
    RETURN_AND_FREE_MEM(status, -1, "Cannot initilize semaphore, return.\n",
        NULL, allocated, memory_size);

    /* Initialize the string array for keys(name). */
    temp_size = sizeof(hash_table);
    hash_table_ptr->keys = allocated + temp_size;

    /* Initialize the string array for value. */
    temp_size += num_elements*120;
    //hash_table_ptr->value = allocated + temp_size;
    hash_table_ptr->value = (allocated+temp_size);

    /* Initialize the int array for flags. */
    temp_size += num_elements*max_element_size;;
    hash_table_ptr->flag = (int*)(allocated + temp_size);
    FORONE(i, num_elements)
        hash_table_ptr->flag[i] = 0;

    /* Initialize the int array for actual size of value. */
    temp_size += num_elements*sizeof(int);
    hash_table_ptr->real_size = (int*)(allocated + temp_size);
    FORONE(i, num_elements)
        hash_table_ptr->real_size[i] = -1;  /* <- -1 for no size. */
    

    #ifdef DEBUG
    printf("Initializing hash table..\n");
    printf("size is :%ld\n", memory_size);
    printf("size of hash_table struct:%ld\n", sizeof(hash_table));
    printf("hash_table addr:                %p\n", hash_table_ptr);
    printf("allocate addr:                  %p\n", allocated);
    printf("hash_table->keys addr:          %p\n", hash_table_ptr->keys);
    printf("hash_table->values addr:        %p\n", hash_table_ptr->value);
    printf("hash_table->flag addr:          %p\n", hash_table_ptr->flag);
    printf("hash_table->real_size addr:     %p\n", hash_table_ptr->real_size);
    printf("Initialize complete. \n--------------------\n");
    #endif /* DEBUG */

    return hash_table_ptr;
}


/*  
* Name:         hash
* Argument:     char*, int
* Return:       int
* Purpose:      hash function.
* Note:         djb2 hash function. this algorithm (k=33) was first 
*               reported by dan bernstein many years ago in comp.lang.c. 
*               another version of this algorithm (now favored by bernstein) 
*               uses xor: hash(i) = hash(i - 1) * 33 ^ str[i]; the magic 
*               of number 33 (why it works better than many other constants, 
*               prime or not) has never been adequately explained.
*/
int hash_func(char *str, int scale){
    unsigned long hash = 5381;
    int c, i=0;

    while ((c = *str++) != 0){
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
        i++;
    }
    return hash%scale;
}


/*  
* Name:         hash_set
* Argument:     vpid*, char*, void*, int
* Return:       int
* Purpose:      Create an entry in the hashtable. 
* Note:         Returns 0 on success. On errors, returns a value specific to 
*               the source of error, as follows:
*                   -1 if the hashtable is NULL
*                   -2 if the name is too long or is NULL
*                   -3 if data_size > maximum allowed element size
*                   -4 if no space exists for the element in the hashtable
*                   -99 if an error other than the above occurs.
*               Error codes are defined in hashtable.h.
*               The hash table is open addressing with quadratic probing with
*               no looping(once hash index is out of boundary, return 
*               HASH_ERR_COLISION). It will not go back from start.
*/
int hash_set(void *hashtable, char *name, void *data, int data_size){
    /* Cast hashtable pointer. */
    hash_table *temp = (hash_table*)hashtable;

    /* Check NULL pointers. */
    if (hashtable == NULL)
        return HASH_ERR_NULL;

    if (name == NULL)
        return HASH_ERR_NAME;

    if (strlen(name) > 120){
        return HASH_ERR_NAME;
    }

    /* Lock semaphore. */
    if (sem_wait(&temp->lock) != 0)
        return HASH_ERR_OTHER;

    if (data_size < 1){
        sem_post(&temp->lock);
        return HASH_ERR_OTHER;
    }

    if (data_size > temp->max_element_size){
        sem_post(&temp->lock);
        return HASH_ERR_DATASIZE;
    }


    /* Find a hash location. */
    int index = hash_func(name, temp->num_elements);
    /* Track times of searching. */
    int counter = 0;
    
    /* Linear probing. */
    while(temp->flag[index]!=0){
        if (memcmp(temp->keys+index*120, name, (strlen(name)+1)) == 0){
            memset(temp->value+index*temp->max_element_size, '\0', 
                   temp->real_size[index]);
            temp->real_size[index] = data_size;
            memcpy(temp->value+index*temp->max_element_size, data, data_size);
            sem_post(&temp->lock);
            return HASH_OK;
        }
        else{
            index++;
            index %= temp->num_elements;
            if (index >= temp->num_elements)
                index = 0;
            counter ++;
            if (counter > temp->num_elements){
                sem_post(&temp->lock);
                return HASH_ERR_COLISION;
            }
        }
    }
    
    temp->n_items++;

    memcpy(temp->keys+index*120, name, (strlen(name)+1));
    memcpy(temp->value+index*temp->max_element_size, data, data_size);
    temp->real_size[index] = data_size;
    temp->flag[index] = 1;

    #ifdef DEBUG
    printf("----------------------\n");
    printf("Trigger set..\n");
    printf("index now is :              %d\n", index);
    printf("keys location:              %p\n", temp->keys+index*120);
    printf("value start location:       %p\n", temp->value);
    printf("temp->keys[%d] is:          %s\n", index, temp->keys+index*120);
    printf("temp->value[%d] is:         %s\n", index, temp->value+index*temp->max_element_size);
    printf("temp->real_size[%d] is:     %d\n", index, temp->real_size[index]);
    printf("temp->flag[%d] is:          %d\n", index, temp->flag[index]);
    printf("----------------------\n");
    #endif /* DEBUG */

    sem_post(&temp->lock);
    return HASH_OK;
}

/*  
* Name:         hash_delete
* Argument:     void*, char*
* Return:       int
* Purpose:      Delete an entry in the hashtable.
* Note:         none
*/
int hash_delete(void *hashtable, char *name){
    /* Cast hashtable pointer. */
    hash_table *temp = (hash_table*)hashtable;

    /* Check NULL pointers. */
    if (hashtable == NULL)
        return HASH_ERR_NULL;

    if (name == NULL)
        return HASH_ERR_NAME;

    if (strlen(name) > 120){
        return HASH_ERR_NAME;
    }

    /* Lock semaphore. */
    if (sem_wait(&temp->lock) != 0)
        return HASH_ERR_OTHER;

    int index = hash_func(name, temp->num_elements);
    /* Track times of searching. */
    int counter = 0;

    /* Search through each quadratic probing entries. */
    while(memcmp(temp->keys+index*120, name, (strlen(name)+1)) != 0){
        index++;
        index %= temp->num_elements;
        if (index >= temp->num_elements)
            index = 0;
        counter ++;
        if (counter >= temp->num_elements){
            sem_post(&temp->lock);
            return HASH_ERR_NOEXIT;
        }
    }

    temp->n_items--;

    /* Reset data. */
    memset(temp->keys+index*120, '\0', 120);
    memset(temp->value+index*temp->max_element_size, '\0', temp->real_size[index]);
    temp->real_size[index] = -1;
    temp->flag[index] = 0;

    #ifdef DEBUG
    printf("----------------------\n");
    printf("Trigger delete:\n");
    printf("temp->keys[%d] is:      %s\n", index, temp->keys+index*120);
    printf("temp->value[%d] is:     %s\n", index, temp->value+index*temp->max_element_size);
    printf("temp->real_size[%d] is: %d\n", index, temp->real_size[index]);
    printf("temp->flag[%d] is:      %d\n", index, temp->flag[index]);
    printf("----------------------\n");
    #endif /* DEBUG */

    sem_post(&temp->lock);
    return HASH_OK;
}

/*  
* Name:         hash_get
* Argument:     void*, char*, void*, int*
* Return:       int
* Purpose:      Get an entry in the hashtable, if find, *buffer
*               will be link to a new piece of memory with that data.
* Note:         none
*/
int hash_get(void *hashtable, char *name, void **buffer, int *size){
    /* Cast hashtable pointer. */
    hash_table *temp = (hash_table*)hashtable;

    /* Check NULL pointers. */
    if (hashtable == NULL)
        return HASH_ERR_NULL;

    if (name == NULL)
        return HASH_ERR_NAME;

    if (strlen(name) > 120)
        return HASH_ERR_NAME;

    if (size == NULL)
        return HASH_ERR_SIZENULL;

    /* Lock semaphore. */
    if (sem_wait(&temp->lock) != 0)
        return HASH_ERR_OTHER;

    int index = hash_func(name, temp->num_elements); 
    /* Track times of searching. */
    int counter = 0;
    
    /* Linear probing. */
    /* Search through each quadratic probing entries. */
    while(memcmp(temp->keys+index*120, name, (strlen(name)+1)) != 0){
        index++;
        index %= temp->num_elements;
        if (index >= temp->num_elements)
            index = 0;
        counter ++;
        if (counter >= temp->num_elements){
            sem_post(&temp->lock);
            return HASH_ERR_NOEXIT;
        }
    }

    /* Create and return a new ptr. */
    void *temp_buffer = malloc(temp->real_size[index]);
    if (temp_buffer == NULL){
        sem_post(&temp->lock);
        return HASH_ERR_MEMALOFAIL;
    }

    memcpy(temp_buffer, temp->value+index*temp->max_element_size, 
           temp->real_size[index]);
    memcpy(size, &temp->real_size[index], sizeof(temp->real_size[index]));
    *buffer = temp_buffer;

    

    // *size = temp->real_size[index];

    #ifdef DEBUG
    printf("----------------------\n");
    printf("Triigger get..\n");
    printf("----------------------\n");
    #endif /* DEBUG */
    
    sem_post(&temp->lock);
    return HASH_OK;
}


/*  
* Name:         hash_detach
* Argument:     void*
* Return:       void
* Purpose:      Clear map, free the memory.
* Note:         none
*/
void hash_detach(void *hashtable){
    hash_table *temp = (hash_table*)hashtable;
    sem_destroy(&temp->lock);
    munmap(temp, temp->memory_size);
}

/*  
* Name:         hash_get_max_elements_size
* Argument:     void*
* Return:       int
* Purpose:      Getter method for max_elements size
* Note:         none
*/
int hash_get_max_elements_size(void *hashtable){
    hash_table *temp = (hash_table*)hashtable;
    return temp->max_element_size;
}

#endif      /* _HASH_TABLE_H_ */
