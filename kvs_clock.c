#include "kvs_clock.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct cache_entry {
  char key[KVS_KEY_MAX];
  char value[KVS_VALUE_MAX];
  int ref_bit;
  int done;
} cache_entry_t;

struct kvs_clock {
  kvs_base_t* base;
  int capacity;
  int entry_count;
  cache_entry_t* entries;
  int current_hand;
  int cache_set_count;
  int eviction_count;
};

kvs_clock_t* kvs_clock_new(kvs_base_t* base, int capacity) {
  kvs_clock_t* clock = malloc(sizeof(kvs_clock_t));
  clock->base = base;
  clock->capacity = capacity;
  clock->entry_count = 0;
  clock->entries = malloc(capacity * sizeof(cache_entry_t));
  clock->current_hand = 0;
  clock->cache_set_count = 0;
  clock->eviction_count = 0;
  for (int i = 0; i < capacity; i++) {
    clock->entries[i].ref_bit = 0;
    clock->entries[i].done = 0;
  }
  return clock;
}

void kvs_clock_free(kvs_clock_t** ptr) {
  if (ptr && *ptr) {
    free((*ptr)->entries);
    free(*ptr);
    *ptr = NULL;
  }
}

static void advance_hand(kvs_clock_t* clock) {
  clock->current_hand = (clock->current_hand + 1) % clock->capacity;
}

int kvs_clock_set(kvs_clock_t* clock, const char* key, const char* value) {
  for (int i = 0; i < clock->entry_count; i++) {
    int idx = (clock->current_hand + i) % clock->capacity;
    if (strcmp(clock->entries[idx].key, key) == 0) {
      strcpy(clock->entries[idx].value, value);
      clock->entries[idx].ref_bit = 1;
      clock->entries[idx].done = 1;
      clock->cache_set_count++;
      return SUCCESS;
    }
  }

  while (clock->entry_count == clock->capacity) {
    if (clock->entries[clock->current_hand].ref_bit == 0) {
      if (clock->entries[clock->current_hand].done) {
        kvs_base_set(clock->base, clock->entries[clock->current_hand].key,
                     clock->entries[clock->current_hand].value);
        clock->eviction_count++;
      }
      strcpy(clock->entries[clock->current_hand].key, key);
      strcpy(clock->entries[clock->current_hand].value, value);
      clock->entries[clock->current_hand].ref_bit = 1;
      clock->entries[clock->current_hand].done = 1;
      advance_hand(clock);
      clock->cache_set_count++;
      return SUCCESS;
    } else {
      clock->entries[clock->current_hand].ref_bit = 0;
      advance_hand(clock);
    }
  }

  strcpy(clock->entries[clock->current_hand].key, key);
  strcpy(clock->entries[clock->current_hand].value, value);
  clock->entries[clock->current_hand].ref_bit = 1;
  clock->entries[clock->current_hand].done = 1;
  advance_hand(clock);
  clock->entry_count++;
  clock->cache_set_count++;
  return SUCCESS;
}

int kvs_clock_get(kvs_clock_t* clock, const char* key, char* value) {
  for (int i = 0; i < clock->entry_count; i++) {
    int idx = (clock->current_hand + i) % clock->capacity;
    if (strcmp(clock->entries[idx].key, key) == 0) {
      strcpy(value, clock->entries[idx].value);
      clock->entries[idx].ref_bit = 1;
      return SUCCESS;
    }
  }

  int result = kvs_base_get(clock->base, key, value);
  if (result == SUCCESS) {
    while (clock->entry_count == clock->capacity) {
      if (clock->entries[clock->current_hand].ref_bit == 0) {
        if (clock->entries[clock->current_hand].done) {
          kvs_base_set(clock->base, clock->entries[clock->current_hand].key,
                       clock->entries[clock->current_hand].value);
          clock->eviction_count++;
        }
        break;
      } else {
        clock->entries[clock->current_hand].ref_bit = 0;
        advance_hand(clock);
      }
    }

    if (clock->entry_count < clock->capacity) {
      strcpy(clock->entries[clock->entry_count].key, key);
      strcpy(clock->entries[clock->entry_count].value, value);
      clock->entries[clock->entry_count].ref_bit = 1;
      clock->entries[clock->entry_count].done = 0;
      clock->entry_count++;
    } else {
      strcpy(clock->entries[clock->current_hand].key, key);
      strcpy(clock->entries[clock->current_hand].value, value);
      clock->entries[clock->current_hand].ref_bit = 1;
      clock->entries[clock->current_hand].done = 0;
      advance_hand(clock);
    }
  }
  return result;
}

int kvs_clock_flush(kvs_clock_t* clock) {
  for (int i = 0; i < clock->entry_count; i++) {
    if (clock->entries[i].done) {
      kvs_base_set(clock->base, clock->entries[i].key, clock->entries[i].value);
      clock->eviction_count++;
      clock->entries[i].done = 0;
    }
  }
  return SUCCESS;
}
