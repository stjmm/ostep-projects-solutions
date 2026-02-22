/*
 * initial-kv project implemented using hash table with chaining (maybe overkill)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DB_FILE "kv.db"

/* Hash Table */

typedef struct ht_item {
    int key;
    char *val;
    struct ht_item *next;
} ht_item;

typedef struct {
    int size;
    int count;
    ht_item **buckets;
} hash_table;

hash_table *ht_new()
{
    hash_table *ht = malloc(sizeof(hash_table));
    ht->size = 53;
    ht->count = 0;
    ht->buckets = calloc(ht->size, sizeof(ht_item*));

    return ht;
}

int hash(hash_table *ht, int key)
{
    return (key < 0 ? -key : key) % ht->size;
}

void ht_resize(hash_table *ht)
{
    int old_size = ht->size;
    ht_item **old_buckets = ht->buckets;

    ht->size *= 2;
    ht->buckets = calloc(ht->size, sizeof(ht_item*));
    ht->count = 0;

    for (int i = 0; i < old_size; i++)
    {
        ht_item *cur = old_buckets[i];
        while (cur) {
            ht_item *next = cur->next;

            // Re-insert
            int idx = hash(ht, cur->key);
            cur->next = ht->buckets[idx];
            ht->buckets[idx] = cur;
            ht->count++;

            cur = next;
        }
    }

    free(old_buckets);
}

void ht_put(hash_table *ht, int key, char *val)
{
    if ((float)ht->count / ht->size > 0.7f) {
        ht_resize(ht);
    }

    int idx = hash(ht, key);
    ht_item *cur = ht->buckets[idx];

    // Walk the chain - update if exists
    while (cur) {
        if (cur->key == key) {
            free(cur->val);
            cur->val = strdup(val);
            return;
        }

        cur = cur->next;
    }

    // Key not found -> prepend to chain
    ht_item *item = malloc(sizeof(ht_item));
    item->key = key;
    item->val = strdup(val);
    item->next = ht->buckets[idx];
    ht->buckets[idx] = item;
    ht->count++;
}

char *ht_get(hash_table *ht, int key)
{
    int idx = hash(ht, key);
    ht_item *cur = ht->buckets[idx];
    while (cur) {
        if (cur->key == key) return cur->val;
        cur = cur->next;
    }

    return NULL;
}

void ht_delete(hash_table *ht, int key)
{
    int idx = hash(ht, key);
    ht_item *cur = ht->buckets[idx];
    ht_item *prev = NULL;

    while (cur) {
        if (cur->key == key) {
            if (prev) prev->next = cur->next;
            else ht->buckets[idx] = cur->next; // Cur is the first one before we set prev
            free(cur->val);
            free(cur);
            ht->count--;
            return;
        }
        prev = cur; 
        cur = cur->next;
    }
    printf("%d not found\n", key);
}

void ht_print_all(hash_table *ht)
{
    for (int i = 0; i < ht->size; i++) {
        ht_item *cur = ht->buckets[i];
        while (cur) {
            printf("%d,%s\n", cur->key, cur->val);
            cur = cur->next;
        }
    }
}

void ht_delete_all(hash_table *ht)
{
    for (int i = 0; i < ht->size; i++) {
        ht_item *cur = ht->buckets[i];
        while (cur) {
            ht_item *next = cur->next;
            free(cur->val);
            free(cur);
            cur = next;
        }
        ht->buckets[i] = NULL;
    }
    ht->count = 0;
}

/* File Saving */

hash_table *load_ht_from_file()
{
    hash_table *ht = ht_new();
    FILE *db = fopen(DB_FILE, "r");
    if (!db) return ht;

    char *line = NULL;
    size_t line_size = 0;
    int key;
    char val[64];

    while (getline(&line, &line_size, db) != -1) {
        if (sscanf(line, "%d,%63s", &key, val) == 2) {
            ht_put(ht, key, val);
        }
    }

    free(line);
    fclose(db);
    return ht;
}

void save_ht_to_file(hash_table *ht)
{
    FILE *db = fopen(DB_FILE, "w");
    if (db == NULL) {
        printf("kv: failed to open file for writing\n");
        return;
    }

    for (int i = 0; i < ht->size; i++) {
        ht_item *cur = ht->buckets[i];
        while (cur) {
            fprintf(db, "%d,%s\n", cur->key, cur->val);
            cur = cur->next;
        }
    }

    fclose(db);
}

void process_arg(hash_table *ht, char *arg)
{
    int key;
    char val[64];

    switch (arg[0]) {
        case 'p':
            if (sscanf(arg, "p,%d,%63s", &key, val) == 2) {
                ht_put(ht, key, val);
            }
            break;
        case 'g':
            if (sscanf(arg, "g,%d", &key) == 1) {
                char *val = ht_get(ht, key);
                if (val != NULL) printf("%d,%s\n", key, val);
                else printf("%d not found\n", key);
            }
            break;
        case 'd':
            if (sscanf(arg, "d,%d", &key) == 1) {
                ht_delete(ht, key);
            }
            break;
        case 'a':
            ht_print_all(ht);
            break;
        case 'c':
            ht_delete_all(ht);
            break;
        default:
            printf("kv: bad command (%s)\n", arg);
            break;
    }
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        return 0;
    }

    hash_table *ht = load_ht_from_file();

    for (int i = 1; i < argc; i++) {
        process_arg(ht, argv[i]);
    }

    save_ht_to_file(ht);

    return 0;
}
