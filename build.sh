if [ ! -d build ]
then
    mkdir build
fi
gcc -Wall -g mpc.c -g repl.c -o build/jblisp -leditline -lm
