#ifndef _TH_HASH_TABLE_H_
#define _TH_HASH_TABLE_H_

#include <stddef.h>

#define HASH_OK             0               /* HASH operation succeeded. */
#define HASH_ERR_NULL       -1              /* Error: hashtable is NULL. */
#define HASH_ERR_NAME       -2              /* Error: name is too long or NULL. */
#define HASH_ERR_DATASIZE   -3              /* Error: data_size > maximum allowed size. */
#define HASH_ERR_COLISION   -4              /* Error: collision, no space.  */
#define HASH_ERR_NOEXIT     -5              /* Error: the element doesn't exits. */
#define HASH_ERR_MEMALOFAIL -6              /* Error: memory allocation fail when get. */
#define HASH_ERR_SIZENULL   -7              /* Error: Size is null when get. */
#define HASH_ERR_OTHER      -99             /* Error: any other errors. */


/*  
* Name:         make_hashtable
* Argument:     int, int
* Return:       void*
* Purpose:      Allocate a piece of memory and return a void 
*               pointer of the hashtable. 
* Note:         none
*/
void* make_hashtable(int num_elements, int max_element_size);


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
int hash_func(char *str, int scale);


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
int hash_set(void *hashtable, char *name, void *data, int data_size);

/*  
* Name:         hash_delete
* Argument:     void*, char*
* Return:       int
* Purpose:      Delete an entry in the hashtable.
* Note:         none
*/
int hash_delete(void *hashtable, char *name);

/*  
* Name:         hash_get
* Argument:     void*, char*, void*, int*
* Return:       int
* Purpose:      Get an entry in the hashtable, if find, *buffer
*               will be link to a new piece of memory with that data.
* Note:         none
*/
int hash_get(void *hashtable, char *name, void **buffer, int *size);

/*  
* Name:         hash_detach
* Argument:     void*
* Return:       void
* Purpose:      Clear map, free the memory.
* Note:         none
*/
void hash_detach(void *hashtable);

/*  
* Name:         hash_get_max_elements_size
* Argument:     void*
* Return:       int
* Purpose:      Getter method for max_elements size
* Note:         none
*/
int hash_get_max_elements_size(void *hashtable);


#endif      /* _TH_HASH_TABLE_H_ */