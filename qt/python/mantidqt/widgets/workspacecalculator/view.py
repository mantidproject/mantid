# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package

from qtpy.QtCore import Qt
from qtpy.QtWidgets import QWidget
from mantidqt.utils.qt import load_ui


class WorkspaceCalculatorView(QWidget):
    """The view for the workspace calculator widget."""

    def __init__(self, parent=None):
        super().__init__(parent)

        self.ui = load_ui(__file__, 'workspace_calculator.ui', baseinstance=self)

        self.setAttribute(Qt.WA_DeleteOnClose, True)

    def closeEvent(self, event):
        self.deleteLater()
        super(WorkspaceCalculatorView, self).closeEvent(event)
