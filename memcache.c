/* 
 *  File:        memcache.c
 *  Author:      Haoyu Shi
 *  Student ID:  100143687
 *  Version:     2.0
 *  Date:        2021.2.11
 *  Purpose:     A server create a TCP socket which will handle date from client.
 * 
 *               ./memcache <port> <num_elements> <element_size>
 *               port:          should be between 0 and 65535.
 *               num_elements:  should be >= 1.
 *               elements_size: should be >= 1.
 * 
 *               Read one lien of text from the client:
 *               <CMD> <name> <size>
 *               CMD:           SET, GET, DELETE.
 *               name:          should be shorter than 120, must be a-z, A-Z, 0-9.
 *               size:          should only be with commend "SET".
 * 
 *               Different with previous will be comment out with: ADD.
 * 
 *  Note:        The program will send ERR<error message> back to the client. 
 *               If no errors countered, "OK" would be sent. 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <ctype.h>

#include "utility_macros.h"
#include "socket_utils.h"
#include "shared_hashtable.h"

#define MAX_LIS_QUEUE   10
#define MAX_INPUT_SIZE  1024
#define DELIMETER_NW    "\r\t\n"
#define DELIIMETER      " \t"

int child_spawn;
static int is_interrupted = 0;
//static int client_number = 1;


/*  
* Name:         sigint_recived
* Argument:     int
* Return:       none
* Purpose:      Set signal flag once received interrupt. 
* Note:         none
*/
void sigint_received(int signum) {
    printf("%d\n", signum);
    is_interrupted = 1;
}

/*  
* Name:         split_str
* Argument:     char*, char**
* Return:       Number of elements in out_str
* Purpose:      Split a commend into form of the char** that recognizeable
                by "exec()" family. 
* Note:         none
*/
int split_str(char *row_str, char **out_str){
    char *pch = strtok(row_str, DELIIMETER);
    int index = 0;
    while(pch != NULL){
        out_str[index] = malloc(MAX_INPUT_SIZE);
        EXIT_ON_VALUE(out_str[index], NULL, "Cannot allocate memory, exit.\n",
                      EXIT_FAILURE);
        strcpy(out_str[index], pch);
        pch = strtok(NULL, DELIIMETER); 
        index++;  
    }
    /* Terminate the char** with NULL. */
    out_str[index] = NULL;
    return index;
}


/*  
* Name:         argv_check
* Argument:     int*
* Return:       int
* Purpose:      Check if argvs contain any error and mistake.
* Note:         none
*/
int argv_check(int *argv_in){
    if (argv_in[0]<0 || argv_in[0]>65535) return 0;
    else if (argv_in[1]<1 || argv_in[2]<1) return 0;
    child_spawn=0;      /*  <--- Pass error checking, initialize static variable. */
    return 1;
}

