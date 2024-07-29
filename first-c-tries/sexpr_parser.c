// sexpr_parser.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"

Node* parse_sexpression(char** sexp, int count) {
    if (count == 0) return NULL;

    if (strcmp(sexp[0], "main") == 0) {
        Node** body = malloc((count - 1) * sizeof(Node*));
        for (int i = 1; i < count; i++) {
            body[i - 1] = parse_sexpression(&sexp[i], 1);
        }
        return create_program(body, count - 1);
    } else if (strcmp(sexp[0], "let") == 0) {
        return create_let(sexp[1], parse_sexpression(&sexp[2], 1));
    } else if (strcmp(sexp[0], "add") == 0) {
        return create_add(parse_sexpression(&sexp[1], 1), parse_sexpression(&sexp[2], 1));
    } else if (sexp[0][0] == '@') {
        char* name = &sexp[0][1];
        if (strcmp(name, "print") == 0) {
            Node** args = malloc((count - 1) * sizeof(Node*));
            for (int i = 1; i < count; i++) {
                args[i - 1] = parse_sexpression(&sexp[i], 1);
            }
            return create_function_call(name, args, count - 1);
        } else {
            char** params = malloc((count - 2) * sizeof(char*));
            for (int i = 1; i < count - 1; i++) {
                params[i - 1] = strdup(sexp[i]);
            }
            Node** body = malloc((count - 1) * sizeof(Node*));
            for (int i = 2; i < count; i++) {
                body[i - 2] = parse_sexpression(&sexp[i], 1);
            }
            return create_function_def(name, params, count - 2, body, count - 2);
        }
    } else {
        return create_function_call(sexp[0], NULL, 0);
    }
}
