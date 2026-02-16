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


class WorkspaceCalculator:
    """Class containing the presenter for the workspace calculator,
    that services the signals from the view and calls the model to perform
    the requested operation."""

    def __init__(self, parent=None, model=None, view=None):
        # Create model and view, or accept mocked versions
        self.model = model or WorkspaceCalculatorModel()
        self.view = view or WorkspaceCalculatorView(parent)

        self.view.pushButton.clicked.connect(self.onPressedGo)
        self.view.lhs_scaling.returnPressed.connect(self.onPressedGo)
        self.view.rhs_scaling.returnPressed.connect(self.onPressedGo)
        # validate inputs when workspace is selected, or the operation changed
        self.view.lhs_ws.currentIndexChanged.connect(self.validateInputs)
        self.view.rhs_ws.currentIndexChanged.connect(self.validateInputs)
        self.view.operation.currentIndexChanged.connect(self.validateInputs)

    def readParameters(self):
        parameters = dict()
        try:
            parameters["lhs_scale"] = float(self.view.lhs_scaling.text())
            parameters["rhs_scale"] = float(self.view.rhs_scaling.text())
        except ValueError:
            return
        parameters["lhs_ws"] = self.view.lhs_ws.currentText()
        parameters["rhs_ws"] = self.view.rhs_ws.currentText()
        parameters["operation"] = self.view.operation.currentText()
        output_ws = self.view.output_ws.currentText()
        if output_ws == str():
            output_ws = "output"
        parameters["output_ws"] = output_ws
        return parameters

    def validateInputs(self):
        parameters = self.readParameters()
        if parameters == dict():
            return
        lhs_ws = parameters["lhs_ws"] if parameters["lhs_ws"] != str() else None
        rhs_ws = parameters["rhs_ws"] if parameters["rhs_ws"] != str() else None
        valid_lhs, valid_rhs, err_msg = self.model.validateInputs(lhs_ws=lhs_ws, rhs_ws=rhs_ws, operation=parameters["operation"])
        if lhs_ws:
            self.view.setValidationLabel(ws="LHS", validationValue=valid_lhs, tooltip=err_msg)
        if rhs_ws:
            self.view.setValidationLabel(ws="RHS", validationValue=valid_rhs, tooltip=err_msg)

    def onPressedGo(self):
        parameters = self.readParameters()
        if parameters == dict():
            return
        valid_lhs, valid_rhs, err_msg = self.model.updateParameters(**parameters)
        self.view.setValidationLabel(ws="LHS", validationValue=valid_lhs, tooltip=err_msg)
        self.view.setValidationLabel(ws="RHS", validationValue=valid_rhs, tooltip=err_msg)
        if valid_lhs and valid_rhs:
            valid_lhs, valid_rhs, err_msg = self.model.performOperation()
            if valid_lhs != self.view.label_validation_lhs.isVisible():
                self.view.setValidationLabel(ws="LHS", validationValue=valid_lhs, tooltip=err_msg)
            if valid_rhs != self.view.label_validation_rhs.isVisible():
                self.view.setValidationLabel(ws="RHS", validationValue=valid_rhs, tooltip=err_msg)
