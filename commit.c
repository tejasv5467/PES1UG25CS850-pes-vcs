// commit.c — Commit creation and history implementation

#include "commit.h"
#include "tree.h"
#include "index.h"
#include "pes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Forward declarations
int object_write(int type, const void *data, size_t size, ObjectID *out);
int object_read(const ObjectID *id, int *type, void **data, size_t *size);

// ─── HEAD MANAGEMENT ───────────────────────────────────────

int head_read(ObjectID *id) {
    FILE *fp = fopen(".pes/HEAD", "r");
    if (!fp) return -1;
    char hex[65];
    if (fscanf(fp, "%64s", hex) != 1) {
        fclose(fp);
        return -1;
    }
    fclose(fp);
    return hex_to_hash(hex, id);
}

int head_update(const ObjectID *id) {
    FILE *fp = fopen(".pes/HEAD", "w");
    if (!fp) return -1;
    char hex[65];
    hash_to_hex(id, hex);
    fprintf(fp, "%s\n", hex);
    fclose(fp);
    return 0;
}

// ─── COMMIT CREATE ─────────────────────────────────────────

int commit_create(const char *message, ObjectID *commit_id_out) {
    ObjectID tree_oid;
    if (tree_from_index(&tree_oid) != 0) return -1;

    ObjectID parent_oid;
    int has_parent = (head_read(&parent_oid) == 0);

    const char *author = pes_author();
    long timestamp = (long)time(NULL);

    char buffer[4096];
    memset(buffer, 0, sizeof(buffer));
    int len = 0;

    char tree_hex[65];
    hash_to_hex(&tree_oid, tree_hex);
    len += sprintf(buffer + len, "tree %s\n", tree_hex);

    if (has_parent) {
        char parent_hex[65];
        hash_to_hex(&parent_oid, parent_hex);
        len += sprintf(buffer + len, "parent %s\n", parent_hex);
    }

    len += sprintf(buffer + len, "author %s %ld\n", author, timestamp);
    len += sprintf(buffer + len, "committer %s %ld\n\n", author, timestamp);
    len += sprintf(buffer + len, "%s\n", message);

    ObjectID commit_oid;
    if (object_write(OBJ_COMMIT, buffer, len, &commit_oid) != 0) return -1;
    if (head_update(&commit_oid) != 0) return -1;
    if (commit_id_out) *commit_id_out = commit_oid;

    return 0;
}

// ─── COMMIT LOG (REPLACED VERSION) ─────────────────────────

int commit_log(void) {
    ObjectID current;

    if (head_read(&current) != 0) {
        printf("No commits yet.\n");
        return 0;
    }

    while (1) {
        int type;
        size_t size;
        void *data = NULL;

        // read object safely
        if (object_read(&current, &type, &data, &size) != 0) {
            fprintf(stderr, "error: failed to read commit\n");
            return -1;
        }

        if (type != OBJ_COMMIT) {
            fprintf(stderr, "error: not a commit object\n");
            free(data);
            return -1;
        }

        // convert to string safely
        char *content = malloc(size + 1);
        if (!content) {
            free(data);
            return -1;
        }
        memcpy(content, data, size);
        content[size] = '\0';

        free(data);

        char hash_hex[65];
        hash_to_hex(&current, hash_hex);

        printf("commit %s\n", hash_hex);
        printf("%s\n", content);

        // find parent
        char *parent_line = strstr(content, "parent ");
        if (!parent_line) {
            free(content);
            break;
        }

        char parent_hex[65];
        if (sscanf(parent_line, "parent %64s", parent_hex) != 1) {
            free(content);
            break;
        }

        if (hex_to_hash(parent_hex, &current) != 0) {
            free(content);
            break;
        }

        free(content);
    }

    return 0;
}

// Keeping a wrapper if pes.c calls commit_walk
int commit_walk(commit_walk_fn callback, void *ctx) {
    (void)callback; (void)ctx;
    return commit_log();
}
// phase4 step1
// phase4 step2
// phase4 step3
// phase4 step4
// phase4 step5
