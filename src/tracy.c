#include <signal.h>   // sigaction etc
#include <syslog.h>   // syslog API
#include <stdio.h>    // printf and co
#include <stdint.h>   // int32_t
#include <stdlib.h>   // _exit()
#include <stdbool.h>  // bool type
#include <stdarg.h>   // variadic function variables (va_list etc)
#include <assert.h>   // assert
#include <fcntl.h>    // open system call and flags
#include <unistd.h>   // read() system call
#include <libgen.h>   // basename
#include <stdio.h>    // printf
#include <string.h>   // strcat, strlen
#include <pthread.h>  // pthread_mutex_t and functions

#include "tracy.h"

// -------------------------------
//      Constants
// -------------------------------
#define TEST_NAME_BUFFSZ 300
#define SRCF_NAME_BUFFSZ 300
#define PROG_NAME_BUFFSZ 129

// -------------------------------
//      Struct definitions
// -------------------------------
struct stack_trace{
    size_t count;
    struct trace_frame *top; 
};

struct trace_frame{
    struct trace_frame *prev;
    char func_name[TEST_NAME_BUFFSZ];
    char source_file[SRCF_NAME_BUFFSZ];
    uint32_t line_num;
};

// --------------------------------------------
//      File-scoped global declarations
// -------------------------------------------
static bool TRACY_USE_SYSLOG = false;                  // dump stacktrace to system logger
static bool TRACY_USE_LOGFILE = false;                 // dump stacktrace to (specified or default) log file
static char *TRACY_LOGF_PATH = TRACY_DEFAULT_LOG_PATH; // path to log file (only used if TRACY_USE_LOGFILE is True) 
static char PROG_NAME[PROG_NAME_BUFFSZ] = {0};         // Tracy_init will save the name of the program here

struct stack_trace tracelog;                           // thread-SHARED stack tracelog; this is what gets dumped
pthread_mutex_t tracy_mtx = PTHREAD_MUTEX_INITIALIZER; // mutex to make the functions thread-safe
// ===============================================================


// -------------------------------
//      Function definitions
// -------------------------------
/*
 * Append the string in src to buff IFF there is enough room.
 * 
 * If there's enough room, src is appended to buff and True is returned.
 * Else False is returned.
 *
 * buff MUST be initialized to all 0s by the caller -- otherwise results
 * are undefined (if buff is already populated, there might not be enough
 * space to append src).
 *
 * buffsz must be the total size of buff, INCLUDING the terminating NULL byte.
 */
static bool strappend(char buff[], size_t buffsz, const char *src){
    if (!(strlen(src) + strlen(buff) < buffsz)){
        // printf("not enough room in '%s' for '%s'\n", buff, src);
        return false;
    }
    strcat(buff, src);
    return true;
};

/*
 * Get program name and save it in the file-local buffer
 * (PROG_NAME_BUFFSZ).
 */
static void __Tracy_get_progname(void){
    char prog_name[1025] = {0};
    int procfd = open("/proc/self/cmdline", O_RDONLY);
    read(procfd, prog_name, 1024);
    snprintf(PROG_NAME, PROG_NAME_BUFFSZ, "%s", basename(prog_name));
}

/*
 * Allocate memory for and initialize a new struct trace_frame
 * to be added on the tracelog stack.
 */
static struct trace_frame *__Tracy_newframe(const char *source_file, const char *func_name, int line_num){
    struct trace_frame *new = calloc(1, sizeof(struct trace_frame));
    assert(new);
    
    new->line_num = line_num;
    snprintf(new->func_name, sizeof(new->func_name), "%s", func_name);
    snprintf(new->source_file, sizeof(new->source_file), "%s", source_file);
    
    return new;
}

/*
 * Create new stack trace frame and push it onto the stack.
 */
