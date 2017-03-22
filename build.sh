if [ ! -d build ]
then
    mkdir build
fi
gcc -Wall -std=c11 -c -g mpc.c -g jblisp.c -g repl.c -g tests/test.c
gcc -o build/jblisp mpc.o jblisp.o repl.o -lm -leditline
gcc -o build/test mpc.o jblisp.o test.o -lm
