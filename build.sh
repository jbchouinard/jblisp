if [ ! -d build ]
then
    mkdir build
fi
gcc -Wall -std=c99 -g mpc.c -g repl.c -o build/jblisp -leditline -lm
