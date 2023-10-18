# Memcache with Shared Hashtable

## Overview

This project extends a basic memcache program to include a shared hashtable. The shared hashtable is implemented in shared memory and uses a semaphore for synchronization. The memcache program accepts TCP connections and performs various operations like SET, GET, and DELETE. 

## Technical Skills Demonstrated

- **C Programming**: Advanced usage of C for system-level programming.
- **Memory Management**: Efficient use of shared memory and custom memory allocation strategies.
- **Signal Handling**: Robust handling of system signals for graceful shutdowns.
- **System Calls**: Extensive use of system calls for process and network management.
- **Multi-Processing**: Effective use of child processes to handle multiple client connections concurrently.

## Highlights

- **Highly Concurrent**: Utilizes semaphores to ensure process-safe operations on the shared hashtable.
- **Efficient Memory Usage**: The shared hashtable is implemented in shared memory, manual control, optimizing resource utilization.
- **Robust Error Handling**: Comprehensive error codes and messages for easy debugging and fault tolerance.
- **Command-Line Customization**: Allows customization of hashtable size and element size via command-line arguments.
- **Zero Downtime**: Controlled shutdown feature ensures graceful termination, cleaning up resources without affecting ongoing operations.

## Features

- **Shared Hashtable**: Implemented in `shared_hashtable.h` and `shared_hashtable.c`.
- **TCP Connection**: Accepts and handles multiple client connections.
- **Commands**: Supports SET, GET, DELETE.
- **Error Handling**: Detailed error messages for various edge cases.

## Installation

Program is only compatiable with Linux (No cross OS compatability), tested on Ubuntu 20.04 LTS. 

```bash
make
```

## Usage

```bash
./memcache [num_elements] [element_size]
```

### Commands

- `SET <name> <size>`: Sets a value in the shared hashtable.
- `GET <name>`: Retrieves a value from the shared hashtable.
- `DELETE <name>`: Deletes a value from the shared hashtable.

## Cleanup

On controlled shutdown:

- Detaches from the shared hashtable.
- Cleans up remaining child processes.

## Additional Notes

- No zombies allowed.
- Makefile included.
- Over 40 edge cases have been successfully passed, ensuring robustness!
