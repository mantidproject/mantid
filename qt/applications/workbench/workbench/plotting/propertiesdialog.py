#  This file is part of the mantid workbench.
#
#  Copyright (C) 2017 mantidproject
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
from __future__ import (absolute_import, unicode_literals)

# std imports

# 3rdparty imports
from qtpy.QtWidgets import QDialog, QHBoxLayout, QLineEdit, QPushButton, QVBoxLayout


class PropertiesEditorBase(QDialog):
    """Base class for all dialogs responsible for providing
    access to change figure properties by clicking on the canvas"""

    def __init__(self, canvas):
        """
        :param canvas: A reference to the canvas to be updated
        """
        super(PropertiesEditorBase, self).__init__()
        self.canvas = canvas
        self.layout = QVBoxLayout()
        self.setLayout(self.layout)
        self.ok_button = QPushButton("OK", self)
        self.cancel_button = QPushButton("Cancel", self)
        self.button_row = QHBoxLayout()
        self.button_row.addWidget(self.ok_button)
        self.button_row.addWidget(self.cancel_button)
        self.ok_button.clicked.connect(self.on_ok)
        self.cancel_button.clicked.connect(self.reject)

    def on_ok(self):
        self.changes_accepted()
        self.canvas.draw_idle()
        self.accept()

    def changes_accepted(self):
        raise NotImplementedError("Derived classes should override changes_accepted()")


class LabelEditor(PropertiesEditorBase):
    """Provides a dialog box to edit a single label"""

    def __init__(self, canvas, target):
        """
        :param target: A reference to the label being edited
       """
        super(LabelEditor, self).__init__(canvas)
        self.target = target
        self.setWindowTitle("Edit label")
        self.editor = QLineEdit(self)
        self.editor.setText(target.get_text())
        self.layout.addWidget(self.editor)
        self.layout.addLayout(self.button_row)
        self.editor.show()

    def changes_accepted(self):
        self.target.set_text(self.editor.text())
