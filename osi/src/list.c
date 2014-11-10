#include <assert.h>

#include "list.h"
#include "osi.h"

typedef struct list_node_t {
  struct list_node_t *next;
  void *data;
} list_node_t;

typedef struct list_t {
  list_node_t *head;
  list_node_t *tail;
  size_t length;
  list_free_cb free_cb;
} list_t;

static list_node_t *list_free_node_(list_t *list, list_node_t *node);

// Returns a new, empty list. Returns NULL if not enough memory could be allocated
// for the list structure. The returned list must be freed with |list_free|. The
// |callback| specifies a function to be called whenever a list element is removed
// from the list. It can be used to release resources held by the list element, e.g.
// memory or file descriptor. |callback| may be NULL if no cleanup is necessary on
// element removal.
list_t *list_new(list_free_cb callback) {
  list_t *list = (list_t *)calloc(sizeof(list_t), 1);
  if (list)
    list->free_cb = callback;
  return list;
}

// Frees the list. This function accepts NULL as an argument, in which case it
// behaves like a no-op.
void list_free(list_t *list) {
  if (list != NULL)
    list_clear(list);

  free(list);
}

// Returns true if the list is empty (has no elements), false otherwise.
// Note that a NULL list is not the same as an empty list. This function
// does not accept a NULL list.
bool list_is_empty(const list_t *list) {
  assert(list != NULL);
  if (list)
    return (list->length == 0);
  else
    return true;
}

// Returns the length of the list. This function does not accept a NULL list.
size_t list_length(const list_t *list) {
  assert(list != NULL);
  if (list)
    return list->length;
  else
    return 0;
}

// Returns the first element in the list without removing it. |list| may not
// be NULL or empty.
void *list_front(const list_t *list) {
  assert(list != NULL);
  assert(!list_is_empty(list));
  if (list && list->head)
    return list->head->data;
  return NULL;
}

// Returns the last element in the list without removing it. |list| may not
// be NULL or empty.
void *list_back(const list_t *list) {
  assert(list != NULL);
  assert(!list_is_empty(list));
  if (list && list->tail)
    return list->tail->data;
  return NULL;
}

bool list_insert_after(list_t *list, list_node_t *prev_node, void *data) {
  assert(list != NULL);
  assert(prev_node != NULL);
  assert(data != NULL);
  if (!list || !prev_node)
    return false;
  list_node_t *node = (list_node_t *)malloc(sizeof(list_node_t));
  if (!node)
    return false;

  node->next = prev_node->next;
  node->data = data;
  prev_node->next = node;
  if (list->tail == prev_node)
    list->tail = node;
  ++list->length;
  return true;
}

// Inserts |data| at the beginning of |list|. Neither |data| nor |list| may be NULL.
// This function does not make a copy of |data| so the pointer must remain valid
// at least until the element is removed from the list or the list is freed.
// Returns true if |data| could be inserted, false otherwise (e.g. out of memory).
bool list_prepend(list_t *list, void *data) {
  assert(list != NULL);
  assert(data != NULL);
  if (!list)
    return false;
  list_node_t *node = (list_node_t *)malloc(sizeof(list_node_t));
  if (!node)
    return false;
  node->next = list->head;
  node->data = data;
  list->head = node;
  if (list->tail == NULL)
    list->tail = list->head;
  ++list->length;
  return true;
}

// Inserts |data| at the end of |list|. Neither |data| nor |list| may be NULL.
// This function does not make a copy of |data| so the pointer must remain valid
// at least until the element is removed from the list or the list is freed.
// Returns true if |data| could be inserted, false otherwise (e.g. out of memory).
bool list_append(list_t *list, void *data) {
  assert(list != NULL);
  assert(data != NULL);
  if (!list)
    return false;
  list_node_t *node = (list_node_t *)malloc(sizeof(list_node_t));
  if (!node)
    return false;
  node->next = NULL;
  node->data = data;
  if (list->tail == NULL) {
    list->head = node;
    list->tail = node;
  } else {
    list->tail->next = node;
    list->tail = node;
  }
  ++list->length;
  return true;
}

