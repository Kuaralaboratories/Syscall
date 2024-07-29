# sexpr_parser.py

from ast import Program, Let, Add, Variable, Number, FunctionDef, FunctionCall

def parse_sexpression(sexp):
    if isinstance(sexp, list):
        if sexp[0] == 'main':
            return Program([parse_sexpression(exp) for exp in sexp[1:]])
        elif sexp[0] == 'let':
            var = sexp[1]
            expr = sexp[2]
            return Let(var, parse_sexpression(expr))
        elif sexp[0] == 'add':
            left = sexp[1]
            right = sexp[2]
            return Add(parse_sexpression(left), parse_sexpression(right))
        elif sexp[0].startswith('@'):
            name = sexp[0][1:]
            if name == 'print':
                return FunctionCall(name, [parse_sexpression(arg) for arg in sexp[1:]])
            else:
                params = sexp[1]
                body = sexp[2:]
                return FunctionDef(name, params, [parse_sexpression(exp) for exp in body])
        else:
            return FunctionCall(sexp[0], [parse_sexpression(arg) for arg in sexp[1:]])
    elif isinstance(sexp, int):
        return Number(sexp)
    elif isinstance(sexp, str):
        return Variable(sexp)
    else:
        raise SyntaxError(f"Unknown expression: {sexp}")
