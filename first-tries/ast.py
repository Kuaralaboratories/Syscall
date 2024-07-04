# ast.py

class Node:
    pass

class Program(Node):
    def __init__(self, body):
        self.body = body

class Let(Node):
    def __init__(self, variable, expression):
        self.variable = variable
        self.expression = expression

class Add(Node):
    def __init__(self, left, right):
        self.left = left
        self.right = right

class Variable(Node):
    def __init__(self, name):
        self.name = name

class Number(Node):
    def __init__(self, value):
        self.value = value

class FunctionCall(Node):
    def __init__(self, name, args):
        self.name = name
        self.args = args

class Print(Node):
    def __init__(self, expression):
        self.expression = expression
