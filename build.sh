if [ ! -d build ]
then
    mkdir build
fi
gcc -Wall -std=c11 -g mpc.c -g jblisp.c -g repl.c -o build/jblisp -leditline -lm
gcc -Wall -std=c11 -g mpc.c -g jblisp.c -g tests/test.c -o build/test -lm
