/*
 * Ecran: A program that (if properly completed) supports the multiplexing
 * of multiple virtual terminal sessions onto a single physical terminal.
 */

#include <unistd.h>
#include <stdlib.h>
#include <ncurses.h>
#include <sys/signal.h>
#include <sys/select.h>

#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <ctype.h>

#include <fcntl.h>
#include <stdio.h>



#include "ecran.h"

static void initialize();
static void curses_init(void);
static void curses_fini(void);
static void finalize(void);
static int switch_terms(char ch);
static int send_kill_signal(int termSession);
//int get_num_sessions();


static void initialize_with_program(char* path, int argc, char* argv[]);

int has_stderr_file = 0;
int stderr_file;
int num_sessions = 1;
int displaying_help_screen = 0;

int main(int argc, char *argv[]) {
    char option;



    //check for -o option
    while((option = getopt(argc, argv, "+o:")) != -1){
        switch(option){
            case 'o':;
                char* filename = optarg;
                stderr_file = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0777);
                //dup2(fd, 2);
                //fprintf(stderr, "hello\n");
                //close(fd);
                has_stderr_file = 1;
                //stderr_file = optarg;
        }
    }
    if(optind == argc){
        initialize();
    }
    else{
        char* new_argv[1 + (argc - optind)];
        for(int i = optind, j = 0; i < argc; i++, j++){
            new_argv[j] = argv[i];
        }
        initialize_with_program(new_argv[0], argc - optind, new_argv);
        //initialize_with_program()
    }
    if(has_stderr_file){
        //char* filename = optarg;
        //int fd = open(stderr_file, O_RDWR | O_CREAT | O_TRUNC, 0666);
        dup2(stderr_file, 2);
        //fprintf(stderr, "hello\n");
    }

    update_time();
    mainloop();
    // NOT REACHED
}

/*
 * Initialize the program and launch a single session to run the
 * default shell.
 */
static void initialize() {
    curses_init();
    char *path = getenv("SHELL");
    if(path == NULL)
	   path = "/bin/bash";
    char *argv[2] = { " (ecran session)", NULL };

    session_init(path, argv);
}

/*
 * Initialize the program and launch a single session to run the
 * specified program
 */
static void initialize_with_program(char* path, int argc, char* argv[]) {
    //curses_init();
    //char* cmd_path = getenv(path);
    //set_status(cmd_path);
    if(path == NULL){
        //set_status("cmdpath null");
        initialize();
        //curses_init();
        //set_status(cmd_path);
    }

    //char *path = getenv("SHELL");
    //if(path == NULL)
       //path = "/bin/bash";
    //char *argv[2] = { " (ecran session)", NULL };
    else{
        curses_init();
        argv[argc] = NULL;

        session_init(path, argv);
    }

}



/*
 * Cleanly terminate the program.  All existing sessions should be killed,
 * all pty file descriptors should be closed, all memory should be deallocated,
 * and the original screen contents should be restored before terminating
 * normally.  Note that the current implementation only handles restoring
 * the original screen contents and terminating normally; the rest is left
 * to be done.
 */
static void finalize(void) {
    // REST TO BE FILLED IN
    for(int i = 0; i < MAX_SESSIONS; i++){
        if(sessions[i] != NULL){
            //close(sessions[i]->ptyfd);
            session_kill(sessions[i]);
            do_other_processing();
        }
    }
    if(has_stderr_file){
        close(stderr_file);
    }

    curses_fini();
    exit(EXIT_SUCCESS);
}

/*
 * Helper method to initialize the screen for use with curses processing.
 * You can find documentation of the "ncurses" package at:
 * https://invisible-island.net/ncurses/man/ncurses.3x.html
 */
static void curses_init(void) {
    initscr();
    raw();                       // Don't generate signals, and make typein
                                 // immediately available.

    noecho();                    // Don't echo -- let the pty handle it.
    main_screen = stdscr;

    init_windows();


    nodelay(main_screen, TRUE);  // Set non-blocking I/O on input.
    wclear(main_screen);         // Clear the screen.
    wrefresh(main_screen);                   // Make changes visible.

    nodelay(status_screen, TRUE);  // Set non-blocking I/O on input.
    wclear(status_screen);         // Clear the screen.
    wrefresh(status_screen);                   // Make changes visible.

}

/*
 * Helper method to finalize curses processing and restore the original
 * screen contents.
 */
void curses_fini(void) {
    endwin();
}

/*
 * Function to read and process a command from the terminal.
 * This function is called from mainloop(), which arranges for non-blocking
 * I/O to be disabled before calling and restored upon return.
 * So within this function terminal input can be collected one character
 * at a time by calling getch(), which will block until input is available.
 * Note that while blocked in getch(), no updates to virtual screens can
 * occur.
 */
void do_command() {
    // Quit command: terminates the program cleanly
    char ch = wgetch(main_screen);
    if(ch == 'q'){
	    finalize();
    }
    else if(ch == 'n'){
        char *path = getenv("SHELL");
        if(path == NULL)
            path = "/bin/bash";
        char *argv[2] = { " (ecran session)", NULL };
        SESSION* new_session = session_init(path, argv);
        if(new_session != NULL){
            session_setfg(new_session);
            wclear(main_screen);
            vscreen_sync(new_session->vscreen);
            set_status("new_session");
            update_time();
            num_sessions++;
        }
        else{
            flash();
        }
    }
    else if(ch >= '0' && ch <= '9'){
        if(!switch_terms(ch)){
            flash();
        }
    }
    else if(ch == 'k'){
        char termSession = wgetch(main_screen);
        if(termSession >= '0' && termSession <= '9'){
            int result = send_kill_signal(termSession - '0');
            if(result == 0)
                flash();
            if(result == 1){
                //kill(getppid(), SIGALRM);
                update_time();
                num_sessions--;
            }
        }
        else{
            flash();
        }
    }
    else if(ch == 'h'){
        if(displaying_help_screen){
            flash();
        }
        else{
        //help screen
            displaying_help_screen = 1;
            display_help_screen();
            set_all_lines_changed(fg_session->vscreen);
            vscreen_sync(fg_session->vscreen);
            displaying_help_screen = 0;
        }
    }
	// OTHER COMMANDS TO BE IMPLEMENTED
    else{
        flash();
    }

}
int send_kill_signal(int termSession){
    if(sessions[termSession] == NULL){
        return 0;
    }
    else{
        session_kill(sessions[termSession]);
        return 1;
    }
}

int switch_terms(char ch){
    int index = ch - '0';
    if(sessions[index] == NULL){
        return 0;
    }
    else{
        session_setfg(sessions[index]);

        return 1;
    }

}

/*
 * Function called from mainloop(), whose purpose is do any other processing
 * that has to be taken care of.  An example of such processing would be
 * to deal with sessions that have terminated.
 */
void do_other_processing() {
    // TO BE FILLED IN
    if(killed){
        killed = 0;
        if(terminated != NULL){
            waitpid(terminated->pid, NULL, WNOHANG);
            session_fini(terminated);
            terminated = NULL;
            if(fg_session == NULL){
                //exit cleanly
                delwin(status_screen);
                delwin(main_screen);
                delwin(stdscr);
                finalize();
            }
        }
    }

}

int get_num_sessions(){
    return num_sessions;
}
