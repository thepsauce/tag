#include "tag.h"
#include "scroller.h"
#include "screen.h"

#include <dirent.h>
#include <glob.h>
#include <stdlib.h>
#include <string.h>
#include <string.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <unistd.h>

const char *Default;

int NotifyFD;
int BaseWD;

struct tag_list TagList;
struct file_list FileList;

static inline int InitAllDirectory(void)
{
    static const char *def = "all";
    Default = getenv("TAG_DEFAULT_DIR");
    if (Default == NULL) {
        Default = def;
    }

    glob_t g;
    char buf[2];
    do {
        if (glob(Default, GLOB_MARK | GLOB_NOSORT, NULL, &g) == 0) {
            globfree(&g);
            return 0;
        }
        chdir("..");
            /* go up until we hit the root which is the only path that
             * can fit in char[2] */
    } while (getcwd(buf, sizeof(buf)) == NULL);

    return -1;
}

static int AddTag(const char *cname)
{
    struct tag tag;

    char *const name = Strdup(cname);
    if (name == NULL) {
        return -1;
    }
    if (TagList.num >= TagList.cap) {
        TagList.cap *= 2;
        TagList.cap++;

        pthread_mutex_lock(&TagList.lock);
        struct tag *const v = Realloc(TagList.tags, sizeof(*TagList.tags) * TagList.cap);
        if (v == NULL) {
            Free(name);
            return -1;
        }
        TagList.tags = v;
        pthread_mutex_unlock(&TagList.lock);
    }
    if (strcmp(name, Default) == 0) {
        TagList.all = TagList.num;
    }
    tag.wd = inotify_add_watch(NotifyFD, name, IN_CREATE | IN_MOVE);
    if (tag.wd == -1) {
        Free(name);
        return -1;
    }
    tag.name = name;
    TagList.tags[TagList.num++] = tag;
    return 0;
}

static int GetTags(struct file *f)
{
    size_t n;
    char *buf;
    size_t cap;

    f->tags = NULL;

    n = strlen(f->name);
    cap = 128 + n;
    buf = Malloc(cap);
    if (buf == NULL) {
        return -1;
    }

    for (size_t i = 0; i < TagList.num; i++) {
        struct tag *const tag = &TagList.tags[i];
        const size_t nt = strlen(tag->name);
        const size_t req = nt + 1 + n + 1;
        if (req > cap) {
            cap = req;
            char *const b = Realloc(buf, cap);
            if (b == NULL) {
                Free(buf);
                return -1;
            }
            buf = b;
        }
        memcpy(buf, tag->name, nt);
        buf[nt] = '/';
        strcpy(&buf[nt + 1], f->name);
        if (access(buf, F_OK) == 0) {
            f->tags = AddComposition(f->tags, i);
            if (f->tags == NULL) {
                Free(buf);
                return -1;
            }
        }
    }
    Free(buf);
    return 0;
}

char *GetFilePath(const char *name)
{
    static char buf[NAME_MAX + 1 + NAME_MAX + 1];
    snprintf(buf, sizeof(buf), "%s/%s", Default, name);
    return buf;
}

static int AddFile(const char *name, int dirfd)
{
    struct file f;
    struct stat st;

    if (dirfd > 0) {
        if (fstatat(dirfd, name, &st, 0) == -1) {
            return -1;
        }
        if (!S_ISREG(st.st_mode)) {
            return 1;
        }
    } else {
        if (stat(GetFilePath(name), &st) == -1) {
            return -1;
        }
    }

    f.name = Strdup(name);
    if (f.name == NULL) {
        return -1;
    }
    f.time = st.st_mtime;
    f.tags = 0;

    if (FileList.num >= FileList.cap) {
        FileList.cap *= 2;
        FileList.cap++;

        pthread_mutex_lock(&FileList.lock);
        struct file *const v = Realloc(FileList.files,
                sizeof(*FileList.files) * FileList.cap);
        if (v == NULL) {
            Free(f.name);
            return -1;
        }
        FileList.files = v;
        pthread_mutex_unlock(&FileList.lock);
    }
    FileList.files[FileList.num] = f;
    if (GetTags(&FileList.files[FileList.num]) < 0) {
        Free(f.name);
        return -1;
    }
    FileList.num++;
    return 0;
}

int CacheFiles(void)
{
    DIR *dir;
    int fd;
    struct dirent *ent;
    size_t c = 0;

    dir = opendir(Default);
    if (dir == NULL) {
        return -1;
    }
    fd = dirfd(dir);

    while ((ent = readdir(dir)) != NULL) {
        if (ent->d_type != DT_REG) {
            continue;
        }
        if (AddFile(ent->d_name, fd) < 0) {
            closedir(dir);
            return -1;
        }
        c++;
        if (c == 64) {
            NotifyScroller();
            c = 0;
        }
    }
    if (c > 0) {
        NotifyScroller();
    }
    return 0;
}

