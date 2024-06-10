#ifndef TAG_H
#define TAG_H

#include <stddef.h>
#include <stdint.h>
#include <time.h>
#include <pthread.h>

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
    uint8_t **comps;
    size_t compn;
} TagList;

#define TAG_ID(name) \
({ \
    const char *const _n = (name); \
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

#define COMP_SIZE() ((TagList.num + TagList.num % 8) / 8)

#define COMP_CONTAINS(c, co) \
({ \
    const uint8_t *_c = (c); \
    const uint8_t *_o = (co); \
    const size_t _s = COMP_SIZE(); \
    size_t _i; \
    for (_i = 0; _i < _s; _i++) { \
        if ((_c[_i] & _o[_i]) != _o[_i]) { \
            break; \
        } \
    } \
    _i == _s; \
})

#define ADD_TAG(c, i) \
({ \
    uint8_t *const _c = (c); \
    const size_t _i = (i); \
    _c[_i / 8] |= 1 << (_i % 8); \
})

#define REMOVE_TAG(c, i) \
({ \
    uint8_t *const _c = (c); \
    const size_t _i = (i); \
    _c[_i / 8] &= ~(1 << (_i % 8)); \
})

#define HAS_TAG(c, i) \
({ \
    const uint8_t *const _c = (c); \
    const size_t _i = (i); \
    !!(_c[_i / 8] & (1 << (_i % 8))); \
})

int InitTagSystem(void);
int CacheTags(void);
char *CompToString(uint8_t *comp);
uint8_t *StringToComp(const char *s);
uint8_t *AddComposition(uint8_t *prev, size_t tagid);

struct file {
    time_t time;
    char *name;
    uint8_t *tags;
};

char *GetFilePath(const char *name);

extern struct file_list {
    pthread_mutex_t lock;
    struct file *files;
    size_t num, cap;
} FileList;

#endif
