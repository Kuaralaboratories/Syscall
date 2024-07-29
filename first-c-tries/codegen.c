// codegen.c

#include <stdio.h>
#include <stdlib.h>
#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
#include <llvm-c/TargetMachine.h>
#include <llvm-c/Transforms/Scalar.h>
#include "ast.h"

LLVMValueRef generate_code(Node* node, LLVMModuleRef module, LLVMBuilderRef builder, LLVMValueRef named_values) {
    switch (node->type) {
        case PROGRAM: {
            LLVMTypeRef func_type = LLVMFunctionType(LLVMInt32Type(), NULL, 0, 0);
            LLVMValueRef main_func = LLVMAddFunction(module, "main", func_type);
            LLVMBasicBlockRef block = LLVMAppendBasicBlock(main_func, "entry");
            LLVMPositionBuilderAtEnd(builder, block);

            for (int i = 0; i < node->data.program.body_count; i++) {
                generate_code(node->data.program.body[i], module, builder, named_values);
            }

            LLVMValueRef ret_val = LLVMConstInt(LLVMInt32Type(), 3, 0);
            LLVMBuildRet(builder, ret_val);
            break;
        }
        case LET: {
            LLVMValueRef val = generate_code(node->data.let.expression, module, builder, named_values);
            // named_values[node->data.let.variable] = val; // Implement a proper symbol table for named values
            break;
        }
        case ADD: {
            LLVMValueRef left = generate_code(node->data.add.left, module, builder, named_values);
            LLVMValueRef right = generate_code(node->data.add.right, module, builder, named_values);
            return LLVMBuildAdd(builder, left, right, "addtmp");
        }
        case VARIABLE: {
            // return named_values[node->data.variable.name]; // Implement a proper symbol table for named values
            break;
        }
        case NUMBER: {
            return LLVMConstInt(LLVMInt32Type(), node->data.number.value, 0);
        }
        case FUNCTION_DEF: {
            // Implement function definition handling
            break;
        }
        case FUNCTION_CALL: {
            if (strcmp(node->data.function_call.name, "print") == 0) {
                if (node->data.function_call.arg_count != 1) {
                    fprintf(stderr, "@print function expects one argument\n");
                    exit(1);
                }
                LLVMValueRef val = generate_code(node->data.function_call.args[0], module, builder, named_values);
                printf("Print: %s\n", LLVMPrintValueToString(val));
            } else {
                // Implement user-defined function calls
            }
            break;
        }
    }
    return NULL;
}

int main() {
    LLVMModuleRef module = LLVMModuleCreateWithName("syscall_module");
    LLVMBuilderRef builder = LLVMCreateBuilder();

    char* sexp[] = {
        "defun", "myfunc", "a", "b",
            "add", "a", "b",
        "main",
            "let", "num", "myfunc", "1", "2",
            "@print", "let", "log", "log", "num"
    };
    Node* ast = parse_sexpression(sexp, sizeof(sexp) / sizeof(char*));

    generate_code(ast, module, builder, NULL);
    LLVMDumpModule(module);

    LLVMDisposeBuilder(builder);
    LLVMDisposeModule(module);
    free_node(ast);

    return 0;
}
