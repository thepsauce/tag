#include "macros.h"
#include "screen.h"
#include "tag.h"
#include "controls.h"
#include "scroller.h"
#include "log.h"

#include <string.h>
#include <unistd.h>

int RunUI(void)
{
    volatile size_t errors = 0;
    struct event ev;

    InitControls();

    if (setjmp(UIJumpBuffer) > 0) {
        errors++;
    }

    UIRunning = true;
    while (UIRunning) {
        GetEvent(&ev);
        ControlsHandle(&ev);
    }
    Log("Exiting UI, counted %zu errors", errors);
    return 0;
}

void RenderUI(void)
{
    if (!UIRunning) {
        return;
    }
    erase();
    RenderScroller();
    RenderControls();

    Rect r = { COLS * 3 / 4, 3, COLS / 4 - 4, LINES - 4 };
    attr_set(A_NORMAL, CP_NORMAL, NULL);
    DrawBox(&r);
    r.x++;
    r.y++;
    r.w -= 2;
    r.h -= 2;
    int y, x;
    getyx(LogWindow, y, x);
    (void) x;
    copywin(LogWindow, stdscr, y - r.h, 0, r.y, r.x, r.y + r.h - 1, r.x + r.w - 1, 0);
}

int main(void)
{
    InitScreen();

    if (InitLogSystem() != 0) {
        EndScreen();
        fprintf(stderr, "Failed initializing log system\n");
        return -1;
    }

    Log("Started logging...");

    chdir("/home/steves/ext"); /* TODO: FOR TESTING */
    if (InitTagSystem() != 0) {
        Log("Failed initializing tag system");
        EndScreen();
        return -1;
    }

    RunUI();

    Log("Normal exit");

    EndScreen();

    Log("Bye!");
    return 0;
}