// Removes |data| from the list. Neither |list| nor |data| may be NULL. If |data|
// is inserted multiple times in the list, this function will only remove the first
// instance. If a free function was specified in |list_new|, it will be called back
// with |data|. This function returns true if |data| was found in the list and removed,
// false otherwise.
bool list_remove(list_t *list, void *data) {
  assert(list != NULL);
  assert(data != NULL);

  if (!list || list_is_empty(list))
    return false;

  if (list->head->data == data) {
    list_node_t *next = list_free_node_(list, list->head);
    if (list->tail == list->head)
      list->tail = next;
    list->head = next;
    return true;
  }

  for (list_node_t *prev = list->head, *node = list->head->next; node; prev = node, node = node->next)
    if (node->data == data) {
      prev->next = list_free_node_(list, node);
      if (list->tail == node)
        list->tail = prev;
      return true;
    }

  return false;
}

// Removes all elements in the list. Calling this function will return the list to the
// same state it was in after |list_new|. |list| may not be NULL.
void list_clear(list_t *list) {
  assert(list != NULL);
  if (list)
    for (list_node_t *node = list->head; node; )
      node = list_free_node_(list, node);
  list->head = NULL;
  list->tail = NULL;
  list->length = 0;
}

// Iterates through the entire |list| and calls |callback| for each data element.
// If the list is empty, |callback| will never be called. It is safe to mutate the
// list inside the callback. If an element is added before the node being visited,
// there will be no callback for the newly-inserted node. Neither |list| nor
// |callback| may be NULL.
void list_foreach(const list_t *list, list_iter_cb callback) {
  assert(list != NULL);
  assert(callback != NULL);
  if (list)
    for (list_node_t *node = list->head; node; ) {
      list_node_t *next = node->next;
      callback(node->data);
      node = next;
    }
}

// Iterates through the entire |list| and calls |callback| for each data element.
// Passes the caller provided data along with node
// If the list is empty, |callback| will never be called. It is safe to mutate the
// list inside the callback. If an element is added before the node being visited,
// there will be no callback for the newly-inserted node. Neither |list| nor
// |callback| may be NULL.
void list_foreach_ext(const list_t *list, list_iter_cb_ext callback, void *cb_data) {
  assert(list != NULL);
  assert(callback != NULL);
  if (list)
    for (list_node_t *node = list->head; node; ) {
      list_node_t *next = node->next;
      callback(node->data, cb_data);
      node = next;
    }
}

// Returns an iterator to the first element in |list|. |list| may not be NULL.
// The returned iterator is valid as long as it does not equal the value returned
// by |list_end|.
list_node_t *list_begin(const list_t *list) {
  assert(list != NULL);
  if (list)
    return list->head;
  else
    return NULL;
}

// Returns an iterator that points past the end of the list. In other words,
// this function returns the value of an invalid iterator for the given list.
// When an iterator has the same value as what's returned by this function, you
// may no longer call |list_next| with the iterator. |list| may not be NULL.
list_node_t *list_end(UNUSED_ATTR const list_t *list) {
  assert(list != NULL);
  return NULL;
}

// Given a valid iterator |node|, this function returns the next value for the
// iterator. If the returned value equals the value returned by |list_end|, the
// iterator has reached the end of the list and may no longer be used for any
// purpose.
list_node_t *list_next(const list_node_t *node) {
  assert(node != NULL);
  if (node)
    return node->next;
  else
    return NULL;
}

// Returns the value stored at the location pointed to by the iterator |node|.
// |node| must not equal the value returned by |list_end|.
void *list_node(const list_node_t *node) {
  assert(node != NULL);
  if (node)
    return node->data;
  return false;
}

static list_node_t *list_free_node_(list_t *list, list_node_t *node) {
  assert(list != NULL);
  assert(node != NULL);

  list_node_t *next = node ?node->next: NULL;

  if (list)
  {
    if (list->free_cb)
      list->free_cb(node->data);
    free(node);
    --list->length;
  }

  return next;
}
