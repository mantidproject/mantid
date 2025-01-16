# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from mantid.plots.utility import convert_color_to_hex
from matplotlib import rcParams
from qtpy.QtCore import QRegExp
from qtpy.QtGui import QColor, QRegExpValidator
from qtpy.QtWidgets import QWidget, QLineEdit, QPushButton, QHBoxLayout, QColorDialog


MPL_DEFAULT = convert_color_to_hex(rcParams["lines.color"])


class ColorSelector(QWidget):
    def __init__(self, initial_color=MPL_DEFAULT, parent=None):
        super(ColorSelector, self).__init__(parent=parent)

        self.initial_color = QColor(initial_color)

        # Create line edit and push button and add to a horizontal layout
        self.line_edit = QLineEdit(self)
        self.button = QPushButton(self)
        self.h_layout = QHBoxLayout(self)
        self.h_layout.addWidget(self.line_edit)
        self.h_layout.addWidget(self.button)
        self.h_layout.setContentsMargins(0, 0, 0, 0)

        self.line_edit.setText(self.initial_color.name())
        self.prev_color = self.initial_color.name()

        # Color input only allows valid hex codes.
        re = QRegExp("^#([A-Fa-f0-9]{6}|[A-Fa-f0-9]{3})$")
        validator = ColorValidator(re, self.line_edit, self)
        self.line_edit.setValidator(validator)

        self.button.setAutoFillBackground(True)
        self.button.setFlat(True)
        self.update_color_button()

        # Signals
        self.button.clicked.connect(self.launch_qcolor_dialog)
        self.line_edit.textChanged.connect(self.update_color_button)
        self.line_edit.editingFinished.connect(self.convert_three_digit_hex_to_six)

    def get_color(self):
        return self.line_edit.text()

    def launch_qcolor_dialog(self):
        color_dialog = QColorDialog(self)
        color_dialog.setCurrentColor(QColor(self.get_color()))
        color_dialog.colorSelected.connect(lambda: self.set_line_edit(color_dialog.selectedColor().name()))
        color_dialog.accepted.connect(lambda: self.set_prev_color(color_dialog.selectedColor().name()))
        color_dialog.setModal(True)
        color_dialog.show()

    def set_prev_color(self, color):
        self.prev_color = color

    def set_color(self, color_hex):
        self.line_edit.setText(convert_color_to_hex(color_hex))

    def set_line_edit(self, color_hex):
        self.line_edit.setText(color_hex)

    def update_color_button(self):
        color = self.get_color()
        self.button.setStyleSheet(f"border:1px solid #000000;background-color: {color}")
        self.button.update()

    def convert_three_digit_hex_to_six(self):
        color = self.get_color()

        # If a 3-digit hex code is inputted, it is converted to 6 digits
        # by duplicating each digit.
        if len(color) == 4:
            new = "#{}".format("".join(2 * c for c in color.lstrip("#")))
            self.set_color(new)
            color = new

        self.prev_color = color


class ColorValidator(QRegExpValidator):
    def __init__(self, regexp, widget, color_selector):
        QRegExpValidator.__init__(self, regexp, widget)
        self.color_selector = color_selector

    def fixup(self, text):
        # If an invalid color is inputted, the field reverts back
        # to the last valid color entered.
        self.color_selector.set_color(self.color_selector.prev_color)
