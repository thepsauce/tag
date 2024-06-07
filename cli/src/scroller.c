#include "tag.h"
#include "scroller.h"
#include "screen.h"

#include <fnmatch.h>
#include <string.h>

struct scroller Scroller;

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
