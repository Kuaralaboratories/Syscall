// ast.c

#include <stdlib.h>
#include <string.h>
#include "ast.h"

Node* create_program(Node** body, int body_count) {
    Node* node = malloc(sizeof(Node));
    node->type = PROGRAM;
    node->data.program.body = body;
    node->data.program.body_count = body_count;
    return node;
}

Node* create_let(char* variable, Node* expression) {
    Node* node = malloc(sizeof(Node));
    node->type = LET;
    node->data.let.variable = strdup(variable);
    node->data.let.expression = expression;
    return node;
}

Node* create_add(Node* left, Node* right) {
    Node* node = malloc(sizeof(Node));
    node->type = ADD;
    node->data.add.left = left;
    node->data.add.right = right;
    return node;
}

Node* create_variable(char* name) {
    Node* node = malloc(sizeof(Node));
    node->type = VARIABLE;
    node->data.variable.name = strdup(name);
    return node;
}

Node* create_number(int value) {
    Node* node = malloc(sizeof(Node));
    node->type = NUMBER;
    node->data.number.value = value;
    return node;
}

Node* create_function_def(char* name, char** params, int param_count, Node** body, int body_count) {
    Node* node = malloc(sizeof(Node));
    node->type = FUNCTION_DEF;
    node->data.function_def.name = strdup(name);
    node->data.function_def.params = params;
    node->data.function_def.param_count = param_count;
    node->data.function_def.body = body;
    node->data.function_def.body_count = body_count;
    return node;
}

Node* create_function_call(char* name, Node** args, int arg_count) {
    Node* node = malloc(sizeof(Node));
    node->type = FUNCTION_CALL;
    node->data.function_call.name = strdup(name);
    node->data.function_call.args = args;
    node->data.function_call.arg_count = arg_count;
    return node;
}

void free_node(Node* node) {
    if (!node) return;
    switch (node->type) {
        case PROGRAM:
            for (int i = 0; i < node->data.program.body_count; i++) {
                free_node(node->data.program.body[i]);
            }
            free(node->data.program.body);
            break;
        case LET:
            free(node->data.let.variable);
            free_node(node->data.let.expression);
            break;
        case ADD:
            free_node(node->data.add.left);
            free_node(node->data.add.right);
            break;
        case VARIABLE:
            free(node->data.variable.name);
            break;
        case NUMBER:
            break;
        case FUNCTION_DEF:
            free(node->data.function_def.name);
            for (int i = 0; i < node->data.function_def.param_count; i++) {
                free(node->data.function_def.params[i]);
            }
            free(node->data.function_def.params);
            for (int i = 0; i < node->data.function_def.body_count; i++) {
                free_node(node->data.function_def.body[i]);
            }
            free(node->data.function_def.body);
            break;
        case FUNCTION_CALL:
            free(node->data.function_call.name);
            for (int i = 0; i < node->data.function_call.arg_count; i++) {
                free_node(node->data.function_call.args[i]);
            }
            free(node->data.function_call.args);
            break;
    }
    free(node);
}
