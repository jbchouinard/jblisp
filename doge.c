#include <stdio.h>
#include <stdlib.h>

#include <editline/readline.h>
#include <editline/history.h>

#include "mpc.h"


int number_of_nodes(mpc_ast_t* ast) {
	if (ast->children_num == 0)
        return 1;
    else {
        int total = 1;
        for (int i = 0; i < ast->children_num; i++) {
            total += number_of_nodes(ast->children[i]);
        }
        return total;
    }
    return 0;
}

int main(int argc, char** argv) {
    mpc_parser_t* Adjective = mpc_new("adjective");
    mpc_parser_t* Noun      = mpc_new("noun");
    mpc_parser_t* Phrase    = mpc_new("phrase");
    mpc_parser_t* Doge      = mpc_new("doge");


    mpca_lang(MPCA_LANG_DEFAULT,
        "                                                                     \
        adjective : \"wow\" | \"many\" | \"so\" | \"such\" ;                  \
        noun      : \"lisp\" | \"language\" | \"book\" | \"build\" | \"c\" ;  \
        phrase    : <adjective> <noun> ;                                      \
        doge      : /^/ <phrase>+ /$/ ;                                       \
        ",
        Adjective, Noun, Phrase, Doge
    );

    puts("doge parser version 0.1.0");
    puts("Press ^C to exit\n");

    mpc_result_t res;
    while (1) {
        char* input = readline("doge> ");
        // Parse input
        add_history(input);
        if (mpc_parse("<stdin>", input, Doge, &res)) {
            mpc_ast_print(res.output);
            mpc_ast_t* a = res.output;
            printf("Number of nodes: %i\n", number_of_nodes(a));
            mpc_ast_delete(res.output);
        } else {
            mpc_err_print(res.error);
            mpc_err_delete(res.error);
        }
        free(input);
    }
    return 0;
}
