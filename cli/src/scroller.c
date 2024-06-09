#include "tag.h"
#include "scroller.h"
#include "screen.h"

#include <fnmatch.h>
#include <string.h>

struct scroller Scroller;

void RenderScroller(void)
{
    struct text t;

    memset(&t, 0, sizeof(t));
    GetDialogRect(&t.r);
    Scroller.height = t.r.h - 2;
    attr_set(A_NORMAL, 0, NULL);
    DrawBox(NULL, &t.r);
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
        }
        t.s = f->name;
        t.len = strlen(t.s);
        t.sel = t.len;
        DrawText(stdscr, &t);
        t.r.x += t.r.w;
        t.r.w = w - t.r.w;
        t.s = CompToString(f->tags);
        t.len = strlen(t.s);
        t.flags = 0;
        DrawText(stdscr, &t);
        t.r.w = w / 2;
        t.r.x -= t.r.w;
        t.r.y++;
    }
    mvprintw(LINES - 1, 0, "%zu", Scroller.num);
}

int NotifyScroller(void)
{
    if (FileList.num > Scroller.cap) {
        Scroller.cap = FileList.num;
        size_t *const v = Realloc(Scroller.rei,
                sizeof(*Scroller.rei) * Scroller.cap);
        if (v == NULL) {
            return -1;
        }
        Scroller.rei = v;
    }
    Scroller.filter = strdup("*a*");
    if (Scroller.filter != NULL) {
        Scroller.num = 0;
        for (size_t i = 0; i < FileList.num; i++) {
            char *const name = FileList.files[i].name;
            if (fnmatch(Scroller.filter, name, FNM_NOESCAPE) == 0) {
                Scroller.rei[Scroller.num++] = i;
            }
        }
    } else {
        for (size_t i = 0; i < FileList.num; i++) {
            Scroller.rei[i] = i;
        }
        Scroller.num = FileList.num;
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
        Scroller.index -= dy;
    } else {
        if (dy >= Scroller.num - Scroller.index) {
            dy = Scroller.num - Scroller.index - 1;
        }
        if (dy == 0) {
            return false;
        }
        Scroller.index += dy;
    }
    if (Scroller.index < Scroller.scroll) {
        Scroller.scroll = Scroller.index;
    } else if (Scroller.index >= Scroller.scroll + Scroller.height) {
        Scroller.scroll = Scroller.index - Scroller.height + 1;
    }
    return true;
}
