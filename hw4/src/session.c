/*
 * Virtual screen sessions.
 *
 * A session has a pseudoterminal (pty), a virtual screen,
 * and a process that is the session leader.  Output from the
 * pty goes to the virtual screen, which can be one of several
 * virtual screens multiplexed onto the physical screen.
 * At any given time there is a particular session that is
 * designated the "foreground" session.  The contents of the
 * virtual screen associated with the foreground session is what
 * is shown on the physical screen.  Input from the physical
 * keyboard is directed to the pty for the foreground session.
 */

#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include <ctype.h>
#include <time.h>
#include <string.h>


#include "session.h"

static void child_handler(int sig);
static void alarm_handler(int sig);



volatile sig_atomic_t killed = 0;

SESSION *sessions[MAX_SESSIONS];  // Table of existing sessions
SESSION *fg_session;              // Current foreground session
SESSION* terminated = NULL;

/*
 * Initialize a new session whose session leader runs a specified command.
 * If the command is NULL, then the session leader runs a shell.
 * The new session becomes the foreground session.
 */
SESSION *session_init(char *path, char *argv[]) {
    for(int i = 0; i < MAX_SESSIONS; i++) {
	if(sessions[i] == NULL) {
	    int mfd = posix_openpt(O_RDWR | O_NOCTTY);
	    if(mfd == -1)
		return NULL; // No more ptys
	    int uptsuccess = unlockpt(mfd);
        if(uptsuccess == -1){
            set_status("Couldn't unlock pt");
            return NULL;
        }
	    char *sname = ptsname(mfd);
        if(sname == NULL){
            set_status("Couldn't find pts name");
            return NULL;
        }
	    // Set nonblocking I/O on master side of pty
	    int fcntl_success = fcntl(mfd, F_SETFL, O_NONBLOCK);
        if(fcntl_success == -1){
            set_status("Couldn't set flags");
            return NULL;
        }
	    SESSION *session = calloc(sizeof(SESSION), 1);
	    sessions[i] = session;
	    session->sid = i;
	    session->vscreen = vscreen_init();
	    session->ptyfd = mfd;

        signal(SIGCHLD, child_handler);
        signal(SIGALRM, alarm_handler);


	    // Fork process to be leader of new session.
	    if((session->pid = fork()) == 0) {
		// Open slave side of pty, create new session,
		// and set pty as controlling terminal.
		int sfd = open(sname, O_RDWR);
		setsid();
		ioctl(sfd, TIOCSCTTY, 0);
        /*
        if(has_stderr_file){
            //char* filename = optarg;
            //int fd = open(stderr_file, O_RDWR | O_CREAT | O_TRUNC, 0666);
            dup2(stderr_file, 2);
            fprintf(stderr, "hello\n");
            }
        else{

		  dup2(sfd, 2);
        }
        */
        //close(sfd);
        dup2(sfd, 2);
        close(mfd);



		// Set TERM environment variable to match vscreen terminal
		// emulation capabilities (which currently aren't that great).
		putenv("TERM=dumb");

		// Set up stdin/stdout and do exec.
		// TO BE FILLED IN

        dup2(sfd, 0);
        dup2(sfd, 1);
        execvp(path, argv);
        close(stderr_file);
        close(sfd);

		fprintf(stderr, "EXEC FAILED (did you fill in this part?)\n");
		exit(1);
	    }
	    // Parent drops through
        //close(mfd);
	    session_setfg(session);
	    return session;
	}
    }
    return NULL;  // Session table full.
}

/*
 * Set a specified session as the foreground session.
 * The current contents of the virtual screen of the new foreground session
 * are displayed in the physical screen, and subsequent input from the
 * physical terminal is directed at the new foreground session.
 */
void session_setfg(SESSION *session) {
    fg_session = session;
    // REST TO BE FILLED IN
    if(session != NULL){
        set_all_lines_changed(session->vscreen);
        vscreen_sync(session->vscreen);
    }
}

/*
 * Read up to bufsize bytes of available output from the session pty.
 * Returns the number of bytes read, or EOF on error.
 */
int session_read(SESSION *session, char *buf, int bufsize) {
    return read(session->ptyfd, buf, bufsize);
}

/*
 * Write a single byte to the session pty, which will treat it as if
 * typed on the terminal.  The number of bytes written is returned,
 * or EOF in case of an error.
 */
int session_putc(SESSION *session, char c) {
    // TODO: Probably should use non-blocking I/O to avoid the potential
    // for hanging here, but this is ignored for now.
    return write(session->ptyfd, &c, 1);
}

/*
 * Forcibly terminate a session by sending SIGKILL to its process group.
 */
void session_kill(SESSION *session) {
    // TO BE FILLED IN
    kill(session->pid, SIGKILL);
    terminated = session;
}

/*
 * Deallocate a session that has terminated.  This function does not
 * actually deal with marking a session as terminated; that would be done
 * by a signal handler.  Note that when a session does terminate,
 * the session leader must be reaped.  In addition, if the terminating
 * session was the foreground session, then the foreground session must
 * be set to some other session, or to NULL if there is none.
 */
