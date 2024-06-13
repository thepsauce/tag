#include "macros.h"
#include "tag.h"
#include "scroller.h"
#include "screen.h"
#include "controls.h"

#include <fnmatch.h>
#include <string.h>

struct scroller Scroller;

void RenderScroller(void)
{
    struct text t;

    memset(&t, 0, sizeof(t));
    t.r = (Rect) { 0, 0, COLS / 2, LINES };
    Scroller.height = t.r.h - 2;
    if (Focused == NULL) {
        attr_set(A_NORMAL, CP_FOCUS, NULL);
    } else {
        attr_set(A_NORMAL, CP_NORMAL, NULL);
    }
    DrawBox(&t.r);
    t.r.y++;
    t.r.x++;
    const int w = t.r.w - 2;
    t.r.w = w / 2;
    t.r.h = 1;
    for (size_t i = Scroller.scroll; i < Scroller.scroll + Scroller.height; i++) {
        if (i >= Scroller.num) {
            break;
        }
        struct file *const f = &FileList.files[Scroller.rei[i]];
        if (Scroller.index == i) {
            attr_set(A_REVERSE, 0, NULL);
            t.flags = DT_SEL;
        } else {
            attr_set(A_NORMAL, 0, NULL);
            t.flags = 0;
        }
        t.s = f->name;
        t.len = strlen(f->name);
        t.sel = t.len;
        DrawText(stdscr, &t);
        t.r.x += t.r.w;
        t.r.w = w - t.r.w;
        t.s = ArchToString(f->archid);
        t.len = strlen(t.s);
        t.flags = DT_SLASH;
        DrawText(stdscr, &t);
        t.r.w = w / 2;
        t.r.x -= t.r.w;
        t.r.y++;
    }
    attr_set(A_NORMAL, 0, NULL);
    mvprintw(LINES - 1, 2, "%zu/%zu (%d%%)", Scroller.index + 1, Scroller.num,
            Scroller.num == 0 ? 100 : (int) (100 * (Scroller.index + 1) / Scroller.num));
}

static void SetIndex(size_t index)
{
    Scroller.index = index;
    if (Scroller.num > 0) {
        free(TagEdit.s);
        TagEdit.s = strdup(ArchToString(FileList.files[Scroller.rei[Scroller.index]].archid));
        if (TagEdit.s == NULL) {
            TagEdit.len = 0;
            TagEdit.cap = 0;
        } else {
            TagEdit.len = strlen(TagEdit.s);
            TagEdit.cap = TagEdit.len;
        }
        TagEdit.index = TagEdit.len;
        TagEdit.flags |= DT_ADJVCT;
    }

    if (Scroller.index < Scroller.scroll) {
        Scroller.scroll = Scroller.index;
    } else if (Scroller.index >= Scroller.scroll + Scroller.height) {
        Scroller.scroll = Scroller.index - Scroller.height + 1;
    }
}

int NotifyScroller(void)
{
    if (FileList.num > Scroller.cap) {
        Scroller.cap = FileList.num;
        size_t *const p = Realloc(Scroller.rei,
                sizeof(*Scroller.rei) * Scroller.cap);
        if (p == NULL) {
            return -1;
        }
        Scroller.rei = p;
    }

    char ffilter[FileFilter.len + 3];
    ffilter[0] = '*';
    memcpy(&ffilter[1], FileFilter.s, FileFilter.len);
    ffilter[FileFilter.len + 1] = '*';
    ffilter[FileFilter.len + 2] = '\0';

    const size_t tfilter = TagFilter.s == NULL ? SIZE_MAX : StringToArch(TagFilter.s);

    Scroller.num = 0;
    for (size_t i = 0; i < FileList.num; i++) {
        char *const name = FileList.files[i].name;
        bool tm = false, fm = false;
        if (fnmatch(ffilter, name, FNM_NOESCAPE) == 0) {
            tm = true;
        }
        if (tfilter == SIZE_MAX || CONTAINS_TAGS(FileList.files[i].archid, tfilter)) {
            fm = true;
        }
        if (tm && fm) {
            Scroller.rei[Scroller.num++] = i;
        }
    }

    if (Scroller.num == 0) {
        SetIndex(0);
    } else {
        SetIndex(MIN(Scroller.index, Scroller.num - 1));
    }
    UIDirty = true;
    return 0;
}

bool MoveScroller(size_t dy, int dir)
{
    if (dir < 0) {
        if (dy > Scroller.index) {
            dy = Scroller.index;
        }
        if (dy == 0) {
            return false;
        }
        SetIndex(Scroller.index - dy);
    } else {
        if (dy >= Scroller.num - Scroller.index) {
            dy = Scroller.num - Scroller.index - 1;
        }
        if (dy == 0) {
            return false;
        }
        SetIndex(Scroller.index + dy);
    }
    if (Scroller.index < Scroller.scroll) {
        Scroller.scroll = Scroller.index;
    } else if (Scroller.index >= Scroller.scroll + Scroller.height) {
        Scroller.scroll = Scroller.index - Scroller.height + 1;
    }
    return true;
}
