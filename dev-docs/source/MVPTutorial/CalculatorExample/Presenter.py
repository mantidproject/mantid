# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +


class Presenter:

    def __init__(self, view, model):
        self._view = view
        self._model = model

        self._view.subscribe_presenter(self)

        self._view.set_options("operations", ["+", "-"])
        self._view.set_options("display", ["print", "update", "print and update"])
        self._view.show_display(False)

    def handle_button_clicked(self) -> None:
        value1 = self._view.get_value(0)
        operation = self._view.get_operation()
        value2 = self._view.get_value(2)

        result = self._model.calculate(value1, operation, value2)

        if self._model.print_to_screen():
            print(result)

        self._view.set_result(result)

    def handle_display_changed(self) -> None:
        display = self._view.get_display()
        self._model.set_print_to_screen(display != "update")
        if display == "update":
            self._view.show_display(True)
        elif display == "print":
            self._view.show_display(False)
        else:
            self._view.show_display(True)
