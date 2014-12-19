#define LOG_TAG "bt_osi_config"

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utils/Log.h>

#include "config.h"
#include "list.h"

typedef struct {
  char *key;
  char *value;
} entry_t;

typedef struct {
  char *name;
  list_t *entries;
} section_t;

struct config_t {
  list_t *sections;
};

static void config_parse(FILE *fp, config_t *config);

static section_t *section_new(const char *name);
static void section_free(void *ptr);
static section_t *section_find(const config_t *config, const char *section);

static entry_t *entry_new(const char *key, const char *value);
static void entry_free(void *ptr);
static entry_t *entry_find(const config_t *config, const char *section, const char *key);

config_t *config_new(const char *filename) {
  assert(filename != NULL);

  FILE *fp = fopen(filename, "rt");
  if (!fp) {
    ALOGE("%s unable to open file '%s': %s", __func__, filename, strerror(errno));
    return NULL;
  }

  config_t *config = calloc(1, sizeof(config_t));
  if (!config) {
    ALOGE("%s unable to allocate memory for config_t.", __func__);
    fclose(fp);
    return NULL;
  }

  config->sections = list_new(section_free);
  config_parse(fp, config);

  fclose(fp);

  return config;
}

void config_free(config_t *config) {
  if (!config)
    return;

  list_free(config->sections);
  free(config);
}

bool config_has_section(const config_t *config, const char *section) {
  assert(config != NULL);
  assert(section != NULL);

  return (section_find(config, section) != NULL);
}

bool config_has_key(const config_t *config, const char *section, const char *key) {
  assert(config != NULL);
  assert(section != NULL);
  assert(key != NULL);

  return (entry_find(config, section, key) != NULL);
}

int config_get_int(const config_t *config, const char *section, const char *key, int def_value) {
  assert(config != NULL);
  assert(section != NULL);
  assert(key != NULL);

  entry_t *entry = entry_find(config, section, key);
  if (!entry)
    return def_value;

  char *endptr;
  int ret = strtol(entry->value, &endptr, 0);
  return (*endptr == '\0') ? ret : def_value;
}

bool config_get_bool(const config_t *config, const char *section, const char *key, bool def_value) {
  assert(config != NULL);
  assert(section != NULL);
  assert(key != NULL);

  entry_t *entry = entry_find(config, section, key);
  if (!entry)
    return def_value;

  if (!strcmp(entry->value, "true"))
    return true;
  if (!strcmp(entry->value, "false"))
    return false;

  return def_value;
}

const char *config_get_string(const config_t *config, const char *section, const char *key, const char *def_value) {
  assert(config != NULL);
  assert(section != NULL);
  assert(key != NULL);

  entry_t *entry = entry_find(config, section, key);
  if (!entry)
    return def_value;

  return entry->value;
}

void config_set_int(config_t *config, const char *section, const char *key, int value) {
  assert(config != NULL);
  assert(section != NULL);
  assert(key != NULL);

  char value_str[32] = { 0 };
  sprintf(value_str, "%d", value);
  config_set_string(config, section, key, value_str);
}

void config_set_bool(config_t *config, const char *section, const char *key, bool value) {
  assert(config != NULL);
  assert(section != NULL);
  assert(key != NULL);

  config_set_string(config, section, key, value ? "true" : "false");
}

void config_set_string(config_t *config, const char *section, const char *key, const char *value) {
  section_t *sec = section_find(config, section);
  if (!sec) {
    sec = section_new(section);
    if (sec)
      list_append(config->sections, sec);
    else
    {
      ALOGE("%s: Unable to allocate memory for section", __func__);
      return;
    }
  }

  for (const list_node_t *node = list_begin(sec->entries); node != list_end(sec->entries); node = list_next(node)) {
    entry_t *entry = list_node(node);
    if (!strcmp(entry->key, key)) {
      free(entry->value);
      entry->value = strdup(value);
      return;
    }
  }

  entry_t *entry = entry_new(key, value);
  list_append(sec->entries, entry);
}

static char *trim(char *str) {
  while (isspace(*str))
    ++str;

  if (!*str)
    return str;

  char *end_str = str + strlen(str) - 1;
  while (end_str > str && isspace(*end_str))
    --end_str;

  end_str[1] = '\0';
  return str;
}

static void config_parse(FILE *fp, config_t *config) {
  assert(fp != NULL);
  assert(config != NULL);

  int line_num = 0;
  char line[1024];
  char section[1024];
  strcpy(section, CONFIG_DEFAULT_SECTION);

  while (fgets(line, sizeof(line), fp)) {
    char *line_ptr = trim(line);
    ++line_num;

    // Skip blank and comment lines.
    if (*line_ptr == '\0' || *line_ptr == '#')
      continue;

    if (*line_ptr == '[') {
      size_t len = strlen(line_ptr);
      if (line_ptr[len - 1] != ']') {
        ALOGD("%s unterminated section name on line %d.", __func__, line_num);
        continue;
      }
      strncpy(section, line_ptr + 1, len - 2);
      section[len - 2] = '\0';
    } else {
      char *split = strchr(line_ptr, '=');
      if (!split) {
        ALOGD("%s no key/value separator found on line %d.", __func__, line_num);
        continue;
      }

      *split = '\0';
      config_set_string(config, section, trim(line_ptr), trim(split + 1));
    }
  }
}

static section_t *section_new(const char *name) {
  section_t *section = calloc(1, sizeof(section_t));
  if (!section)
    return NULL;

  section->name = strdup(name);
  section->entries = list_new(entry_free);
  return section;
}

static void section_free(void *ptr) {
  if (!ptr)
    return;

  section_t *section = ptr;
  free(section->name);
  list_free(section->entries);
}

static section_t *section_find(const config_t *config, const char *section) {
  for (const list_node_t *node = list_begin(config->sections); node != list_end(config->sections); node = list_next(node)) {
    section_t *sec = list_node(node);
    if (!strcmp(sec->name, section))
      return sec;
  }

  return NULL;
}

static entry_t *entry_new(const char *key, const char *value) {
  entry_t *entry = calloc(1, sizeof(entry_t));
  if (!entry)
    return NULL;

  entry->key = strdup(key);
  entry->value = strdup(value);
  return entry;
}

static void entry_free(void *ptr) {
  if (!ptr)
    return;

  entry_t *entry = ptr;
  free(entry->key);
  free(entry->value);
}

static entry_t *entry_find(const config_t *config, const char *section, const char *key) {
  section_t *sec = section_find(config, section);
  if (!sec)
    return NULL;

  for (const list_node_t *node = list_begin(sec->entries); node != list_end(sec->entries); node = list_next(node)) {
    entry_t *entry = list_node(node);
    if (!strcmp(entry->key, key))
      return entry;
  }

  return NULL;
}