void session_fini(SESSION *session) {
    // TO BE FILLED IN
    vscreen_fini(session->vscreen);
    int found_other_session = 0;
    for(int i = 0; i < MAX_SESSIONS; i++){
        if(sessions[i] == session){
            close(sessions[i]->ptyfd);
            //close(0);
            sessions[i] = NULL;
            break;
        }
    }
    if(fg_session == session){
        for(int i = 0; i < MAX_SESSIONS; i++){
            if(sessions[i] != NULL){
                found_other_session = 1;
                session_setfg(sessions[i]);
                break;
            }
        }
        if(!found_other_session)
            session_setfg(NULL);
    }

    free(session);

}

void child_handler(int sig){
    killed = 1;
}

void update_time(){
    time_t cur_time;
    time(&cur_time);
    int tot_seconds = (int) cur_time;
    tot_seconds -= 4*3600; //account for gmt time
    int hour = (tot_seconds / 3600) % 12;
    int minute = (tot_seconds / 60) % 60;
    int second = tot_seconds % 60;
    int str_length = 11;
    char formatted[str_length];
    if(second >= 10){
        if(minute >= 10){
            sprintf(formatted, "%d:%d:%d %d", hour, minute, second, get_num_sessions());
        }
        else{
            sprintf(formatted, "%d:0%d:%d %d", hour, minute, second, get_num_sessions());
        }
    }
    else{
        if(minute >= 10){
            sprintf(formatted, "%d:%d:0%d %d", hour, minute, second, get_num_sessions());

        }
        else{
            sprintf(formatted, "%d:0%d:0%d %d", hour, minute, second, get_num_sessions());

        }
    }
    //set_status(formatted);
    wmove(status_screen, 0, COLS - 1 - str_length);
    wrefresh(status_screen);
    if(get_num_sessions() < 10){
        for(int i = 0; i < str_length - 1; i++){
            char ch = formatted[i];
            if(isprint(ch)){
                waddch(status_screen, ch);
            }
        }
    }
    else{
        for(int i = 0; i < str_length; i++){
            char ch = formatted[i];
            if(isprint(ch)){
                waddch(status_screen, ch);
            }

        }
    }
    wmove(main_screen, get_cur_line(fg_session->vscreen), get_cur_col(fg_session->vscreen));
    wrefresh(main_screen);
    alarm(1);

}

static void alarm_handler(int sig){
    update_time();

}

void display_help_screen(){
    VSCREEN* help_screen = vscreen_init();
    //debug("wassup");
    char* screen[21];

    screen[0] = "HELP SCREEN";
    screen[1] = "ECRAN";
    screen[2] = "Press ESC to return to program";
    screen[3] = "Type CTRL -a to activate commands";
    screen[4] = "Commands:";
    screen[5] = "q: quit";
    screen[6] = "n: new session";
    screen[7] = "a 0, a 1 ... a 9: switch sessions";
    screen[8] = "k 0, k 1 ... k 9: kill session";
    screen[9] = "h: help screen";
    screen[10] = "SESSIONS:";
    screen[11] = sessions[0] == NULL? "SESSION 1 NOT USED" : "SESSION 1 IN USE";
    screen[12] = sessions[1] == NULL? "SESSION 2 NOT USED" : "SESSION 2 IN USE";
    screen[13] = sessions[2] == NULL? "SESSION 3 NOT USED" : "SESSION 3 IN USE";
    screen[14] = sessions[3] == NULL? "SESSION 4 NOT USED" : "SESSION 4 IN USE";
    screen[15] = sessions[4] == NULL? "SESSION 5 NOT USED" : "SESSION 5 IN USE";
    screen[16] = sessions[5] == NULL? "SESSION 6 NOT USED" : "SESSION 6 IN USE";
    screen[17] = sessions[6] == NULL? "SESSION 7 NOT USED" : "SESSION 7 IN USE";
    screen[18] = sessions[7] == NULL? "SESSION 8 NOT USED" : "SESSION 8 IN USE";
    screen[19] = sessions[8] == NULL? "SESSION 9 NOT USED" : "SESSION 9 IN USE";
    screen[20] = sessions[9] == NULL? "SESSION 10 NOT USED" : "SESSION 10 IN USE";
    /*
    for(int i = 0; i < MAX_SESSIONS; i++){
        if(sessions[i] == NULL){
            char output[19];
            sprintf(output, "SESSION %d: NOT USED", i);
            //screen[i + 11] = output;
            debug("session is null");
            memcpy(screen[i + 11], output, strlen(output));
        }
        else{
            char output[17];
            sprintf(output, "SESSION %d: IN USE", i);
            //screen[i + 11] = output;
            debug("session is not null");

            memcpy(screen[i + 11], output, strlen(output));
            debug("successful memcpy");
        }
    }*/
    //set_all_lines_changed(help_screen);
    //vscreen_sync(help_screen);
    /*
    for(int i = 0; i < help_screen->num_lines; i++){
        for(int j = 0; j < help_screen->num_cols; j++){
            if(help_screen->lines[i][j] == '\0'){
                break;
            }
            vscreen_putc(help_screen, help_screen->lines[i][j]);
        }
    }*/
    for(int i = 0; i < 21; i++){
        memcpy(help_screen->lines[i], screen[i], strlen(screen[i]));
    }/*
    for(int i = 0; i < 20; i++){
        update_line(help_screen, i);
    }*/
    set_all_lines_changed(help_screen);
    vscreen_sync(help_screen);

    while(wgetch(main_screen) != 27){
    }


    vscreen_fini(help_screen);

}
