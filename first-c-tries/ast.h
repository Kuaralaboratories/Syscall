// ast.h

#ifndef AST_H
#define AST_H

typedef struct Node {
    enum { PROGRAM, LET, ADD, VARIABLE, NUMBER, FUNCTION_DEF, FUNCTION_CALL } type;
    union {
        struct { struct Node** body; int body_count; } program;
        struct { char* variable; struct Node* expression; } let;
        struct { struct Node* left; struct Node* right; } add;
        struct { char* name; } variable;
        struct { int value; } number;
        struct { char* name; char** params; int param_count; struct Node** body; int body_count; } function_def;
        struct { char* name; struct Node** args; int arg_count; } function_call;
    } data;
} Node;

Node* create_program(Node** body, int body_count);
Node* create_let(char* variable, Node* expression);
Node* create_add(Node* left, Node* right);
Node* create_variable(char* name);
Node* create_number(int value);
Node* create_function_def(char* name, char** params, int param_count, Node** body, int body_count);
Node* create_function_call(char* name, Node** args, int arg_count);
void free_node(Node* node);

#endif
