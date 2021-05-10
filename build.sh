#!/bin/bash
set -eux
mkdir -p build bin
gcc -Wall -std=c11 -c -g mpc.c -g jblisp.c -g repl.c -g tests/test.c
gcc -o bin/jblisp mpc.o jblisp.o repl.o -lm -lreadline
gcc -o bin/test mpc.o jblisp.o test.o -lm
