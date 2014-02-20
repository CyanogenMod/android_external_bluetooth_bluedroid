#pragma once

#include <stdbool.h>
#include <stdlib.h>

struct list_node_t;
typedef struct list_node_t list_node_t;

struct list_t;
typedef struct list_t list_t;

typedef void (*list_free_cb)(void *data);
typedef bool (*list_iter_cb)(void *data);
typedef bool (*list_iter_cb_ext)(void *data, void *cb_data);

// Lifecycle.
list_t *list_new(list_free_cb callback);
void list_free(list_t *list);

// Accessors.
bool list_is_empty(const list_t *list);
size_t list_length(const list_t *list);
void *list_front(const list_t *list);
void *list_back(const list_t *list);

// Mutators.
bool list_insert_after(list_t *list, list_node_t *prev_node, void *data);
bool list_prepend(list_t *list, void *data);
bool list_append(list_t *list, void *data);
bool list_remove(list_t *list, void *data);
void list_clear(list_t *list);

// Iteration.
void list_foreach(const list_t *list, list_iter_cb callback);
void list_foreach_ext(const list_t *list, list_iter_cb_ext callback, void *cb_data);

list_node_t *list_begin(const list_t *list);
list_node_t *list_end(const list_t *list);
list_node_t *list_next(const list_node_t *node);
void *list_node(const list_node_t *node);
