# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from __future__ import (absolute_import, unicode_literals)

from qtpy.QtGui import QColor, QPalette
from qtpy.QtWidgets import (QWidget, QLineEdit, QPushButton, QHBoxLayout,
                            QColorDialog)


class ColorSelector(QWidget):

    def __init__(self, initial_color=None, parent=None):
        super(ColorSelector, self).__init__(parent=parent)

        self.initial_color = initial_color
        if not self.initial_color:
            self.initial_color = QColor(200, 200, 100)

        # Create line edit and push button and add to horizontal layout
        self.line_edit = QLineEdit(self)
        self.button = QPushButton(self)
        self.h_layout = QHBoxLayout(self)
        self.h_layout.addWidget(self.line_edit)
        self.h_layout.addWidget(self.button)
        self.h_layout.setContentsMargins(0, 0, 0, 0)

        self.line_edit.setText(self.initial_color.name())
        self.line_edit.setReadOnly(True)
        self.update_color_button()

        # Signals
        self.button.clicked.connect(self.launch_qcolor_dialog)
        self.line_edit.textChanged.connect(self.update_color_button)

    @property
    def current_color(self):
        return self.line_edit.text()

    def launch_qcolor_dialog(self):
        color_dialog = QColorDialog(self)
        color_dialog.setCurrentColor(QColor(self.current_color))
        color_dialog.colorSelected.connect(
            lambda: self.set_line_edit(color_dialog.selectedColor().name())
        )
        color_dialog.show()

    def set_color(self, color_hex):
        self.line_edit.setText(color_hex)

    def set_line_edit(self, color_hex):
        self.line_edit.setText(color_hex)

    def update_color_button(self):
        palette = QPalette(self.button.palette())
        qcolor = QColor(self.current_color)
        palette.setColor(QPalette.Button, qcolor)
        self.button.setAutoFillBackground(True)
        self.button.setPalette(palette)
        self.button.setFlat(True)
        self.button.update()