void Tracy_push(const char *source_file, const char *func_name, int line_num){
    assert(!pthread_mutex_lock(&tracy_mtx));

    ++tracelog.count;
    struct trace_frame *prev = tracelog.top;
    tracelog.top = __Tracy_newframe(source_file, func_name, line_num); // new stack top
    tracelog.top->prev = prev; // the item on the stack just underneath


    assert(!pthread_mutex_unlock(&tracy_mtx));
}

/* 
 * Free all stack trace frames on the tracelog stack.
 */
void Tracy_destroy(void){
    assert(!pthread_mutex_lock(&tracy_mtx));

    int32_t count = tracelog.count;
    
    struct trace_frame *tmp = tracelog.top;
    while (count){
        tmp = tracelog.top->prev;
        free(tracelog.top);
        tracelog.top = tmp;
        --count;
    }

    assert(!pthread_mutex_lock(&tracy_mtx));
}

/*
 * Pop the top of the stack and free it.
 * Does nothing if called on an empty stack.
 */
void Tracy_pop(void){
    assert(!pthread_mutex_lock(&tracy_mtx));

    if (!tracelog.count) goto unlock;

    --tracelog.count;
    struct trace_frame *top = tracelog.top;
    tracelog.top = tracelog.top->prev;
    free(top);

unlock:
    assert(!pthread_mutex_lock(&tracy_mtx));
}

/*
 * Write a message to stdout, syslog, or a file.
 *
 * You can choose where __Tracy_log() writes to by setting
 * an environment variable, as follows:
 * - TRACY_USE_SYSLOG   // write to the system logger
 * - TRACY_USE_LOG_FILE // write to file 
 *
 * If neither is specified, the default is to write to stdout.
 * If TRACY_USE_LOG_FILE is set, then the default file to write
 * to is /tmp/tracy.log; this can be overridden by setting 
 * TRACY_LOG_FILE.
 *
 * TLDR: use stdout or a file to dump tracelog as syslog can't handle 
 * multline messages.
 *
 * Note that writing to syslog will most likely not do what you want:
 * syslog (as per the relevant RFC) is line-oriented and newline escapes
 * are removed. The result is that a multiline message (containing \n 
 * escapes) will appear on a SINGLE line. This is rarely what you want, 
 * as it makes everything unreadable. Similarly, were tracy to print a
 * tracelog split over multiple lines, it would appear interspersed with
 * other system messages, which again is not what you want. 
 */
static void __Tracy_log(char *fmt, ...){
    assert(!pthread_mutex_lock(&tracy_mtx));

    // write to system logger
    if (TRACY_USE_SYSLOG){
        va_list (vargs);
        va_start(vargs, fmt);
        vsyslog(LOG_DEBUG, fmt, vargs);
        va_end(vargs);
    }

    // write to file
    else if (TRACY_USE_LOGFILE){
        FILE *tf = fopen(TRACY_LOGF_PATH, "a");
        assert(tf);

        va_list (vargs);
        va_start(vargs, fmt);
        vfprintf(tf, fmt, vargs);
        va_end(vargs);

        fflush(tf);
        fclose(tf);
    }
    
    // write to stdout
    else{
        va_list (vargs);
        va_start(vargs, fmt);
        vfprintf(stdout, fmt, vargs);
        va_end(vargs);
    }

    assert(!pthread_mutex_lock(&tracy_mtx));
}

/*
 * Print the current stack tracelog.
 *
 * The function is thread-safe and can be called from
 * multiple threads. Note however that the stack tracelog
 * is SHARED by all threads i.e. it's not per-thread.
 *
 * The tracelog is built piecemeal into a long string that
 * gets printed out (to stdout, a file, or syslog) all at 
 * once via __Tracy_log().
 */
