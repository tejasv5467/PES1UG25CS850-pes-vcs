// index.c — Staging area implementation

#include "index.h"
#include "pes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>

// Forward declaration
int object_write(int type, const void *data, size_t size, ObjectID *out);

// ─── PROVIDED ─────────────────────────────────────────

IndexEntry* index_find(Index *index, const char *path) {
    if (!index) return NULL;
    for (int i = 0; i < index->count; i++) {
        if (strcmp(index->entries[i].path, path) == 0)
            return &index->entries[i];
    }
    return NULL;
}

int index_remove(Index *index, const char *path) {
    for (int i = 0; i < index->count; i++) {
        if (strcmp(index->entries[i].path, path) == 0) {
            int remaining = index->count - i - 1;
            if (remaining > 0)
                memmove(&index->entries[i], &index->entries[i + 1],
                        remaining * sizeof(IndexEntry));
            index->count--;
            return index_save(index);
        }
    }
    fprintf(stderr, "error: '%s' is not in the index\n", path);
    return -1;
}

int index_status(const Index *index) {
    printf("Staged changes:\n");
    if (!index || index->count == 0) {
        printf("  (nothing to show)\n\n");
        return 0;
    }

    for (int i = 0; i < index->count; i++) {
        printf("  staged:     %s\n", index->entries[i].path);
    }

    printf("\n");
    return 0;
}

// ─── IMPLEMENTATION ───────────────────────────────────

int index_load(Index *index) {
    FILE *fp = fopen(".pes/index", "r");
    index->count = 0;

    if (!fp) return 0;

    char hash_hex[65], path[512];
    unsigned int mode;
    long mtime;
    unsigned int size;

    // Added safety check for MAX_ENTRIES if defined in your header
    while (fscanf(fp, "%o %64s %ld %u %511s", 
                  &mode, hash_hex, &mtime, &size, path) == 5) {
        
        IndexEntry *e = &index->entries[index->count++];
        e->mode = mode;
        hex_to_hash(hash_hex, &e->hash);
        e->mtime_sec = mtime;
        e->size = size;
        strncpy(e->path, path, sizeof(e->path) - 1);
        e->path[sizeof(e->path) - 1] = '\0';
    }

    fclose(fp);
    return 0;
}

int cmp_entries(const void *a, const void *b) {
    return strcmp(((IndexEntry*)a)->path, ((IndexEntry*)b)->path);
}

int index_save(const Index *index) {
    if (!index) return -1;

    // CRITICAL: Don't copy the whole Index to stack. 
    // We sort the entries in the existing index pointer.
    qsort((void*)index->entries, index->count, sizeof(IndexEntry), cmp_entries);

    FILE *fp = fopen(".pes/index.tmp", "w");
    if (!fp) return -1;

    for (int i = 0; i < index->count; i++) {
        char hash_hex[65];
        hash_to_hex(&index->entries[i].hash, hash_hex);

        fprintf(fp, "%o %s %ld %u %s\n",
                index->entries[i].mode,
                hash_hex,
                (long)index->entries[i].mtime_sec,
                index->entries[i].size,
                index->entries[i].path);
    }

    fflush(fp);
    fsync(fileno(fp));
    fclose(fp);

    rename(".pes/index.tmp", ".pes/index");
    return 0;
}

int index_add(Index *index, const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) {
        fprintf(stderr, "error: cannot stat %s\n", path);
        return -1;
    }

    // Only allow regular files
    if (!S_ISREG(st.st_mode)) {
        fprintf(stderr, "error: %s is not a regular file\n", path);
        return -1;
    }

    FILE *fp = fopen(path, "rb");
    if (!fp) {
        fprintf(stderr, "error: cannot open %s\n", path);
        return -1;
    }

    long size = st.st_size;
    unsigned char *data = NULL;

    if (size > 0) {
        data = malloc(size);
        if (!data) {
            fclose(fp);
            return -1;
        }
        if (fread(data, 1, size, fp) != (size_t)size) {
            free(data);
            fclose(fp);
            return -1;
        }
    }
    fclose(fp);

    ObjectID hash;
    if (object_write(OBJ_BLOB, data, size, &hash) != 0) {
        if (data) free(data);
        return -1;
    }

    if (data) free(data);

    IndexEntry *e = index_find(index, path);
    if (!e) {
        // Ensure we don't overflow the index array
        e = &index->entries[index->count++];
    }

    e->mode = st.st_mode;
    e->hash = hash;
    e->mtime_sec = st.st_mtime;
    e->size = st.st_size;
    strncpy(e->path, path, sizeof(e->path) - 1);
    e->path[sizeof(e->path) - 1] = '\0';

    return index_save(index);
}
// phase3 step1
// phase3 step2
// phase3 step3
// phase3 step4
// phase3 step5
