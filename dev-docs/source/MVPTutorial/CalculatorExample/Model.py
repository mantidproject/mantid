# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +


class Model:

    def __init__(self):
        self._result: float = 0.0
        self._print_to_screen: bool = True

    def calculate(self, value1: float, operation: str, value2: float) -> float:
        if operation == "+":
            self._result = value1 + value2
        elif operation == "-":
            self._result = value1 - value2
        return self._result

    def set_print_to_screen(self, display: bool) -> None:
        self._print_to_screen = display

    def print_to_screen(self) -> bool:
        return self._print_to_screen
