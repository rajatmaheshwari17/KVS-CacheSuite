# KVS-CacheSuite

KVS-CacheSuite is a C-based project that implements a cached layer on top of a file-based key-value store (KVS). This project aims to improve the efficiency of key-value operations by introducing in-memory caching with three different cache replacement policies: FIFO, Clock, and LRU.

## Features

-   **File-Based Key-Value Store**: Provides basic SET and GET operations directly interacting with the file system.
-   **Cached Key-Value Store**: Implements an in-memory caching layer to handle operations more efficiently.
-   **Cache Replacement Policies**:
    -   **FIFO (First-In First-Out)**: Evicts the oldest entry in the cache.
    -   **Clock**: Uses a circular list and reference bits to manage evictions.
    -   **LRU (Least Recently Used)**: Evicts the least recently accessed entry.
-   **Operations**:
    -   **SET**: Updates or adds an entry in the cache.
    -   **GET**: Retrieves an entry from the cache or file system.
    -   **FLUSH**: Persists all modified entries in memory to the disk.

## Getting Started

### Setup

1.  **Clone the repository**:
 
    
    `git clone https://github.com/UCSC-CSE-130/project4-cacheflex-kvs.git
    cd project4-cacheflex-kvs` 
    
2.  **Build the project**:
    
    `make` 
    
3.  **Format the source files**:
    
    `make format` 
    

## Usage

The provided `client.c` program offers a command-line interface to interact with the key-value store.

### Command

`./client DIRECTORY POLICY CAPACITY` 

-   **DIRECTORY**: Directory used by the key-value store.
-   **POLICY**: Cache replacement policy (`NONE`, `FIFO`, `CLOCK`, or `LRU`).
-   **CAPACITY**: Size of the cache (ignored if `POLICY` is `NONE`).

### Example


`./client data FIFO 2
SET file1.txt hey
GET file1.txt` 

### Statistics

At the end of the session, the client prints statistics like GET and SET counts, and cache hit rates.

## Implementation Details

### Files

-   `kvs_base.c`: Base KVS implementation.
-   `kvs_fifo.c`, `kvs_clock.c`, `kvs_lru.c`: Cached KVS implementations with respective replacement policies.
-   `client.c`: Command-line interface for testing the KVS.

### Functions

-   **kvs_{fifo, clock, lru}_new**: Constructor for the cached KVS.
-   **kvs_{fifo, clock, lru}_free**: Destructor for the cached KVS.
-   **kvs_{fifo, clock, lru}_set**: SET operation for the cached KVS.
-   **kvs_{fifo, clock, lru}_get**: GET operation for the cached KVS.
-   **kvs_{fifo, clock, lru}_flush**: FLUSH operation for the cached KVS.

## Testing

Use the provided `client` program to test the KVS implementation. Ensure to check for memory leaks using `valgrind`.

## Limitations

-   Maximum length for keys and values are defined in `constants.h`.
-   Single-threaded key-value store operations.
#
_This README is a part of the KVS-CacheSuite Project by Rajat Maheshwari._
