#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>

#include "vscreen.h"

/*
 * Functions to implement a virtual screen that can be multiplexed
 * onto a physical screen.  The current contents of a virtual screen
 * are maintained in an in-memory buffer, from which they can be
 * used to initialize the physical screen to correspond.
 */

WINDOW *main_screen;
WINDOW* status_screen;
int status_bar_rows = 1;


void init_status_bar();


static void update_line(VSCREEN *vscreen, int l);

/*
 * Create a new virtual screen of the same size as the physical screen.
 */
VSCREEN *vscreen_init() {
    VSCREEN *vscreen = calloc(sizeof(VSCREEN), 1);
    vscreen->num_lines = LINES - status_bar_rows;
    vscreen->num_cols = COLS;
    vscreen->cur_line = 0;
    vscreen->cur_col = 0;
    vscreen->lines = calloc(sizeof(char *), vscreen->num_lines);
    vscreen->line_changed = calloc(sizeof(char), vscreen->num_lines);
    for(int i = 0; i < vscreen->num_lines; i++)
	vscreen->lines[i] = calloc(sizeof(char), vscreen->num_cols);
    return vscreen;
}

/*
 * Erase the physical screen and show the current contents of a
 * specified virtual screen.
 */
void vscreen_show(VSCREEN *vscreen) {
    wclear(main_screen);
    for(int l = 0; l < vscreen->num_lines; l++) {
	if(vscreen->line_changed[l]) {
	    update_line(vscreen, l);
	    vscreen->line_changed[l] = 0;
	}
    }
    wmove(main_screen, vscreen->cur_line, vscreen->cur_col);
    wrefresh(main_screen);
}

/*
 * Function to be called after a series of state changes,
 * to cause changed lines on the physical screen to be refreshed
 * and the cursor position to be updated.
 * Although the same effect could be achieved by calling vscreen_show(),
 * the present function tries to be more economical about what is displayed,
 * by only rewriting the contents of lines that have changed.
 */
void vscreen_sync(VSCREEN *vscreen) {
    for(int l = 0; l < vscreen->num_lines; l++) {
	if(vscreen->line_changed[l]) {
	    update_line(vscreen, l);
	    vscreen->line_changed[l] = 0;
	}
    }
    wmove(main_screen, vscreen->cur_line, vscreen->cur_col);
    wrefresh(main_screen);
}

/*
 * Helper function to clear and rewrite a specified line of the screen.
 */
static void update_line(VSCREEN *vscreen, int l) {
    char *line = vscreen->lines[l];
    wmove(main_screen, l, 0);
    wclrtoeol(main_screen);
    for(int c = 0; c < vscreen->num_cols; c++) {
	char ch = line[c];
	if(isprint(ch))
	    waddch(main_screen, line[c]);
    }
    wmove(main_screen, vscreen->cur_line, vscreen->cur_col);
    wrefresh(main_screen);
}

/*
 * Output a character to a virtual screen, updating the cursor position
 * accordingly.  Changes are not reflected to the physical screen until
 * vscreen_show() or vscreen_sync() is called.  The current version of
 * this function emulates a "very dumb" terminal.  Each printing character
 * output is placed at the cursor position and the cursor position is
 * advanced by one column.  If the cursor advances beyond the last column,
 * it is reset to the first column.  The only non-printing characters
 * handled are carriage return, which causes the cursor to return to the
 * beginning of the current line, and line feed, which causes the cursor
 * to advance to the next line and clear from the current column position
 * to the end of the line.  There is currently no scrolling: if the cursor
 * advances beyond the last line, it wraps to the first line.
 */
void vscreen_putc(VSCREEN *vscreen, char ch) {
    int l = vscreen->cur_line;
    int c = vscreen->cur_col;
    if(isprint(ch)) {
	vscreen->lines[l][c] = ch;
	if(vscreen->cur_col + 1 < vscreen->num_cols)
	    vscreen->cur_col++;
    } else if(ch == '\n') {
        if((l = vscreen->cur_line) == vscreen->num_lines - 1){
            for(int i = 0; i < vscreen->num_lines - 1; i++){
                memcpy(vscreen->lines[i], vscreen->lines[i + 1], vscreen->num_cols);
                update_line(vscreen, i);
                //vscreen->lines[i] = vscreen->lines[i + 1];
            }

            memset(vscreen->lines[vscreen->num_lines - 1], 0, vscreen->num_cols);
            //vscreen->lines[vscreen->num_lines - 1] = calloc(vscreen->num_cols, sizeof(char));
            l = vscreen->cur_line = vscreen->num_lines - 1;
            //vscreen_sync(vscreen);
        }
        else{
            l = vscreen->cur_line = (vscreen->cur_line + 1) % vscreen->num_lines;
            memset(vscreen->lines[l], 0, vscreen->num_cols);
        }

	       } else if(ch == '\r') {
	vscreen->cur_col = 0;
    } else if(ch == '\a'){
        flash();
    }

    vscreen->line_changed[l] = 1;
}

void set_status(char* status){
    werase(status_screen);
    for(int c = 0; c < COLS && status[c]; c++) {
        char ch = status[c];
    //if(ch == 0)
      //  break;
        if(isprint(ch))
            waddch(status_screen, ch);
        }
    //kill(getppid(), SIGALRM);
    update_time();
    wrefresh(status_screen);
}

/*
 * Deallocate a virtual screen that is no longer in use.
 */
void vscreen_fini(VSCREEN *vscreen) {
    // TO BE FILLED IN
    for(int i = 0; i < vscreen->num_lines; i++)
        free(vscreen->lines[i]);

    free(vscreen->line_changed);
    free(vscreen->lines);
    free(vscreen);
}

void init_windows(){
    status_screen = newwin(status_bar_rows, COLS, LINES - status_bar_rows, 0);
    main_screen = newwin(LINES - status_bar_rows, COLS, 0, 0);

}

void set_all_lines_changed(VSCREEN* vscreen){
    for(int i = 0; i < vscreen->num_lines; i++){
        vscreen->line_changed[i] = 1;
    }
}

int get_cur_line(VSCREEN* vscreen){
    return vscreen->cur_line;
}

int get_cur_col(VSCREEN* vscreen){
    return vscreen->cur_col;
}

