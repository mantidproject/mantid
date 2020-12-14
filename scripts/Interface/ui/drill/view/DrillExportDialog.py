# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import os

from qtpy.QtWidgets import QDialog, QCheckBox
from qtpy import uic


class DrillExportDialog(QDialog):

    UI_FILENAME = "ui/export.ui"

    def __init__(self, parent=None):
        """
        Create the export dialog.

        Args:
            parent (QWidget): parent widget
        """
        super().__init__(parent)
        self.here = os.path.dirname(os.path.realpath(__file__))
        uic.loadUi(os.path.join(self.here, self.UI_FILENAME), self)
        self.okButton.clicked.connect(self.accept)
        self.cancelButton.clicked.connect(self.reject)
        self.applyButton.clicked.connect(
                lambda : self.accepted.emit()
                )
