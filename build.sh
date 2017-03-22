if [ ! -d build ]
then
    mkdir build
fi
gcc -Wall -std=c11 -g mpc.c -g repl.c -o build/jblisp -leditline -lm
