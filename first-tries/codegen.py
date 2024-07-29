# codegen.py

from llvmlite import ir
from ast import Program, Let, Add, Variable, Number, FunctionDef, FunctionCall

def generate_code(node, module, builder, named_values):
    if isinstance(node, Program):
        # Main function tanımla
        func_type = ir.FunctionType(ir.IntType(32), [])
        main_func = ir.Function(module, func_type, name="main")
        block = main_func.append_basic_block(name="entry")
        builder.position_at_end(block)

        # Program gövdesindeki ifadeleri işle
        for stmt in node.body:
            generate_code(stmt, module, builder, named_values)

        # 3 sayısını döndür
        builder.ret(ir.Constant(ir.IntType(32), 3))

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

    elif isinstance(node, FunctionDef):
        # Fonksiyon tipi ve parametreler
        func_type = ir.FunctionType(ir.IntType(32), [ir.IntType(32)] * len(node.params))
        func = ir.Function(module, func_type, name=node.name)

        # Parametreleri named_values içine ekle
        block = func.append_basic_block(name="entry")
        builder.position_at_end(block)
        local_named_values = {param: func.args[i] for i, param in enumerate(node.params)}
        named_values.update(local_named_values)

        # Fonksiyon gövdesindeki ifadeleri işle
        for stmt in node.body:
            generate_code(stmt, module, builder, named_values)

        # Sonuç döndür
        builder.ret(ir.Constant(ir.IntType(32), 3))

    elif isinstance(node, FunctionCall):
        if node.name == 'print':
            # @print fonksiyonu için özel işlemler buraya eklenebilir
            if len(node.args) != 1:
                raise RuntimeError("@print fonksiyonu tek bir argüman bekler")
            val = generate_code(node.args[0], module, builder, named_values)
            # Python'da terminale yazdıralım
            print("Print:", val)
        elif node.name == 'log':
            # log fonksiyonu native olarak mevcut, ekrana yazdırmak için kullanılıyor
            if len(node.args) != 1:
                raise RuntimeError("log fonksiyonu tek bir argüman bekler")
            val = generate_code(node.args[0], module, builder, named_values)
            # Python'da terminale yazdıralım
            print("Log:", val)
        else:
            # Kullanıcı tanımlı fonksiyon çağrıları
            func = module.get_global(node.name)
            args = [generate_code(arg, module, builder, named_values) for arg in node.args]
            return builder.call(func, args)

    else:
        raise RuntimeError(f"Bilinmeyen düğüm türü: {type(node)}")

# LLVM modülü oluştur
module = ir.Module(name="syscall_module")

# AST'yi LLVM IR'ye dönüştürmek için örnek
if __name__ == "__main__":
    from sexpr_parser import parse_sexpression

    sexp = [
        'defun', 'half-adder', ['a', 'b'],
            ['let', 'sum', ['xor', 'a', 'b']],
            ['let', 'carry', ['and', 'a', 'b']],
            ['list', 'sum', 'carry'],

        'defun', 'full-adder', ['a', 'b', 'carry-in'],
            ['let', 'half1', ['half-adder', 'a', 'b']],
            ['let', 'sum1', ['first', 'half1']],
            ['let', 'carry1', ['second', 'half1']],
            ['let', 'half2', ['half-adder', 'sum1', 'carry-in']],
            ['let', 'sum', ['first', 'half2']],
            ['let', 'carry2', ['second', 'half2']],
            ['let', 'carry', ['or', 'carry1', 'carry2']],
            ['list', 'sum', 'carry'],

        'defun', 'add-4bit', ['a3', 'a2', 'a1', 'a0', 'b3', 'b2', 'b1', 'b0', 'carry-in'],
            ['let', 'adder0', ['full-adder', 'a0', 'b0', 'carry-in']],
            ['let', 'sum0', ['first', 'adder0']],
            ['let', 'carry0', ['second', 'adder0']],
            ['let', 'adder1', ['full-adder', 'a1', 'b1', 'carry0']],
            ['let', 'sum1', ['first', 'adder1']],
            ['let', 'carry1', ['second', 'adder1']],
            ['let', 'adder2', ['full-adder', 'a2', 'b2', 'carry1']],
            ['let', 'sum2', ['first', 'adder2']],
            ['let', 'carry2', ['second', 'adder2']],
            ['let', 'adder3', ['full-adder', 'a3', 'b3', 'carry2']],
            ['let', 'sum3', ['first', 'adder3']],
            ['let', 'carry3', ['second', 'adder3']],
            ['list', 'sum3', 'sum2', 'sum1', 'sum0', 'carry3'],

        'main', 
            ['let', 'result', ['add-4bit', 1, 0, 1, 0, 1, 1, 0, 1, 0]],
            ['let', 'sum3', ['first', 'result']],
            ['let', 'sum2', ['second', 'result']],
            ['let', 'sum1', ['third', 'result']],
            ['let', 'sum0', ['fourth', 'result']],
            ['let', 'carry', ['fifth', 'result']],
            ['@print', 'sum3'],
            ['@print', 'sum2'],
            ['@print', 'sum1'],
            ['@print', 'sum0'],
            ['@print', 'carry']
    ]
    ast = parse_sexpression(sexp)

    builder = ir.IRBuilder()
    named_values = {}
    generate_code(ast, module, builder, named_values)
    print(module)
