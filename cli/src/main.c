#include "macros.h"
#include "screen.h"
#include "tag.h"
#include "input.h"
#include "scroller.h"

#include <unistd.h>
#include <string.h>

bool UIRunning;

int RunUI(void)
{
    struct event ev;

    Input.text.r = (Rect) { 3, 4, 40, 5 };

    UIRunning = true;
    while (UIRunning) {
        RenderUI();
        RenderInput();

        GetEvent(&ev);
        switch (ev.type) {
        case EV_KEYDOWN:
            if (ev.key == 'C' - 'A' + 1) {
                UIRunning = false;
                break;
            }
            InputHandle(ev.key);
            break;
            switch (ev.key) {
            case 'k':
                MoveScroller(1, -1);
                break;
            case 'j':
                MoveScroller(1, 1);
                break;
            case 'g':
                MoveScroller(SIZE_MAX, -1);
                break;
            case 'G':
                MoveScroller(SIZE_MAX, 1);
                break;
            case 'q':
                UIRunning = false;
                break;
            }
            break;
        default:
        }
    }
    return 0;
}

void RenderUI(void)
{
    Rect r;

    if (!UIRunning) {
        return;
    }
    erase();
    GetDialogRect(&r);
    Scroller.height = r.h - 2;
    attr_set(A_NORMAL, 0, NULL);
    DrawBox(NULL, &r);
    r.y++;
    r.x++;
    const int w = r.w - 2;
    r.w = w / 2;
    r.h = 1;
    for (size_t i = Scroller.scroll; i < Scroller.scroll + Scroller.height; i++) {
        struct file *const f = &FileList.files[Scroller.rei[i]];
        if (Scroller.index == i) {
            attr_set(A_REVERSE, 0, NULL);
        } else {
            attr_set(A_NORMAL, 0, NULL);
        }
        DrawString(&r, 0, f->name);
        r.x += r.w;
        r.w = w - r.w;
        DrawString(&r, 0, CompToString(f->tags));
        r.w = w / 2;
        r.x -= r.w;
        r.y++;
    }
    mvprintw(0, 0, "%zu", TagList.num);
}

int main(void)
{
    InitScreen();

    chdir("/home/steves/ext");
    if (InitTagSystem() != 0) {
        EndScreen();
        return -1;
    }

    RunUI();

    EndScreen();
    return 0;
}

