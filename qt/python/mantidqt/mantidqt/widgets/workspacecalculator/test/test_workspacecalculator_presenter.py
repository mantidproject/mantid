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
from mantidqt.widgets.workspacecalculator.presenter import WorkspaceCalculator
from qtpy.QtCore import Qt
from qtpy.QtTest import QTest
from mantidqt.utils.qt.testing import start_qapplication
from mantid.simpleapi import CreateSingleValuedWorkspace


@start_qapplication
class WorkspaceCalculatorTest(unittest.TestCase):

    def setUp(self):
        patch = mock.patch('mantidqt.widgets.workspacecalculator.view')
        self.view = patch.start()
        self.addCleanup(patch.stop)
        self.view.lhs_ws.currentText = mock.Mock(return_value="ws1")
        self.view.rhs_ws.currentText = mock.Mock(return_value="ws2")
        self.view.operation.currentText = mock.Mock(return_value="*")
        self.view.output_ws.currentText = mock.Mock(return_value="output")

        patch_model = mock.patch('mantidqt.widgets.workspacecalculator.model')
        self.model = patch_model.start()
        self.addCleanup(patch_model.stop)
        self.model.updateParameters = mock.Mock(return_value=(False, False,
                                                              ["Input1: failed during update",
                                                               "Input2: failed during update"]))
        self.model.validateInputs = mock.Mock(return_value=(False, False,
                                                            ["Input1: not valid",
                                                             "Input2 not valid"]))
        self.model.performOperation = mock.Mock(return_value=(False, False,
                                                              ["Input1: execution failed",
                                                               "Input2: execution failed"]))

        self.lhs_ws = 'lhs_ws'
        CreateSingleValuedWorkspace(DataValue=1.0, OutputWorkspace=self.lhs_ws)
        self.rhs_ws = 'rhs_ws'
        CreateSingleValuedWorkspace(DataValue=2.0, OutputWorkspace=self.rhs_ws)

    def test_workspaceCalculator_onPressedGo_update_fail(self):
        """This tests signals when both inputs don't go through validation."""
        presenter = WorkspaceCalculator(None, model=self.model, view=self.view)
        presenter.onPressedGo()
        # check calls
        presenter.model.updateParameters.assert_called_with(lhs_scale=1.0, lhs_ws='ws1',
                                                            operation='*', output_ws='output',
                                                            rhs_scale=1.0, rhs_ws='ws2')
        self.assertEqual(presenter.model.updateParameters.call_count, 1)
        self.assertEqual(presenter.model.validateInputs.call_count, 0)
        self.assertEqual(presenter.model.performOperation.call_count, 0)
        self.assertEqual(presenter.view.setValidationLabel.call_count, 2)
        presenter.view.setValidationLabel.\
            assert_called_with(tooltip=['Input1: failed during update',
                                        'Input2: failed during update'],
                               validationValue=False, ws='RHS')
        # here one can only catch the second call, thus ws is RHS, while both fail

    def test_workspaceCalculator_onPressedGo_execution_fail(self):
        """This tests signals when updateParameters (and validation in the updateParameters)
         succeeded but the execution failed."""
        self.model.updateParameters = mock.Mock(return_value=(True, True,
                                                              ["", ""]))
        presenter = WorkspaceCalculator(None, model=self.model, view=self.view)
        presenter.onPressedGo()
        # check calls
        presenter.model.updateParameters.assert_called_with(lhs_scale=1.0, lhs_ws='ws1',
                                                            operation='*', output_ws='output',
                                                            rhs_scale=1.0, rhs_ws='ws2')
        self.assertEqual(presenter.model.updateParameters.call_count, 1)
        self.assertEqual(presenter.model.validateInputs.call_count, 0)
        self.assertEqual(presenter.model.performOperation.call_count, 1)
        self.assertEqual(presenter.view.setValidationLabel.call_count, 4)
        presenter.view.setValidationLabel.\
            assert_called_with(tooltip=['Input1: execution failed',
                                        'Input2: execution failed'],
                               validationValue=False, ws='RHS')
        # here one can only catch the second call, thus ws is RHS, while both fail

    def test_workspaceCalculator_onPressedGo_success(self):
        """This tests signals when updateParameters (and validation in the updateParameters)
         succeeded but the execution failed."""
        self.model.updateParameters = mock.Mock(return_value=(True, True,
                                                              ["", ""]))
        self.model.performOperation = mock.Mock(return_value=(True, True,
                                                              ["", ""]))
        presenter = WorkspaceCalculator(None, model=self.model, view=self.view)
        presenter.onPressedGo()
        # check calls
        presenter.model.updateParameters.assert_called_with(lhs_scale=1.0, lhs_ws='ws1',
                                                            operation='*', output_ws='output',
                                                            rhs_scale=1.0, rhs_ws='ws2')
        self.assertEqual(presenter.model.updateParameters.call_count, 1)
        self.assertEqual(presenter.model.validateInputs.call_count, 0)
        self.assertEqual(presenter.model.performOperation.call_count, 1)
        self.assertEqual(presenter.view.setValidationLabel.call_count, 4)
        presenter.view.setValidationLabel.\
            assert_called_with(tooltip=['',''],validationValue=True, ws='RHS')
        # here one can only catch the second call, thus ws is RHS, while both succeeded

    def test_workspaceCalculator_enterPressed_lhs_scaling(self):
        presenter = WorkspaceCalculator(None, model=self.model)
        QTest.keyClick(presenter.view.lhs_scaling, Qt.Key_Return)
        # check calls
        self.assertEqual(presenter.model.updateParameters.call_count, 1)
        self.assertEqual(presenter.model.validateInputs.call_count, 0)
        self.assertEqual(presenter.model.performOperation.call_count, 0)

    def test_workspaceCalculator_enterPressed_rhs_scaling(self):
        presenter = WorkspaceCalculator(None, model=self.model)
        QTest.keyClick(presenter.view.rhs_scaling, Qt.Key_Return)
        # check calls
        self.assertEqual(presenter.model.updateParameters.call_count, 1)
        self.assertEqual(presenter.model.validateInputs.call_count, 0)
        self.assertEqual(presenter.model.performOperation.call_count, 0)

    def test_workspaceCalculator_lhs_validation(self):
        presenter = WorkspaceCalculator(None, model=self.model)
        presenter.view.lhs_ws.setCurrentIndex(0)
        presenter.view.lhs_ws.setCurrentIndex(1)
        # check calls
        self.assertEqual(presenter.model.updateParameters.call_count, 0)
        self.assertEqual(presenter.model.validateInputs.call_count, 1)
        self.assertEqual(presenter.model.performOperation.call_count, 0)

    def test_workspaceCalculator_rhs_validation(self):
        presenter = WorkspaceCalculator(None, model=self.model)
        presenter.view.rhs_ws.setCurrentIndex(0)
        presenter.view.rhs_ws.setCurrentIndex(1)
        # check calls
        self.assertEqual(presenter.model.updateParameters.call_count, 0)
        self.assertEqual(presenter.model.validateInputs.call_count, 1)
        self.assertEqual(presenter.model.performOperation.call_count, 0)


if __name__ == '__main__':
    unittest.main()
