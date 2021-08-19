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

        self.label_validation_lhs.setVisible(False)
        self.label_validation_rhs.setVisible(False)

        scale_validator = ScaleValidator()
        self.lhs_scaling.setValidator(scale_validator)
        self.rhs_scaling.setValidator(scale_validator)

        # setting these selectors to be optional allows to have an empty entry
        self.lhs_ws.setOptional(True)
        self.rhs_ws.setOptional(True)
        self.output_ws.setOptional(True)

        # connecting ADS observers
        self.lhs_ws.focussed.connect(lambda: self.connectADS('lhs'))
        self.rhs_ws.focussed.connect(lambda: self.connectADS('rhs'))
        self.output_ws.focussed.connect(lambda: self.connectADS('output'))

        # cases for disconnecting ADS observers
        self.lhs_ws.activated.connect(lambda: self.disconnectADS('lhs'))
        self.rhs_ws.activated.connect(lambda: self.disconnectADS('rhs'))
        self.output_ws.activated.connect(lambda: self.disconnectADS('output'))

        # by default the observers to the ADS should be disconnected, and connected only when user focuses on the widget
        self.lhs_ws.disconnectObservers()
        self.rhs_ws.disconnectObservers()
        self.output_ws.disconnectObservers()

    def setValidationLabel(self, ws, validationValue, tooltip=""):
        """Sets the visibility of the validity indicator (asterisk) next to the workspace selector."""
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

    def connectADS(self, selector_name):
        """Explicitly connects the workspace selector observers to the ADS."""
        if selector_name == "lhs" and not self.lhs_ws.isConnected():
            self.lhs_ws.connectObservers()
        elif selector_name == "rhs" and not self.rhs_ws.isConnected():
            self.rhs_ws.connectObservers()
        elif selector_name == "output" and not self.output_ws.isConnected():
            self.output_ws.connectObservers()

    def disconnectADS(self, selector_name = ""):
        """Disconnects connected workspace selectors from signals coming from the ADS."""
        if selector_name == "lhs":
            self.lhs_ws.disconnectObservers()
        elif selector_name == "rhs":
            self.rhs_ws.disconnectObservers()
        elif selector_name == "output":
            self.output_ws.disconnectObservers()
        else:
            self.lhs_ws.disconnectObservers()
            self.rhs_ws.disconnectObservers()
            self.output_ws.disconnectObservers()

    def hideEvent(self, event):
        """Handles hide event of the calculator widget."""
        self.disconnectADS()
        super(WorkspaceCalculatorView, self).hideEvent(event)

    def closeEvent(self, event):
        """Handles close event of the calculator widget."""
        self.deleteLater()
        super(WorkspaceCalculatorView, self).closeEvent(event)


class ScaleValidator(QtGui.QValidator):
    """Validator for QLineEdit input used for scaling of LHS and RHS workspaces."""

    def __init__(self, parent=None):
        QtGui.QValidator.__init__(self, parent=parent)

    def validate(self, input, pos):
        try:
            float(input)
        except ValueError:
            try:
                last_char = input[-1].lower()
                penultimate_char = input[-2].lower()
                if last_char == "e":
                    try:
                        int(penultimate_char)
                        return QtGui.QValidator.Acceptable, input, pos
                    except ValueError:
                        pass
                elif penultimate_char == "e" and last_char == "-":
                    return QtGui.QValidator.Acceptable, input, pos
            except IndexError:
                pass
            return QtGui.QValidator.Invalid, input, pos
        if float(input) == 0:
            return QtGui.QValidator.Intermediate, input, pos
        return QtGui.QValidator.Acceptable, input, pos

    def fixup(self, input):
        return "1.0"
