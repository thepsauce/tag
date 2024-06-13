#include "tag.h"
#include "scroller.h"
#include "screen.h"

#include <dirent.h>
#include <glob.h>
#include <stdlib.h>
#include <string.h>
#include <string.h>
#include <sys/inotify.h>
#include <unistd.h>

const char *Default;

int NotifyFD;

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

    for (size_t i = 0; i < TagList.num; i++) {
        if (strcmp(TagList.tags[i].name, cname) == 0) {
            return 0;
        }
    }

    char *const name = Strdup(cname);
    if (name == NULL) {
        return -1;
    }
    if (TagList.num >= TagList.cap) {
        TagList.cap *= 2;
        TagList.cap++;

        if ((TagList.num + 1) % 8 == 0) {
            const size_t as = TagList.num / 8 + 2;
            for (size_t id = 0; id < TagList.archn; id++) {
                uint8_t *const a = Realloc(TagList.archs[id],
                        sizeof(*a) * as);
                if (a == NULL) {
                    return -1;
                }
                TagList.archs[id] = a;
                TagList.archs[id][as - 1] = 0;
            }
        }

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
    tag.wd = inotify_add_watch(NotifyFD, name, IN_CREATE | IN_DELETE | IN_MOVE | IN_ATTRIB);
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

    f->archid = SIZE_MAX;

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
            f->archid = AddArch(f->archid, i);
            if (f->archid == SIZE_MAX) {
                Free(buf);
                return -1;
            }
        }
    }
    Free(buf);
    return 0;
}

int SetTags(struct file *f)
{
    char *buf;
    size_t cap;

    const size_t n = strlen(f->name);

    cap = 128 + n;
    buf = Malloc(cap);
    if (buf == NULL) {
        return -1;
    }

    for (size_t id = 0; id < TagList.num; id++) {
        if (id == TagList.all) {
            continue;
        }
        struct tag *const tag = &TagList.tags[id];
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
        sprintf(buf, "%s/%s", tag->name, f->name);
        if (access(buf, F_OK) == 0) {
            if (!HAS_TAG(f->archid, id)) {
                unlink(buf);
            }
        } else {
            if (HAS_TAG(f->archid, id)) {
                char buf2[3 + strlen(Default) + 1 + n + 1];
                sprintf(buf2, "../%s/%s", Default, f->name);
                symlink(buf2, buf);
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

static int CacheFile(const char *name, int dirfd)
{
    struct file f;
    bool e = true;

    if (dirfd > 0) {
        if (fstatat(dirfd, name, &f.st, 0) == -1) {
            e = false;
        }
    } else {
        if (stat(GetFilePath(name), &f.st) == -1) {
            e = false;
        }
    }
    if (!S_ISREG(f.st.st_mode)) {
        e = false;
    }

    for (size_t i = 0; i < FileList.num; i++) {
        if (strcmp(FileList.files[i].name, name) == 0) {
            if (!e) {
                pthread_mutex_lock(&FileList.lock);
                FileList.num--;
                memmove(&FileList.files[i], &FileList.files[i + 1],
                        sizeof(*FileList.files) * (FileList.num - i));
                pthread_mutex_unlock(&FileList.lock);
                return 0;
            }
            FileList.files[i].st = f.st;
            if (GetTags(&FileList.files[i]) < 0) {
                return -1;
            }
            return 0;
        }
    }

    if (!e) {
        return -1;
    }

    f.name = Strdup(name);
    if (f.name == NULL) {
        return -1;
    }
    f.archid = SIZE_MAX;

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
        if (CacheFile(ent->d_name, fd) < 0) {
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
                    CacheFile(ie->name, -1);
                    NotifyScroller();
                }
            }
            if (ie->mask & IN_DELETE) {
                if (TagList.wd == ie->wd) {
                    const size_t tagid = TAG_ID(ie->name);
                    if (tagid == SIZE_MAX) {
                        continue;
                    }
                    for (size_t id = 0; id < TagList.archn; id++) {
                        REMOVE_TAG(id, tagid);
                    }
                } else {
                    CacheFile(ie->name, -1);
                }
                NotifyScroller();
            }
            if (ie->mask & (IN_ATTRIB | IN_MOVED_FROM | IN_MOVED_TO)) {
                CacheFile(ie->name, -1);
                NotifyScroller();
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
    TagList.wd = inotify_add_watch(NotifyFD, ".",
            IN_CREATE | IN_DELETE | IN_MOVE | IN_ATTRIB);
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

char *ArchToString(size_t archid)
{
    static char buf[1024];
    size_t n = 0;

    const size_t s = ARCH_SIZE();
    for (size_t i = 0; i < s; i++) {
        uint8_t c = TagList.archs[archid][i];
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

size_t StringToArch(const char *s)
{
    size_t archid = SIZE_MAX;
    while (*s != '\0') {
        const char *e = s;
        while (*e != '/' && *e != '\0') {
            e++;
        }
        size_t tagid = TAG_ID_L(s, e - s);
        if (tagid == TagList.num) {
            char name[e - s + 1];
            memcpy(name, s, e - s + 1);
            name[e - s] = '\0';
            if (mkdir(name, 0755) == -1) {
                return SIZE_MAX;
            }
            if (AddTag(name) == -1) {
                return SIZE_MAX;
            }
        }
        archid = AddArch(archid, tagid);
        if (*e == '\0') {
            break;
        }
        s = e + 1;
    }
    return archid;
}

size_t AddArch(size_t archid, size_t tagid)
{
    const size_t s = ARCH_SIZE();
    for (size_t id = 0; id < TagList.archn; id++) {
        if (!HAS_TAG(id, tagid)) {
            continue;
        }

        REMOVE_TAG(id, tagid);

        bool miss = false;

        for (size_t j = 0; j < s; j++) {
            miss = TagList.archs[id][j] !=
                (archid == SIZE_MAX ? 0 :
                TagList.archs[archid][j]);
            if (miss) {
                break;
            }
        }

        ADD_TAG(id, tagid);

        if (!miss) {
            return id;
        }
    }

    uint8_t **const p = Realloc(TagList.archs,
            sizeof(*TagList.archs) * (TagList.archn + 1));
    if (p == NULL) {
        return SIZE_MAX;
    }
    TagList.archs = p;

    uint8_t *const arch = Malloc(sizeof(*arch) * s);
    if (arch == NULL) {
        return SIZE_MAX;
    }
    if (archid == SIZE_MAX) {
        memset(arch, 0, sizeof(*arch) * s);
    } else {
        memcpy(arch, TagList.archs[archid], sizeof(*arch) * s);
    }
    archid = TagList.archn++;
    TagList.archs[archid] = arch;
    ADD_TAG(archid, tagid);
    return archid;
}

