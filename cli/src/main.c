#include "macros.h"
#include "screen.h"
#include "tag.h"
#include "input.h"
#include "scroller.h"

#include <unistd.h>
#include <string.h>

int RunUI(void)
{
    struct event ev;

    Input.text.r = (Rect) { 3, 4, 40, 5 };

    UIRunning = true;
    while (UIRunning) {
        GetEvent(&ev);
        switch (ev.type) {
        case EV_KEYDOWN:
            if (Input.shown) {
                InputHandle(ev.key);
                break;
            }
            switch (ev.key) {
            case 'f':
                Input.shown = true;
                break;
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
            case '\n': {
                if (Scroller.num == 0) {
                    break;
                }
                struct file *const file = &FileList.files[Scroller.rei[Scroller.index]];
                if (fork() == 0) {
                    execl("/usr/bin/feh", "feh", GetFilePath(file->name), NULL);
                }
                break;
            }
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
    if (!UIRunning) {
        return;
    }
    erase();
    RenderScroller();
    if (Input.shown) {
        RenderInput();
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

