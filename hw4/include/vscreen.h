#ifndef VSCREEN_H
#define VSCREEN_H

/*
 * Data structure maintaining information about a virtual screen.
 */

#include <ncurses.h>
#include <signal.h>

#include "debug.h"

typedef struct vscreen VSCREEN;

extern WINDOW *main_screen;
extern WINDOW* status_screen;



extern int status_bar_rows;

VSCREEN *vscreen_init(void);
void vscreen_show(VSCREEN *vscreen);
void vscreen_sync(VSCREEN *vscreen);
void vscreen_putc(VSCREEN *vscreen, char c);
void vscreen_fini(VSCREEN *vscreen);
void set_status(char* status);
void init_windows();
void set_all_lines_changed(VSCREEN* vscreen);
int get_cur_line(VSCREEN* vscreen);
int get_cur_col(VSCREEN* vscreen);
void update_time();


struct vscreen {
    int num_lines;
    int num_cols;
    int cur_line;
    int cur_col;
    char **lines;
    char *line_changed;
};



#endif
