# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
import unittest

from unittest import mock
from mantidqt.widgets.workspacecalculator.model import WorkspaceCalculatorModel
from mantidqt.widgets.workspacecalculator.presenter import WorkspaceCalculator
from mantidqt.widgets.workspacecalculator.view import WorkspaceCalculatorView


class WorkspaceCalculatorTest(unittest.TestCase):

    def setUp(self):
        self.view = mock.Mock(spec=WorkspaceCalculatorView)
        self.view.lhs_scaling = mock.Mock(return_value=1)
        self.view.rhs_scaling = mock.Mock(return_value=0)
        self.view.lhs_ws = mock.Mock(return_value=None)
        self.view.rhs_ws = mock.Mock(return_value=None)
        self.view.pushButton = mock.Mock()

        self.model = mock.Mock(spec=WorkspaceCalculatorModel)
        self.model.validateInputs = mock.Mock(return_value=(False,False,["Input1 not valid",
                                                                         "Input2 not valid"]))

    def test_workspaceCalculator(self):

        presenter = WorkspaceCalculator(None, model=self.model, view=self.view)
        presenter.view.pushButton.click()
        # setup calls
        self.assertEqual(self.view.setValidationLabel.call_count, 2)
        self.assertEqual(self.model.call_count, 1)
        self.assertEqual(self.model.validateInputs.call_count, 1)
        self.assertEqual(self.model.performOperation.call_count, 1)

    def test_workspaceCalculator_validate_scaling(self):
        pass

    def test_workspaceCalculator_validate_lhs(self):
        pass

    def test_workspaceCalculator_validate_rhs(self):
        pass

    def test_workspaceCalculator_validate_inputs(self):
        pass


if __name__ == '__main__':
    unittest.main()
