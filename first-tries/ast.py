# ast.py

class Program:
    def __init__(self, body):
        self.body = body

class Let:
    def __init__(self, variable, expression):
        self.variable = variable
        self.expression = expression

class Add:
    def __init__(self, left, right):
        self.left = left
        self.right = right

class Variable:
    def __init__(self, name):
        self.name = name

class Number:
    def __init__(self, value):
        self.value = value

class FunctionDef:
    def __init__(self, name, params, body):
        self.name = name
        self.params = params
        self.body = body

class FunctionCall:
    def __init__(self, name, args):
        self.name = name
        self.args = args
