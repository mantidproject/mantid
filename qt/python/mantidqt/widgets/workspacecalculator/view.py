# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package

from qtpy.QtCore import Qt
from qtpy import QtGui
from qtpy.QtWidgets import QWidget
from mantidqt.utils.qt import load_ui


class WorkspaceCalculatorView(QWidget):
    """The view for the workspace calculator widget."""

    def __init__(self, parent=None):
        super().__init__(parent)

        self.ui = load_ui(__file__, 'workspace_calculator.ui', baseinstance=self)

        self.setAttribute(Qt.WA_DeleteOnClose, True)

        scale_validator = ScaleValidator()
        self.lhs_scaling.setValidator(scale_validator)
        self.rhs_scaling.setValidator(scale_validator)

    def setValidationLabel(self, ws, validationValue, tooltip=""):
        if ws == "LHS":
            if isinstance(tooltip, list):
                tooltip = tooltip[0]
            self.label_validation_lhs.setVisible(not validationValue)
            self.label_validation_lhs.setToolTip(tooltip)
        else:
            if isinstance(tooltip, list):
                tooltip = tooltip[1]
            self.label_validation_rhs.setVisible(not validationValue)
            self.label_validation_rhs.setToolTip(tooltip)

    def closeEvent(self, event):
        self.deleteLater()
        super(WorkspaceCalculatorView, self).closeEvent(event)


class ScaleValidator(QtGui.QValidator):
    def __init__(self, parent=None):
        QtGui.QValidator.__init__(self, parent=parent)

    def validate(self, input, pos):
        try:
            float(input)
        except ValueError:
            if (input[-1].lower() == "e"
                    or input[-2].lower() == "e" and input[-1].lower() == "-"):
                return QtGui.QValidator.Acceptable, input, pos
            return QtGui.QValidator.Invalid, input, pos
        if float(input) == 0:
            return QtGui.QValidator.Intermediate, input, pos
        return QtGui.QValidator.Acceptable, input, pos

    def fixup(self, input):
        return "1.0"
