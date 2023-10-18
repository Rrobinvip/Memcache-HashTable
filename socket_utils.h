/*
 *  Thanks Duane!
 */

#ifndef _SOCKET_UTILS_H_
#define _SOCKET_UTILS_H_

typedef struct sockaddr SA;

/* Function: read_in_full
 * Wrapper function for read which reads until EOF, error, or all requested
 * data is read.  Necessary for dealing with signals and other network I/O
 * conditions which may cause read to abnormally abort.
 */
int read_in_full(int fd, void *data, size_t size);

/* Function: write_in_full
 * Wrapper function for write which writes until error, or all requested
 * data is written.  Necessary for dealing with signals and other network I/O
 * conditions which may cause write to abnormally abort.
 */
int write_in_full(int fd, void *data, size_t size);


#endif
