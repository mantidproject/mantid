# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, unicode_literals)

from qtpy import QtWidgets

from Muon.GUI.Common.checkbox import Checkbox


class LineSelectorView(QtWidgets.QWidget):
    def __init__(self, parent=None):
        super(LineSelectorView, self).__init__(parent)

        self.list = QtWidgets.QVBoxLayout()

        self.total = Checkbox("Plot Total")
        self.prompt = Checkbox("Plot Prompt")
        self.delayed = Checkbox("Plot Delayed")

        self.line_checkboxes = [self.total,
                                self.prompt,
                                self.delayed]
        for line_type in self.line_checkboxes:
            self.list.addWidget(line_type)
        self.setLayout(self.list)