int CacheTags(void)
{
    DIR *dir;
    struct dirent *ent;

    dir = opendir(".");
    if (dir == NULL) {
        return -1;
    }

    TagList.num = 0;
    while ((ent = readdir(dir)) != NULL) {
        if (ent->d_name[0] == '.' || ent->d_type != DT_DIR) {
            continue;
        }
        if (AddTag(ent->d_name) < 0) {
            goto err;
        }
    }
    return 0;

err:
    closedir(dir);
    for (size_t i = 0; i < TagList.num; i++) {
        inotify_rm_watch(NotifyFD, TagList.tags[i].wd);
        Free(TagList.tags[i].name);
    }
    Free(TagList.tags);
    return -1;
}

void *WatchThread(void *unused)
{
    (void) unused;

    char buf[4096];
    ssize_t len;
    struct inotify_event *ie;
    struct stat st;

    while (1) {
        len = read(NotifyFD, buf, sizeof(buf));
        for (char *b = buf; b < buf + len; b += sizeof(*ie) + ie->len) {
            ie = (struct inotify_event*) b;
            if (ie->mask & IN_CREATE) {
                if (TagList.wd == ie->wd) {
                    if (stat(Default, &st) == 0 && S_ISDIR(st.st_mode)) {
                        AddTag(ie->name);
                    }
                } else {
                    if (TagList.tags[TagList.all].wd == ie->wd) {
                        AddFile(ie->name, -1);
                        NotifyScroller();
                    }
                }
            }
            if (ie->mask & IN_MOVED_FROM) {
            }
            if (ie->mask & IN_MOVED_TO) {
            }
        }
    }
    return NULL;
}

void *CacheThread(void *unused)
{
    (void) unused;

    CacheFiles();
    NotifyScroller();
    return NULL;
}

int InitTagSystem(void)
{
    if (InitAllDirectory() < 0) {
        return -1;
    }
    NotifyFD = inotify_init();
    if (NotifyFD < 0) {
        return -1;
    }
    if (CacheTags() < 0) {
        close(NotifyFD);
        return -1;
    }
    TagList.wd = inotify_add_watch(NotifyFD, ".", IN_CREATE | IN_MOVE);
    if (TagList.wd == -1) {
        goto err;
    }
    pthread_t tid;
    if (pthread_create(&tid, 0, WatchThread, NULL) != 0) {
        goto err;
    }
    if (pthread_create(&tid, 0, CacheThread, NULL) != 0) {
        goto err;
    }
    return 0;

err:
    for (size_t i = 0; i < TagList.num; i++) {
        Free(TagList.tags[i].name);
    }
    Free(TagList.tags);
    close(NotifyFD);
    return -1;
}

char *CompToString(uint8_t *comp)
{
    static char buf[1024];
    size_t n = 0;

    const size_t s = COMP_SIZE();
    for (size_t i = 0; i < s; i++) {
        uint8_t c = comp[i];
        size_t id = i * 8;
        while (c) {
            if (c & 0x1) {
                if (n > 0) {
                    buf[n++] = '/';
                }

                const char *const name = TagList.tags[id].name;
                const size_t l = strlen(name);
                memcpy(&buf[n], name, l);
                n += l;
            }
            c >>= 1;
            id++;
        }
    }
    buf[n] = '\0';
    return buf;
}

uint8_t *StringToComp(const char *s)
{
    const size_t cs = COMP_SIZE();
    uint8_t empty[cs];
    uint8_t *comp;
    memset(empty, 0, cs);
    comp = empty;
    while (*s != '\0') {
        const char *e = s;
        while (*e != '/' && *e != '\0') {
            e++;
        }
        const size_t id = TAG_ID_L(s, e - s);
        if (id != TagList.num) {
            comp = AddComposition(comp, id);
        }
        if (*e == '\0') {
            break;
        }
        s = e + 1;
    }
    return comp;
}

uint8_t *AddComposition(uint8_t *prev, size_t tagid)
{
    const size_t s = COMP_SIZE();
    for (size_t i = 0; i < TagList.compn; i++) {
        uint8_t *const comp = TagList.comps[i];

        if (!HAS_TAG(comp, tagid)) {
            continue;
        }

        REMOVE_TAG(comp, tagid);

        bool miss = false;

        for (size_t j = 0; j < s; j++) {
            miss = comp[j] != (prev == NULL ? 0 : prev[j]);
            if (miss) {
                break;
            }
        }

        ADD_TAG(comp, tagid);

        if (!miss) {
            return comp;
        }
    }

    uint8_t **const p = Realloc(TagList.comps, sizeof(*TagList.comps) *
            (TagList.compn + 1));
    if (p == NULL) {
        return NULL;
    }
    TagList.comps = p;

    uint8_t *const comp = Malloc(sizeof(*comp) * s);
    if (comp == NULL) {
        return NULL;
    }
    if (prev == NULL) {
        memset(comp, 0, sizeof(*comp) * s);
    } else {
        memcpy(comp, prev, sizeof(*comp) * s);
    }
    ADD_TAG(comp, tagid);
    TagList.comps[TagList.compn++] = comp;
    return comp;
}

