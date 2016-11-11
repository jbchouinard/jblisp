// Standard libs
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

// Installed libs
#include <readline/readline.h>
#include <readline/history.h>

void sighandler(int signo) {
    if (signo == SIGINT)
        puts("lol no");
}

int main(int argc, char** argv) {
    struct sigaction sa;

    sa.sa_handler = sighandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    
    if (sigaction(SIGINT, &sa, NULL) == -1)
        puts("sigaction is sad");

    puts("jblisp version 0.0.1");
    puts("Press ^C to exit\n");

    while (1) {
        char* input = readline("jblisp> ");
        add_history(input);
        printf("No, YOU'RE a %s\n", input);
        free(input);
    }
    return 0;
}