int main(int argc, char **argv){
    int argv_in[3];     /*  <--- argv transformed to int. */
    
    /* 
     * status_o: for storing operations status. 
     * status_s: for special operations status, espacilly save for later use.
     * flag:     for which commend mode it's in, -1 as defult. 
     * status_hash: for reading result returned by hashtable function. 
     */
    int status, status_2, status_3, status_o, status_exit, flag=-1, 
        status_hash, server_socket;

    struct sockaddr_in address /*client_address*/;
    //int client_address_size;

    /* Initial cmd_list for compare. 
    *  0 for SET.
    *  1 for GET.
    *  2 for DELETE.
    */
    char cmd_list[3][10] = {{"SET\0"}, {"GET\0"}, {"DELETE\0"}};

    /* Argument checking. */
    EXIT_NOT_ON_VALUE(argc, 4, "TOO MANY OR TO FEW ARGUMENTS, EXIT.\n", 
                      EXIT_FAILURE);
    FORONE(i, 3){
        status = sscanf(argv[i+1],"%d", &argv_in[i]);
        EXIT_NOT_ON_VALUE(status, 1, "BAD COMMANDLINE ARGUMENT, EXIT.\n", 
                          EXIT_FAILURE);
    } 
    if (!argv_check(argv_in)){
        fprintf(stderr,"BAD COMMANDLINE ARGUMENT, EXIT.\n");
        exit(EXIT_FAILURE);
    }

    /* ADD: Initialize a hashtable with shared memory. */
    fprintf(stderr, "num_elements:%d\nmax_num_elements:%d\n", argv_in[1], argv_in[2]);
    void *hash_table_ptr = make_hashtable(argv_in[1], argv_in[2]);
    EXIT_ON_VALUE(hash_table_ptr, NULL, "Cannot locate share memory, exit.\n",
                  EXIT_FAILURE);

    /* Set up signal handler. */
    struct sigaction sigint_handler;

    sigint_handler.sa_handler = sigint_received;
    sigemptyset(&sigint_handler.sa_mask);
    sigint_handler.sa_flags = 0;
    
    status_3 = sigaction(SIGINT, &sigint_handler, NULL);
    if (status_3 != 0) {
        perror(argv[0]);
        exit(EXIT_FAILURE);
    }

    /* Define an address that means port defined by user on all my network interfaces. */
    address.sin_family = AF_INET;
    address.sin_port = htons(argv_in[0]);
    address.sin_addr.s_addr = htonl(INADDR_ANY);

    /* Create a TCP socket. */
    server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    EXIT_ON_VALUE(server_socket, -1, "SOCKET CREATION FAILED, EXIT.\n", 
        EXIT_FAILURE);
    fprintf(stderr, "socket created: %d\n", server_socket);

    /* Assign it the address defined above. */
    status = bind(server_socket, (SA*)&address, sizeof(address));
    EXIT_ON_VALUE(status, -1, "BIND FAILED, EXIT.\n", EXIT_FAILURE);
    fprintf(stderr, "bound: %d\n", status);

    /* Ask the kernel to make it a listening socket with a queue of 10 */
    status = listen(server_socket, MAX_LIS_QUEUE);
    EXIT_ON_VALUE(status, -1, "LISTEN CREATION FAILED, EXIT.\n", EXIT_FAILURE);
    fprintf(stderr, "Server is running, waiting for connections..\n");

    while(1){
        /* Handle signal interrupt. */
        if (is_interrupted == 1) {
            fprintf(stderr, "\nReceived interrupt, ready to "
                "controlled shutdown.\n");
            fprintf(stderr, "Number of max child now is: "
                "%d\nWaiting..\n", (child_spawn));
            FORONE(i, child_spawn){
                int result;
                int status_55;
                status_55 = wait(&result);
                if (status_55 == -1) perror("MSG");
            }
            fprintf(stderr, 
                    "All child process are finished, Detaching memory...\n");
            /* ADD: detach hashtable while control shutdown. */
            hash_detach(hash_table_ptr);
            fprintf(stderr, "Shared memory detached, exit now.\n");
            exit(EXIT_SUCCESS);
        }

        struct sockaddr_in client_address;
        socklen_t address_size;

        /* Wait to accept a new connection from a client */
        int client = accept(server_socket, (SA*)&client_address, &address_size);
        if (client==-1 && errno == EINTR) continue;
        EXIT_ON_VALUE(client, -1, 
            "------------------\nACCEPT FAILED, EXIT.\n", EXIT_FAILURE);
             
        fprintf(stderr, "---------------\nAccepted.Chind No: %d.File No: %d\n",
                child_spawn, client);

        /* Create child process to handle client request. */
        int child_pid = fork();
        EXIT_ON_VALUE(child_pid, -1, "CHILD PROCESS CREATION FAILED, EXIT.\n", 
                      EXIT_FAILURE);

        if(child_pid==0){
            fprintf(stderr, "----------------\nIn child #: %d\n", child_spawn);
            
            /* Read in data from client, romove \r\n from back. */
            char *row_r = malloc(MAX_INPUT_SIZE);
            EXIT_ON_VALUE(row_r, NULL, "CANNOT ALLOCATE MEMORY, EXIT. \n",
                          EXIT_FAILURE);

            status = read(client, row_r, MAX_INPUT_SIZE);
            EXIT_ON_VALUE(status, -1, "CANNOT READ, EXIT.\n", EXIT_FAILURE);

            size_t row_size = strlen(row_r);

            fprintf(stderr, "bytes read:%d, real size:%ld\n -With client:%d\n",
                    status, row_size,(child_spawn+1));
            fprintf(stderr, "row_r is :%s\n", row_r);
            
            char *temp_pch = strtok(row_r, "\r");
            char *row = malloc(MAX_INPUT_SIZE);
            EXIT_ON_VALUE(row, NULL, "CANNOT ALLOCATE MEMORY, EXIT. \n",
                          EXIT_FAILURE);

            strcpy(row, temp_pch);

            char *remain_row = malloc(MAX_INPUT_SIZE);
            EXIT_ON_VALUE(remain_row, NULL, "CANNOT ALLOCATE MEMORY, EXIT. \n",
                          EXIT_FAILURE);

            fprintf(stderr, "row:%s\n", row);

            size_t compare_status = (status-2);

            /* 
             * Data send always ended with \r\n, compare bytes read in to
             * decide if there are any remaining data. 
             */
            if (row_size > compare_status){
                temp_pch = strtok(NULL, "");
                strcpy(remain_row, (temp_pch+1));
                fprintf(stderr, "Remaining row is :%s\nWith bytes:%ld\n", 
                        remain_row, strlen(remain_row));
                fprintf(stderr, "Row commend include 'r' is :%ld\n", 
                        strlen(row));
            }

            /* 
             *   Do not perform read_in_full() and write_in_full() on unknow 
             *   length of data. 
             */
            

            fprintf(stderr, "DATA is:%s\nwith length:%ld\n -With client:%d\n",
                    row, strlen(row), (child_spawn+1));
            
 
            char *cmd = malloc(MAX_INPUT_SIZE); 
            EXIT_ON_VALUE(cmd, NULL, "CANNOT ALLOCATE MEMORY, EXIT. \n", 
                          EXIT_FAILURE);

            /* size appear SET commend. */
            int size;

            /* String list for commend. */
            char **input_cmd = malloc(MAX_INPUT_SIZE*sizeof(input_cmd[0]));
            EXIT_ON_VALUE(input_cmd, NULL, "CANNOT ALLOCATE MEMORY, EXIT. \n",
                EXIT_FAILURE);
            /* cmd_size should be 3 with SET, 2 with others. */
            int cmd_size = split_str(row, input_cmd);

            /* name(key) for hash. */
            char *name = input_cmd[1];

            fprintf(stderr, "DEBUG: cmd_size is:%d\n", cmd_size);

            /* Error checking process. */
            FORONE(i, 3){
                status_2 = strcmp(cmd_list[i], input_cmd[0]);
                if (status_2 == 0) {
                    flag = i;
                    break;
                }
                status_2 = 100;
            }
            fprintf(stderr, "CMD is :%s\n - With client:%d\n",cmd_list[flag], 
                (child_spawn+1));
            fprintf(stderr, "DEBUG: the flag is:%d\n", flag);

            /* First element is not command. */
            if (status_2 == 100){
                char err_msg[] = "ERR INVALID_COMMAND\r\n";
                status_o = write_in_full(client, err_msg, strlen(err_msg));
                //status = write(client, err_msg, strlen(err_msg));
                EXIT_ON_VALUE(status, -1, "ERR OTHER\r\n", EXIT_FAILURE);

                /* Prepare to close. */
                status_o = close(client);
                EXIT_ON_VALUE(status_o, -1, "ERR OTHER\r\n", EXIT_FAILURE);
                child_spawn--;
                FREE_ON_VAL(row, cmd, name);
                
                exit(EXIT_FAILURE);
            }

            /* Commend "SET" requires 3 exzact arguments. */
            if(flag == 0 && cmd_size != 3){

                char err_msg[] = "ERR INVALID_COMMAND\r\n";
                fprintf(stderr, "DEBUG: triggered. \n");

                status_o = write_in_full(client, err_msg, strlen(err_msg));
                //status = write(client, err_msg, strlen(err_msg));
                EXIT_ON_VALUE(status, -1, "ERR OTHER\r\n", EXIT_FAILURE);

                /* Prepare to close. */
                status_o = close(client);
                EXIT_ON_VALUE(status_o, -1, "ERR OTHER\r\n", EXIT_FAILURE);
                child_spawn--;
                FREE_ON_VAL(row, cmd, name);

                
                exit(EXIT_FAILURE);
                
            }
            /* Commend "GET" requires 2 exzact arguments. */
            else if(flag==1 && cmd_size != 2){
                char err_msg[] = "ERR INVALID_COMMAND\r\n";
                status_o = write_in_full(client, err_msg, strlen(err_msg));
                //status = write(client, err_msg, strlen(err_msg));
                EXIT_ON_VALUE(status, -1, "ERR OTHER\r\n", EXIT_FAILURE);

                /* Prepare to close. */
                status_o = close(client);
                EXIT_ON_VALUE(status_o, -1, "ERR OTHER\r\n", EXIT_FAILURE);
                child_spawn--;
                FREE_ON_VAL(row, cmd, name);

                
                exit(EXIT_FAILURE);
            }
            /* Commend "DELETE" requries 2 exzact arguements. */
            else if(flag==2 && cmd_size != 2){
                char err_msg[] = "ERR INVALID_COMMAND\r\n";
                status_o = write_in_full(client, err_msg, strlen(err_msg));
                //status = write(client, err_msg, strlen(err_msg));
                EXIT_ON_VALUE(status, -1, "ERR OTHER\r\n", EXIT_FAILURE);

                /* Prepare to close. */
                status_o = close(client);
                EXIT_ON_VALUE(status_o, -1, "ERR OTHER\r\n", EXIT_FAILURE);
                child_spawn--;
                FREE_ON_VAL(row, cmd, name);

                
                exit(EXIT_FAILURE);
            }
            else fprintf(stderr, "Error checking: arguments # Passed!\n");

            /* Name should be shorter than 120. */
            fprintf(stderr, "DEBUG: name size is:%ld\n", strlen(name));
            if (strlen(name)>120){
                char err_msg[] = "ERR NAME_TOO_LONG\r\n";
                status_o = write_in_full(client, err_msg, strlen(err_msg));
                EXIT_ON_VALUE(status_o, -1, "ERR OTHER\r\n", EXIT_FAILURE);
                fprintf(stderr, "DEBUG: message:%d\n", status_o);

                /* Prepare to close. */
                status_o = close(client);
                EXIT_ON_VALUE(status_o, -1, "ERR OTHER\r\n", EXIT_FAILURE);
                child_spawn--;
                FREE_ON_VAL(row, cmd, name);

                exit(EXIT_FAILURE);
            }
            fprintf(stderr, "Error checking: name length Passed!\n");

            /* Name should not contain except a-z, A-Z, 0-9. */
            FORONE(i, (int)strlen(name)){
                if (!((name[i]>=48 && name[i]<=57)||
                    (name[i]>=65 && name[i]<=90)||
                    (name[i]>=97 && name[i]<=122))){
                        char err_msg[] = "ERR BAD_NAME\r\n";
                        status_o = write_in_full(client, err_msg, 
                            strlen(err_msg));
                        EXIT_ON_VALUE(status_o, -1, "ERR OTHER\r\n", 
                            EXIT_FAILURE);

                        /* Prepare to close. */
                        status_o = close(client);
                        EXIT_ON_VALUE(status_o, -1, "ERR OTHER\r\n", 
                            EXIT_FAILURE);
                        child_spawn--;
                        FREE_ON_VAL(row, cmd, name);

                        exit(EXIT_FAILURE);
                    }
            }
            fprintf(stderr, "Error checking: name character Passed!\n");
            

            /* Size should be an int where at least 1. */
            if (flag == 0){
                fprintf(stderr, "INTO THIS STEP1\ninput_cmd[2] is:%s\n", 
                    input_cmd[2]);
                for (size_t i = 0; i < strlen(input_cmd[2]); i++){
                    if (!isdigit(input_cmd[2][i])){
                        fprintf(stderr, "INTO THIS STEP\n");
                        char err_msg[] = "ERR INVALID_SIZE\r\n";
                        status_o = write_in_full(client, err_msg, 
                            strlen(err_msg));
                        EXIT_ON_VALUE(status_o, -1, "ERR OTHER\r\n", 
                            EXIT_FAILURE);

                        /* Prepare to close. */
                        status_o = close(client);
                        EXIT_ON_VALUE(status_o, -1, "ERR OTHER\r\n", 
                            EXIT_FAILURE);
                        child_spawn--;
                        FREE_ON_VAL(row, cmd, name);

                        exit(EXIT_FAILURE);
                    }
                }

                sscanf(input_cmd[2], "%d", &size);

                /* ADD: compare size with max_elements_size. */
                if (size<1 || sizeof(size)!=sizeof(int) || 
                    size > hash_get_max_elements_size(hash_table_ptr)){
                        char err_msg[] = "ERR INVALID_SIZE\r\n";
                        status_o = write_in_full(client, err_msg, 
                            strlen(err_msg));
                        EXIT_ON_VALUE(status_o, -1, "ERR OTHER\r\n", 
                            EXIT_FAILURE);

                        /* Prepare to close. */
                        status_o = close(client);
                        EXIT_ON_VALUE(status_o, -1, "ERR OTHER\r\n", 
                            EXIT_FAILURE);
                        child_spawn--;
                        FREE_ON_VAL(row, cmd, name);

                        exit(EXIT_FAILURE);
                }
            }
            fprintf(stderr, "Error checking: SET size Passed!\n");

            /* End of error checking.*/

            /* Start performing operations depends on different commend. */
            /* flag status: 0 for SET, 1 for GET, 2 for DELETE. */
            if (flag == 0){
                /* Data with everythiing after commend. */
                void *buffer_1 = malloc(MAX_INPUT_SIZE);
                //status_s = read(client, buffer_1, MAX_INPUT_SIZE);
                size_t compare_size = size;

                // char *pch = strtok(buffer_1, DELIMETER_NW);
                // fprintf(stderr, "Status_s is original: %d\n", status_s);
                // if (strlen(pch)<=strlen(buffer_1)) status_s = strlen(pch);
                
                fprintf(stderr, "size is %d\nremaining row is %ld\n", 
                    size, strlen(remain_row));
                /* Check if client send to much data. */
                if (strlen(remain_row) < compare_size){
                    char err_msg[] = "ERR TOO_SMALL\r\n";
                    status_o = write_in_full(client, err_msg, strlen(err_msg));
                    EXIT_ON_VALUE(status, -1, "ERR OTHER\r\n", EXIT_FAILURE);

                    /* Prepare to close. */
                    status_o = close(client);
                    EXIT_ON_VALUE(status, -1, "ERR OTHER\r\n", EXIT_FAILURE);
                    child_spawn--;
                    FREE_ON_VAL(row, cmd, name); FREE(buffer_1);

                    exit(EXIT_FAILURE);
                }
                /* SET commend all error checking pass, handling data. */
                else{
                    memcpy(buffer_1, remain_row, size);
                    fprintf(stderr, "Reamining data waiting for handling is"
                        " :%s\n", buffer_1);

                    /* ADD: set the value in the shared hashtable for the give
                    *  name and value (the first <size> bytes after the first 
                    *  line of the request read above in (g)).*/
                    status_hash = hash_set(hash_table_ptr, name, buffer_1, size);

                    char *err_msg = malloc(20*sizeof(char));
                    if (status_hash == HASH_OK) 
                        strcpy(err_msg, "OK\r\n");
                    else if (status_hash == HASH_ERR_COLISION)
                        strcpy(err_msg, "ERR NO_SPACE\r\n");
                    else
                        strcpy(err_msg, "ERR OTHER\r\n");
                    
                    status_o = write_in_full(client, err_msg, strlen(err_msg));
                    EXIT_ON_VALUE(status_o, -1, "ERR OTHER\r\n", EXIT_FAILURE);

                    /* Prepare to close. */
                    status_o = close(client);
                    EXIT_ON_VALUE(status_o, -1, "ERR OTHER\r\n", EXIT_FAILURE);

                    fprintf(stderr, "DEBUG: close OK successful.\n");
                    child_spawn--;
                    FREE_ON_VAL(row, cmd, name); FREE(buffer_1);
                    FREE(err_msg);

                    exit(EXIT_SUCCESS);
                }
            }
            /* GET commend all error checking pass, handling data. */
            else if (flag == 1){
                /* ADD: The process should attempt to get the value of 
                *  the element from the hashtable from the entry with 
                *  the given name, and the size in bytes of the element.
                */ 
                
                int size_from_hash;
                void *data_out;
                status_hash = hash_get(hash_table_ptr, name, &(data_out), 
                                       &size_from_hash);   

                char *err_msg = malloc(20*sizeof(char));
                if (status_hash == HASH_OK)
                    sprintf(err_msg, "OK %d\r\n%s", size_from_hash, 
                            (char*)data_out);
                else if (status_hash == HASH_ERR_NOEXIT)
                    strcpy(err_msg, "ERR NOT_FOUND\r\n");
                else
                    strcpy(err_msg, "ERR OTHER\r\n");

                status_o = write_in_full(client, err_msg, strlen(err_msg));
                EXIT_ON_VALUE(status_o, -1, "ERR OTHER\r\n", EXIT_FAILURE);

                /* Prepare to close. */
                status_o = close(client);
                EXIT_ON_VALUE(status_o, -1, "ERR OTHER\r\n", EXIT_FAILURE);
                child_spawn--;
                FREE_ON_VAL(row, cmd, name); FREE(err_msg);

                exit(EXIT_SUCCESS);
            }
            /* DELETE commend all error checking pass, hadnling data. */
            else{
                /* ADD: The process should attempt to delete the element 
                *  in the hashtable with the given name.
                */
                status_hash = hash_delete(hash_table_ptr, name);
                char *err_msg = malloc(20*sizeof(char));
                if (status_hash == HASH_OK)
                    strcpy(err_msg, "OK 1\r\n");
                else if (status_hash == HASH_ERR_NOEXIT)
                    strcpy(err_msg, "OK 0\r\n");
                else
                    strcpy(err_msg, "ERR OTHER\r\n");

                status_o = write_in_full(client, err_msg, strlen(err_msg));
                EXIT_ON_VALUE(status_o, -1, "ERR OTHER\r\n", EXIT_FAILURE);

                /* Prepare to close. */
                status_o = close(client);
                EXIT_ON_VALUE(status_o, -1, "ERR OTHER\r\n", EXIT_FAILURE);
                child_spawn--;
                FREE_ON_VAL(row, cmd, name); FREE(err_msg);

                exit(EXIT_SUCCESS);
            }

            write(client, "TEST, no errors.\n", 17);

        }
        else{
            close(client);
            while(waitpid(0, &status_exit, WNOHANG|WUNTRACED)!= 0)
                ;
            child_spawn++;
        }
    }

    exit(EXIT_SUCCESS);
}
