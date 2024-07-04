# sexpr_parser.py

from ast import Program, Let, Add, Variable, Number, FunctionCall, Print

def parse_sexpression(sexp):
    if isinstance(sexp, int):
        return Number(sexp)
    elif isinstance(sexp, str):
        return Variable(sexp)
    elif isinstance(sexp, list):
        if sexp[0] == 'main':
            return Program([parse_sexpression(exp) for exp in sexp[1:]])
        elif sexp[0] == 'let':
            _, var, expr = sexp
            return Let(var, parse_sexpression(expr))
        elif sexp[0] == 'add':
            _, left, right = sexp
            return Add(parse_sexpression(left), parse_sexpression(right))
        elif sexp[0].startswith('@'):
            function_name = sexp[0][1:]
            args = [parse_sexpression(arg) for arg in sexp[1:]]
            return FunctionCall(function_name, args)
        elif sexp[0] == 'log':
            _, var = sexp
            return FunctionCall('log', [parse_sexpression(var)])
        else:
            raise SyntaxError(f"Unknown expression: {sexp}")
    else:
        raise SyntaxError(f"Unknown type: {type(sexp)}")

# Örnek kullanımı
if __name__ == "__main__":
    sexp = ['main', ['let', 'num', ['add', 1, 2]], 'num', ['@print', ['let', 'log', ['log', 'num']]]]
    ast = parse_sexpression(sexp)
    print(ast)
