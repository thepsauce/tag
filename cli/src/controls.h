#ifndef CONTROLS_H
#define CONTROLS_H

#include "gfx.h"

extern struct text TagFilter;
extern struct text FileFilter;
extern struct text Search;
extern struct text TagEdit;

extern const char *Titles[];
extern struct text *Inputs[];
extern int Binds[];

extern struct text *Focused;

void InitControls(void);
void ControlsHandle(struct event *ev);
void RenderControls(void);

#endif
