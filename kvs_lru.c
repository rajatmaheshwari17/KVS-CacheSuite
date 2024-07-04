#include "kvs_lru.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct cache_entry {
  char key[KVS_KEY_MAX];
  char value[KVS_VALUE_MAX];
  bool modified;
  struct cache_entry* prev;
  struct cache_entry* next;
} cache_entry_t;

struct kvs_lru {
  kvs_base_t* kvs_base;
  int capacity;
  int size;
  cache_entry_t* head;
  cache_entry_t* tail;
  cache_entry_t** map;
  int get_count_cache;
  int get_count_disk;
  int set_count_cache;
  int set_count_disk;
};

static cache_entry_t* create_cache_entry(const char* key, const char* value,
                                         bool modified) {
  cache_entry_t* entry = malloc(sizeof(cache_entry_t));
  strncpy(entry->key, key, KVS_KEY_MAX);
  strncpy(entry->value, value, KVS_VALUE_MAX);
  entry->modified = modified;
  entry->prev = NULL;
  entry->next = NULL;
  return entry;
}

static void remove_entry(kvs_lru_t* kvs_lru, cache_entry_t* entry) {
  if (entry->prev) {
    entry->prev->next = entry->next;
  }
  if (entry->next) {
    entry->next->prev = entry->prev;
  }
  if (entry == kvs_lru->head) {
    kvs_lru->head = entry->next;
  }
  if (entry == kvs_lru->tail) {
    kvs_lru->tail = entry->prev;
  }
}

static void add_to_head(kvs_lru_t* kvs_lru, cache_entry_t* entry) {
  entry->next = kvs_lru->head;
  entry->prev = NULL;
  if (kvs_lru->head) {
    kvs_lru->head->prev = entry;
  }
  kvs_lru->head = entry;
  if (kvs_lru->tail == NULL) {
    kvs_lru->tail = entry;
  }
}

static void move_to_head(kvs_lru_t* kvs_lru, cache_entry_t* entry) {
  remove_entry(kvs_lru, entry);
  add_to_head(kvs_lru, entry);
}

static void remove_tail(kvs_lru_t* kvs_lru) {
  if (kvs_lru->tail) {
    if (kvs_lru->tail->modified) {
      kvs_base_set(kvs_lru->kvs_base, kvs_lru->tail->key, kvs_lru->tail->value);
      kvs_lru->set_count_disk++;
    }
    cache_entry_t* old_tail = kvs_lru->tail;
    remove_entry(kvs_lru, old_tail);
    free(old_tail);
    kvs_lru->size--;
  }
}

kvs_lru_t* kvs_lru_new(kvs_base_t* kvs, int capacity) {
  kvs_lru_t* kvs_lru = malloc(sizeof(kvs_lru_t));
  kvs_lru->kvs_base = kvs;
  kvs_lru->capacity = capacity;
  kvs_lru->size = 0;
  kvs_lru->head = NULL;
  kvs_lru->tail = NULL;
  kvs_lru->map = calloc(capacity, sizeof(cache_entry_t*));
  kvs_lru->get_count_cache = 0;
  kvs_lru->get_count_disk = 0;
  kvs_lru->set_count_cache = 0;
  kvs_lru->set_count_disk = 0;
  return kvs_lru;
}

void kvs_lru_free(kvs_lru_t** ptr) {
  if (ptr && *ptr) {
    cache_entry_t* current = (*ptr)->head;
    while (current) {
      cache_entry_t* next = current->next;
      free(current);
      current = next;
    }
    free((*ptr)->map);
    free(*ptr);
    *ptr = NULL;
  }
}

int kvs_lru_set(kvs_lru_t* kvs_lru, const char* key, const char* value) {
  for (cache_entry_t* entry = kvs_lru->head; entry; entry = entry->next) {
    if (strcmp(entry->key, key) == 0) {
      strncpy(entry->value, value, KVS_VALUE_MAX);
      entry->modified = true;
      move_to_head(kvs_lru, entry);
      kvs_lru->set_count_cache++;
      return SUCCESS;
    }
  }

  if (kvs_lru->size == kvs_lru->capacity) {
    remove_tail(kvs_lru);
  }

  cache_entry_t* new_entry = create_cache_entry(key, value, true);
  add_to_head(kvs_lru, new_entry);
  kvs_lru->size++;
  kvs_lru->set_count_disk++;

  return SUCCESS;
}

int kvs_lru_get(kvs_lru_t* kvs_lru, const char* key, char* value) {
  for (cache_entry_t* entry = kvs_lru->head; entry; entry = entry->next) {
    if (strcmp(entry->key, key) == 0) {
      strncpy(value, entry->value, KVS_VALUE_MAX);
      move_to_head(kvs_lru, entry);
      kvs_lru->get_count_cache++;
      return SUCCESS;
    }
  }

  int result = kvs_base_get(kvs_lru->kvs_base, key, value);
  if (result == SUCCESS) {
    if (kvs_lru->size == kvs_lru->capacity) {
      remove_tail(kvs_lru);
    }

    cache_entry_t* new_entry = create_cache_entry(key, value, false);
    add_to_head(kvs_lru, new_entry);
    kvs_lru->size++;
    kvs_lru->get_count_disk++;
  }

  return result;
}

int kvs_lru_flush(kvs_lru_t* kvs_lru) {
  for (cache_entry_t* current = kvs_lru->head; current;
       current = current->next) {
    if (current->modified) {
      kvs_base_set(kvs_lru->kvs_base, current->key, current->value);
      current->modified = false;
      kvs_lru->set_count_disk++;
    }
  }
  return SUCCESS;
}
