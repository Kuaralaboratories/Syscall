# codegen.py

from llvmlite import ir
from ast import Program, Let, Add, Variable, Number, FunctionCall, Print

def generate_code(node, module, builder, named_values):
    if isinstance(node, Program):
        # Main function tanımla
        func_type = ir.FunctionType(ir.VoidType(), [])
        main_func = ir.Function(module, func_type, name="main")
        block = main_func.append_basic_block(name="entry")
        builder.position_at_end(block)

        # Program gövdesindeki ifadeleri işle
        for stmt in node.body:
            generate_code(stmt, module, builder, named_values)

        # Main fonksiyonunu bitir
        builder.ret_void()
    elif isinstance(node, Let):
        val = generate_code(node.expression, module, builder, named_values)
        named_values[node.variable] = val
    elif isinstance(node, Add):
        left = generate_code(node.left, module, builder, named_values)
        right = generate_code(node.right, module, builder, named_values)
        return builder.add(left, right, name="addtmp")
    elif isinstance(node, Variable):
        return named_values[node.name]
    elif isinstance(node, Number):
        return ir.Constant(ir.IntType(32), node.value)
    elif isinstance(node, FunctionCall):
        if node.name == 'print':
            # @print fonksiyonu için özel işlemler buraya eklenebilir
            if len(node.args) != 1:
                raise RuntimeError("@print fonksiyonu tek bir argüman bekler")
            val = generate_code(node.args[0], module, builder, named_values)
            # Örneğin, terminale yazdıralım
            print("Print:", val)
        elif node.name == 'log':
            # log fonksiyonu native olarak mevcut, ekrana yazdırmak için kullanılıyor
            if len(node.args) != 1:
                raise RuntimeError("log fonksiyonu tek bir argüman bekler")
            val = generate_code(node.args[0], module, builder, named_values)
            # Örneğin, terminale yazdıralım
            print("Log:", val)
        else:
            raise RuntimeError(f"Bilinmeyen fonksiyon: {node.name}")
    else:
        raise RuntimeError(f"Bilinmeyen düğüm türü: {type(node)}")

# LLVM modülü oluştur
module = ir.Module(name="syscall_module")

# AST'yi LLVM IR'ye dönüştürmek için örnek
if __name__ == "__main__":
    from sexpr_parser import parse_sexpression

    sexp = ['main', ['let', 'num', ['add', 1, 2]], 'num', ['@print', ['let', 'log', ['log', 'num']]]]
    ast = parse_sexpression(sexp)

    builder = ir.IRBuilder()
    named_values = {}
    generate_code(ast, module, builder, named_values)
    print(module)
