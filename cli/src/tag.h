#ifndef TAG_H
#define TAG_H

#include <stddef.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/stat.h>

extern const char *Default;

struct tag {
    int wd;
    char *name;
};

extern struct tag_list {
    pthread_mutex_t lock;
    int wd;
    struct tag *tags;
    size_t all;
    size_t num, cap;
    uint8_t **archs;
    size_t archn;
} TagList;

#define TAG_ID(_name) \
({ \
    const char *const _n = (_name); \
    size_t _id; \
    for (_id = 0; _id < TagList.num; _id++) { \
        if (strcmp(TagList.tags[_id].name, _n) == 0) { \
            break; \
        } \
    } \
    _id; \
})

#define TAG_ID_L(_name, len) \
({ \
    const char *const _n = (_name); \
    const size_t _l = (len); \
    size_t _id; \
    for (_id = 0; _id < TagList.num; _id++) { \
        const char *const _o = TagList.tags[_id].name; \
        if (strncmp(_o, _n, _l) == 0 && _o[_l] == '\0') { \
            break; \
        } \
    } \
    _id; \
})

#define ARCH_SIZE() (TagList.num / 8 + 1)

#define CONTAINS_TAGS(a, ao) \
({ \
    const uint8_t *const _a = TagList.archs[a]; \
    const uint8_t *const _o = TagList.archs[ao]; \
    const size_t _s = ARCH_SIZE(); \
    size_t _i; \
    for (_i = 0; _i < _s; _i++) { \
        if ((_a[_i] & _o[_i]) != _o[_i]) { \
            break; \
        } \
    } \
    _i == _s; \
})

#define HAS_TAGS(a, ao) \
({ \
    const uint8_t *const _a = TagList.archs[a]; \
    const uint8_t *const _o = TagList.archs[ao]; \
    const size_t _s = ARCH_SIZE(); \
    size_t _i; \
    for (_i = 0; _i < _s; _i++) { \
        if (_a[_i] != _o[_i]) { \
            break; \
        } \
    } \
    _i == _s; \
})

#define ADD_TAG(a, i) \
({ \
    uint8_t *const _a = TagList.archs[a]; \
    const size_t _i = (i); \
    _a[_i / 8] |= 1 << (_i % 8); \
})

#define REMOVE_TAG(a, i) \
({ \
    uint8_t *const _a = TagList.archs[a]; \
    const size_t _i = (i); \
    _a[_i / 8] &= ~(1 << (_i % 8)); \
})

#define HAS_TAG(a, i) \
({ \
    const uint8_t *const _a = TagList.archs[a]; \
    const size_t _i = (i); \
    !!(_a[_i / 8] & (1 << (_i % 8))); \
})

int InitTagSystem(void);
int CacheTags(void);
char *ArchToString(size_t archid);
size_t StringToArch(const char *s);
size_t AddArch(size_t archid, size_t tagid);

struct file {
    struct stat st;
    char *name;
    size_t archid;
};

/*
 * creates/removes symlinks to match the archid
 */
int SetTags(struct file *file);
char *GetFilePath(const char *name);

extern struct file_list {
    pthread_mutex_t lock;
    struct file *files;
    size_t num, cap;
} FileList;

#endif
