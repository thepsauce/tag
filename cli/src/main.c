#include "macros.h"
#include "screen.h"
#include "tag.h"
#include "controls.h"
#include "scroller.h"

#include <string.h>
#include <unistd.h>

int RunUI(void)
{
    struct event ev;

    InitControls();

    UIRunning = true;
    while (UIRunning) {
        GetEvent(&ev);
        ControlsHandle(&ev);
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
    RenderControls();
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

