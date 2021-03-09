# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#

from .model import WorkspaceCalculatorModel
from .view import WorkspaceCalculatorView
from mantid.kernel import logger as log


class WorkspaceCalculator():
    """Class containing the presenter for the workspace calculator,
     that services the signals from the view and calls the model to perform
     the requested operation."""

    def __init__(self, parent=None, model=None, view=None):
        # Create model and view, or accept mocked versions
        self.model = model if model else WorkspaceCalculatorModel()
        self.view = view if view else WorkspaceCalculatorView(parent)

        self.lhs_scale = 1.0
        self.lhs_ws = None
        self.rhs_scale = 1.0
        self.rhs_ws = None
        self.operation = None
        self.output_ws = None

        self.view.pushButton.clicked.connect(self.onPressedGo)
        self.view.lhs_scaling.returnPressed.connect(self.onPressedGo)
        self.view.rhs_scaling.returnPressed.connect(self.onPressedGo)

    def readParameters(self):
        try:
            self.lhs_scale = float(self.view.lhs_scaling.text())
            self.rhs_scale = float(self.view.rhs_scaling.text())
        except ValueError:
            log.error("At least one of the scaling factors is not a valid number.")
            return False
        self.lhs_ws = self.view.lhs_ws.currentText()
        self.rhs_ws = self.view.rhs_ws.currentText()
        self.operation = self.view.operation.currentText()
        self.output_ws = self.view.output_ws.currentText()
        if self.output_ws == str():
            self.output_ws = "output"
        return True

    def onPressedGo(self):
        if self.readParameters():
            self.model = WorkspaceCalculatorModel(lhs_scale=self.lhs_scale,
                                                  lhs_ws=self.lhs_ws,
                                                  rhs_scale=self.rhs_scale,
                                                  rhs_ws=self.rhs_ws,
                                                  output_ws=self.output_ws,
                                                  operation=self.operation)
            self.model.performOperation()
