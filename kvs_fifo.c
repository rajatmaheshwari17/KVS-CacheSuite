#include "kvs_fifo.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct cache_entry {
  char key[KVS_KEY_MAX];
  char value[KVS_VALUE_MAX];
  bool modified;
} cache_entry_t;

struct kvs_fifo {
  kvs_base_t* kvs_base;
  int capacity;
  int size;
  cache_entry_t* entries;
  int head;
  int tail;
  int get_count_cache;
  int get_count_disk;
  int set_count_cache;
  int set_count_disk;
};

kvs_fifo_t* kvs_fifo_new(kvs_base_t* kvs, int capacity) {
  kvs_fifo_t* kvs_fifo = malloc(sizeof(kvs_fifo_t));
  kvs_fifo->kvs_base = kvs;
  kvs_fifo->capacity = capacity;
  kvs_fifo->size = 0;
  kvs_fifo->entries = malloc(capacity * sizeof(cache_entry_t));
  kvs_fifo->head = 0;
  kvs_fifo->tail = 0;
  kvs_fifo->get_count_cache = 0;
  kvs_fifo->get_count_disk = 0;
  kvs_fifo->set_count_cache = 0;
  kvs_fifo->set_count_disk = 0;
  return kvs_fifo;
}

void kvs_fifo_free(kvs_fifo_t** ptr) {
  if (ptr && *ptr) {
    free((*ptr)->entries);
    free(*ptr);
    *ptr = NULL;
  }
}

int kvs_fifo_set(kvs_fifo_t* kvs_fifo, const char* key, const char* value) {
  for (int i = 0; i < kvs_fifo->size; i++) {
    int index = (kvs_fifo->head + i) % kvs_fifo->capacity;
    if (strcmp(kvs_fifo->entries[index].key, key) == 0) {
      strcpy(kvs_fifo->entries[index].value, value);
      kvs_fifo->entries[index].modified = true;
      kvs_fifo->set_count_cache++;
      return SUCCESS;
    }
  }

  if (kvs_fifo->size == kvs_fifo->capacity) {
    if (kvs_fifo->entries[kvs_fifo->head].modified) {
      kvs_base_set(kvs_fifo->kvs_base, kvs_fifo->entries[kvs_fifo->head].key,
                   kvs_fifo->entries[kvs_fifo->head].value);
      kvs_fifo->set_count_disk++;
    }
    kvs_fifo->head = (kvs_fifo->head + 1) % kvs_fifo->capacity;
  } else {
    kvs_fifo->size++;
  }

  strcpy(kvs_fifo->entries[kvs_fifo->tail].key, key);
  strcpy(kvs_fifo->entries[kvs_fifo->tail].value, value);
  kvs_fifo->entries[kvs_fifo->tail].modified = true;
  kvs_fifo->tail = (kvs_fifo->tail + 1) % kvs_fifo->capacity;
  kvs_fifo->set_count_cache++;
  return SUCCESS;
}

int kvs_fifo_get(kvs_fifo_t* kvs_fifo, const char* key, char* value) {
  for (int i = 0; i < kvs_fifo->size; i++) {
    int index = (kvs_fifo->head + i) % kvs_fifo->capacity;
    if (strcmp(kvs_fifo->entries[index].key, key) == 0) {
      strcpy(value, kvs_fifo->entries[index].value);
      kvs_fifo->get_count_cache++;
      return SUCCESS;
    }
  }

  int result = kvs_base_get(kvs_fifo->kvs_base, key, value);
  if (result == SUCCESS) {
    kvs_fifo->get_count_disk++;
    if (kvs_fifo->size == kvs_fifo->capacity) {
      if (kvs_fifo->entries[kvs_fifo->head].modified) {
        kvs_base_set(kvs_fifo->kvs_base, kvs_fifo->entries[kvs_fifo->head].key,
                     kvs_fifo->entries[kvs_fifo->head].value);
        kvs_fifo->set_count_disk++;
      }
      kvs_fifo->head = (kvs_fifo->head + 1) % kvs_fifo->capacity;
    } else {
      kvs_fifo->size++;
    }

    strcpy(kvs_fifo->entries[kvs_fifo->tail].key, key);
    strcpy(kvs_fifo->entries[kvs_fifo->tail].value, value);
    kvs_fifo->entries[kvs_fifo->tail].modified = false;
    kvs_fifo->tail = (kvs_fifo->tail + 1) % kvs_fifo->capacity;
  }

  return result;
}

int kvs_fifo_flush(kvs_fifo_t* kvs_fifo) {
  for (int i = 0; i < kvs_fifo->size; i++) {
    int index = (kvs_fifo->head + i) % kvs_fifo->capacity;
    if (kvs_fifo->entries[index].modified) {
      kvs_base_set(kvs_fifo->kvs_base, kvs_fifo->entries[index].key,
                   kvs_fifo->entries[index].value);
      kvs_fifo->entries[index].modified = false;
      kvs_fifo->set_count_disk++;
    }
  }
  kvs_fifo->size = 0;
  kvs_fifo->head = 0;
  kvs_fifo->tail = 0;
  return SUCCESS;
}
