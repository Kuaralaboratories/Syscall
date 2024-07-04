# main.py

from sexpr_parser import parse_sexpression
from codegen import generate_code
from llvmlite import ir

# Örnek S-expression
sexp = ['main', ['let', 'num', ['add', 1, 2]], 'num', ['@print', ['let', 'log', ['log', 'num']]]]

# S-expression'ı AST'ye dönüştür
ast = parse_sexpression(sexp)

# LLVM modülü oluştur
module = ir.Module(name="syscall_module")
named_values = {}

# AST'yi LLVM IR'ye dönüştür
builder = ir.IRBuilder()
generate_code(ast, module, builder, named_values)

# Üretilen LLVM IR kodunu yazdır
print(module)
