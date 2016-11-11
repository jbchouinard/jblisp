#include <stdio.h>

static char input[2048];

int main(int argc, char** argv) {

    puts("jblisp version 0.0.1");
    puts("Press ^C to exit\n");

    while (1) {
        fputs("jblisp> ", stdout);
        fgets(input, 2048, stdin);
        printf("No, YOU'RE a %s", input);
    }
    return 0;
}