void Tracy_dump(void){
    assert(!pthread_mutex_lock(&tracy_mtx));

    uint32_t count = tracelog.count;
    struct trace_frame *top = tracelog.top;
    bool do_print = false;

    // there's something to print: stacktrace log not empty
    if (count) do_print = true;

    // concatenate whole message here
    char buff[4096] = {0};
    char tmpbuff[4096] = {0};

    if (do_print){  // build up message mainly consisting of stack tracelog
        if (!strappend(buff, sizeof(buff), "-----------------------------------\n")){
            puts("not enough room for stacktrace log dump");
            goto unlock;
        }
        
        snprintf(tmpbuff, 4096, "=== Stack trace of '%s' ===\n", PROG_NAME);
        if (!strappend(buff, sizeof(buff), tmpbuff)){
            puts("not enough room for stacktrace log dump");
            goto unlock;
        }

        strappend(buff, sizeof(buff), "-----------------------------------\n");
        strappend(buff, sizeof(buff), "  ^\n");

        while (count){
            snprintf(tmpbuff, sizeof(tmpbuff), "  | %-10i *** %s(), %s: L%i\n", count, top->func_name,
                                                        top->source_file, 
                                                        top->line_num
                    );

            if (!strappend(buff, sizeof(buff), tmpbuff)){
                goto unlock; 
            }

            --count;
            top = top->prev;
        }
    }
    
    // write to file, stdout, or syslog
    __Tracy_log("%s", buff); 

unlock:
    assert(!pthread_mutex_unlock(&tracy_mtx));
}

/*
 * On fatal signal caught (SIGINT, SIGBUS, SIGSEGV), print some
 * debug information relating to the signal and dump stack tracelog 
 * and exit.
 *
 * Note it's NOT async signal safe to use stdio functions;
 * however, they're used here because it doesn't really matter -
 * the program is exitted immediately after anyway so the output
 * coming out wrong or any internal buffers being corrupted is of
 * little significance.
 */
static void sighandler(int signum, siginfo_t *siginfo, void *ucontext){
    printf("in sighandler\n");
    __Tracy_log("^^^^^ '%s' terminated with signal %i (%s), caused by error code %i (%s)\n", 
                            PROG_NAME, 
                            siginfo->si_signo, 
                            strsignal(siginfo->si_signo), 
                            siginfo->si_code, 
                            strerror(siginfo->si_code)
                            );
    Tracy_dump();
    _exit(EXIT_FAILURE);
}

/*
 * Register sighandler as the signal handler for the following signals:
 * SIGBUS, SIGSEGV, SIGINT.
 *
 * When the signal handler runs all three signals are added the process
 * signal mask so they can't interrupt the signal handler.
 */
void __register_sighandler(void){
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    // add these to the process mask while the handler runs
    sigaddset(&sa.sa_mask, SIGBUS);
    sigaddset(&sa.sa_mask, SIGINT);
    sigaddset(&sa.sa_mask, SIGSEGV);

    // pass additional information to signal handler
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = sighandler;
    assert(!sigaction(SIGINT, &sa, NULL));
    assert(!sigaction(SIGBUS, &sa, NULL));
    assert(!sigaction(SIGSEGV, &sa, NULL));
}

/*
 * Initialize tracy.
 *
 * Initialize file-scoped global structures and variables
 * (the actual tracelog struct, flags saying whether to write
 * to syslog, stdout, or a file etc), register signal handler,
 * etc.
 */
void Tracy_init(void){
    assert(!pthread_mutex_lock(&tracy_mtx));

    __Tracy_get_progname();
    if (getenv(TRACY_SYSLOG_ENV_VAR)){
        TRACY_USE_SYSLOG = true;
        puts("will use syslog");
    }
    else if(getenv(TRACY_USE_LOGF_ENV_VAR)){
        TRACY_USE_LOGFILE = true;
        if (getenv(TRACY_LOGF_NAME_ENV_VAR)) TRACY_LOGF_PATH = getenv(TRACY_LOGF_NAME_ENV_VAR);
        puts("will use logfile");
    }
    
    __register_sighandler();

    assert(!pthread_mutex_lock(&tracy_mtx));
}


