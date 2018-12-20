class Model(object):
    def __init__(self):
        self.result = 0

    def calc(self, value1, operation, value2):
        if operation == "+":
            self.result = value1 + value2
        elif operation == "-":
            self.result = value1 - value2
        return self.result
