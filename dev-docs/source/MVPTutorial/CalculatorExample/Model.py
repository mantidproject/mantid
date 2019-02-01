# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +


class Model(object):
    def __init__(self):
        self.result = 0

    def calc(self, value1, operation, value2):
        if operation == "+":
            self.result = value1 + value2
        elif operation == "-":
            self.result = value1 - value2
        return self.result
