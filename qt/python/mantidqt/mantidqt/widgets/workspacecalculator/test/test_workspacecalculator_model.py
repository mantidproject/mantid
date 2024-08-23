# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
from mantid.simpleapi import CloneWorkspace, CreateMDHistoWorkspace, CreateSingleValuedWorkspace, CreateSampleWorkspace, GroupWorkspaces
from mantid.api import mtd
from mantidqt.widgets.workspacecalculator.model import WorkspaceCalculatorModel

import unittest


class WorkspaceCalculatorModelTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        """Sets up all operations and supported workspace types for testing."""
        cls.operations = ["+", "-", "/", "*", "WM"]

        cls.single_value_lhs = "single_value_lhs"
        CreateSingleValuedWorkspace(DataValue=1, ErrorValue=0.1, OutputWorkspace=cls.single_value_lhs)
        cls.single_value_rhs = "single_value_rhs"
        CreateSingleValuedWorkspace(DataValue=2, ErrorValue=0.2, OutputWorkspace=cls.single_value_rhs)

        ws1 = CreateSampleWorkspace(NumEvents=2, XMax=100, BinWidth=10)
        ws2 = CreateSampleWorkspace(NumEvents=2, XMax=100, BinWidth=10)
        ws3 = CreateSampleWorkspace(NumEvents=2, XMax=100, BinWidth=10)
        ws4 = CreateSampleWorkspace(NumEvents=2, XMax=100, BinWidth=10)

        cls.matrix_lhs = "matrix_lhs"
        cls.matrix_rhs = "matrix_rhs"
        CloneWorkspace(InputWorkspace=ws1, OutputWorkspace=cls.matrix_lhs)
        CloneWorkspace(InputWorkspace=ws2, OutputWorkspace=cls.matrix_rhs)

        cls.matrix_group_lhs = "matrix_group_lhs"
        GroupWorkspaces(InputWorkspaces=[ws1, ws2], OutputWorkspace=cls.matrix_group_lhs)
        cls.matrix_group_rhs = "matrix_group_rhs"
        GroupWorkspaces(InputWorkspaces=[ws3, ws4], OutputWorkspace=cls.matrix_group_rhs)
        cls.matrix_group_larger = "matrix_group_large"
        lhs_group_clone = CloneWorkspace(InputWorkspace=cls.matrix_group_lhs)
        rhs_group_clone = CloneWorkspace(InputWorkspace=cls.matrix_group_rhs)
        GroupWorkspaces(InputWorkspaces=[lhs_group_clone, rhs_group_clone], OutputWorkspace=cls.matrix_group_larger)

        ws_event_1 = CreateSampleWorkspace(NumEvents=2, XMax=100, BinWidth=10, WorkspaceType="Event")
        ws_event_2 = CreateSampleWorkspace(NumEvents=2, XMax=100, BinWidth=10, WorkspaceType="Event")
        ws_event_3 = CreateSampleWorkspace(NumEvents=2, XMax=100, BinWidth=10, WorkspaceType="Event")
        ws_event_4 = CreateSampleWorkspace(NumEvents=2, XMax=100, BinWidth=10, WorkspaceType="Event")

        cls.event_lhs = "event_lhs"
        cls.event_rhs = "event_rhs"
        CloneWorkspace(InputWorkspace=ws_event_1, OutputWorkspace=cls.event_lhs)
        CloneWorkspace(InputWorkspace=ws_event_2, OutputWorkspace=cls.event_rhs)

        cls.event_group_lhs = "event_group_lhs"
        GroupWorkspaces(InputWorkspaces=[ws_event_1, ws_event_2], OutputWorkspace=cls.event_group_lhs)
        cls.event_group_rhs = "event_group_rhs"
        GroupWorkspaces(InputWorkspaces=[ws_event_3, ws_event_4], OutputWorkspace=cls.event_group_rhs)
        cls.event_group_larger = "event_group_large"
        lhs_group_clone = CloneWorkspace(InputWorkspace=cls.event_group_lhs)
        rhs_group_clone = CloneWorkspace(InputWorkspace=cls.event_group_rhs)
        GroupWorkspaces(InputWorkspaces=[lhs_group_clone, rhs_group_clone], OutputWorkspace=cls.event_group_larger)

        ws_matrix_group_1 = CloneWorkspace(InputWorkspace=ws1)
        ws_event_group_1 = CloneWorkspace(InputWorkspace=ws_event_1)
        cls.mixed_group = "matrix_event_mixed_group"
        GroupWorkspaces(InputWorkspaces=[ws_matrix_group_1, ws_event_group_1], OutputWorkspace=cls.mixed_group)

        S1 = range(0, 10)
        S2 = range(5, 15)
        ERR = range(0, 10)
        # create HistoMD workspace
        cls.md_lhs = "md_lhs"
        CreateMDHistoWorkspace(
            Dimensionality=2,
            Extents="-3,3,-10,10",
            SignalInput=S1,
            ErrorInput=ERR,
            NumberOfBins="2,5",
            Names="Dim1,Dim2",
            Units="MomentumTransfer,EnergyTransfer",
            OutputWorkspace=cls.md_lhs,
        )
        cls.md_rhs = "md_rhs"
        CreateMDHistoWorkspace(
            Dimensionality=2,
            Extents="-3,3,-10,10",
            SignalInput=S2,
            ErrorInput=ERR,
            NumberOfBins="2,5",
            Names="Dim1,Dim2",
            Units="MomentumTransfer,EnergyTransfer",
            OutputWorkspace=cls.md_rhs,
        )

        tmp_lhs = CloneWorkspace(InputWorkspace=cls.md_lhs)
        tmp_rhs = CloneWorkspace(InputWorkspace=cls.md_rhs)
        cls.md_group_lhs = "md_group_lhs"
        GroupWorkspaces(InputWorkspaces=[cls.md_lhs, tmp_lhs], OutputWorkspace=cls.md_group_lhs)
        cls.md_group_rhs = "md_group_rhs"
        GroupWorkspaces(InputWorkspaces=[cls.md_rhs, tmp_rhs], OutputWorkspace=cls.md_group_rhs)

        tmp_md_group_lhs = CloneWorkspace(InputWorkspace=cls.md_group_lhs)
        tmp_md_group_rhs = CloneWorkspace(InputWorkspace=cls.md_group_rhs)
        cls.md_group_larger = "md_group_large"
        GroupWorkspaces(InputWorkspaces=[tmp_md_group_lhs, tmp_md_group_rhs], OutputWorkspace=cls.md_group_larger)

        cls.md_mixed_group = "md_mixed_group"
        ws_matrix_group_2 = CloneWorkspace(InputWorkspace=cls.matrix_group_lhs)
        ws_md_group = CloneWorkspace(InputWorkspace=cls.md_group_lhs)
        GroupWorkspaces(InputWorkspaces=[ws_matrix_group_2, ws_md_group], OutputWorkspace=cls.md_mixed_group)

    @classmethod
    def tearDownClass(cls):
        mtd.clear()

    def check_validity(self, lhs_validation, rhs_validation, err_msg, passed):
        self.assertEqual(lhs_validation, passed)
        self.assertEqual(rhs_validation, passed)
        if passed:
            self.assertEqual(err_msg, str())
        else:
            self.assertTrue(len(err_msg) != 0)

    def test_default_model(self):
        """Tests the default model and whether the correct validity flags are set."""
        model = WorkspaceCalculatorModel()
        (
            valid_lhs,
            valid_rhs,
            _,
        ) = model.performOperation()
        self.assertEqual(valid_lhs, False)
        self.assertEqual(valid_rhs, False)

    def test_model_single(self):
        """Tests binary operations on SingleValuedWorkspace."""
        for operation in self.operations:
            output_ws = "test_model_single_{}".format(operation)
            model = WorkspaceCalculatorModel(
                lhs_ws=self.single_value_lhs, rhs_ws=self.single_value_rhs, output_ws=output_ws, operation=operation
            )
            valid_lhs, valid_rhs, err_msg = model.performOperation()
            self.assertTrue(mtd[output_ws])
            self.check_validity(lhs_validation=valid_lhs, rhs_validation=valid_rhs, err_msg=err_msg, passed=True)
            if operation == "+":
                self.assertEqual(mtd[output_ws].readY(0)[0], 3)
            elif operation == "-":
                self.assertEqual(mtd[output_ws].readY(0)[0], -1)
            elif operation == "*":
                self.assertEqual(mtd[output_ws].readY(0)[0], 2)
            elif operation == "/":
                self.assertAlmostEqual(mtd[output_ws].readY(0)[0], 0.500, delta=0.001)
            elif operation == "WM":
                self.assertAlmostEqual(mtd[output_ws].readY(0)[0], 1.200, delta=0.001)

    def test_model_single_scaling(self):
        """Tests SingleValuedWorkspace scaling by arbitrary float parameters."""
        lhs_scaling = -1.2452
        rhs_scaling = 1e-5
        output_ws = "test_model_single_scaling"
        model = WorkspaceCalculatorModel(
            lhs_scale=lhs_scaling,
            lhs_ws=self.single_value_lhs,
            rhs_scale=rhs_scaling,
            rhs_ws=self.single_value_rhs,
            output_ws=output_ws,
            operation="+",
        )
        valid_lhs, valid_rhs, err_msg = model.performOperation()
        self.assertTrue(mtd[output_ws])
        self.check_validity(lhs_validation=valid_lhs, rhs_validation=valid_rhs, err_msg=err_msg, passed=True)
        value = mtd[output_ws].readY(0)[0]
        self.assertAlmostEqual(value, -1.24518, delta=1e-5)

    def test_model_matrix(self):
        """Tests binary operations on MatrixWorkspaces."""
        for operation in self.operations:
            output_ws = "test_model_matrix_{}".format(operation)
            model = WorkspaceCalculatorModel(lhs_ws=self.matrix_lhs, rhs_ws=self.matrix_rhs, output_ws=output_ws, operation=operation)
            valid_lhs, valid_rhs, err_msg = model.performOperation()
            self.assertTrue(mtd[output_ws])
            self.check_validity(lhs_validation=valid_lhs, rhs_validation=valid_rhs, err_msg=err_msg, passed=True)

    def test_model_matrix_scaling(self):
        """Tests binary operations on MatrixWorkspaces with scaling by arbitrary floats."""
        lhs_scaling = 10.12452
        rhs_scaling = -1e2
        output_ws = "test_model_matrix_scaling"
        model = WorkspaceCalculatorModel(
            lhs_scale=lhs_scaling, lhs_ws=self.matrix_lhs, rhs_scale=rhs_scaling, rhs_ws=self.matrix_rhs, output_ws=output_ws, operation="+"
        )
        valid_lhs, valid_rhs, err_msg = model.performOperation()
        self.assertTrue(mtd[output_ws])
        self.check_validity(lhs_validation=valid_lhs, rhs_validation=valid_rhs, err_msg=err_msg, passed=True)
        value = mtd[output_ws].readY(0)[0]
        self.assertAlmostEqual(value, -26.9626, delta=1e-4)

    def test_model_matrix_same(self):
        """Tests binary operations on MatrixWorkspaces with having two of the same workspace."""
        lhs_scaling = 3
        rhs_scaling = 2
        output_ws = "test_model_matrix_scaling"
        model = WorkspaceCalculatorModel(
            lhs_scale=lhs_scaling, lhs_ws=self.matrix_lhs, rhs_scale=rhs_scaling, rhs_ws=self.matrix_lhs, output_ws=output_ws, operation="+"
        )
        valid_lhs, valid_rhs, err_msg = model.performOperation()
        self.assertTrue(mtd[output_ws])
        self.check_validity(lhs_validation=valid_lhs, rhs_validation=valid_rhs, err_msg=err_msg, passed=True)
        value = mtd[output_ws].readY(0)[0]
        self.assertEqual(value, 1.5)

    def test_model_matrix_groups(self):
        """Tests binary operations on equal size groups containing MatrixWorkspaces."""
        for operation in self.operations:
            output_ws = "test_model_matrix_groups_{}".format(operation)
            model = WorkspaceCalculatorModel(
                lhs_ws=self.matrix_group_lhs, rhs_ws=self.matrix_group_rhs, output_ws=output_ws, operation=operation
            )
            valid_lhs, valid_rhs, err_msg = model.performOperation()
            self.assertTrue(mtd[output_ws])
            self.check_validity(lhs_validation=valid_lhs, rhs_validation=valid_rhs, err_msg=err_msg, passed=True)

    def test_model_non_equal_matrix_groups(self):
        """Tests binary operations on non-equal size groups containing MatrixWorkspaces."""
        for operation in self.operations:
            output_ws = "test_model_matrix_groups_{}".format(operation)
            model = WorkspaceCalculatorModel(
                lhs_ws=self.matrix_group_lhs, rhs_ws=self.matrix_group_larger, output_ws=output_ws, operation=operation
            )
            valid_lhs, valid_rhs, err_msg = model.performOperation()
            passed = False
            self.check_validity(lhs_validation=valid_lhs, rhs_validation=valid_rhs, err_msg=err_msg, passed=passed)

    def test_model_event(self):
        """Tests binary operations on EventWorkspaces."""
        for operation in self.operations:
            output_ws = "test_model_event_{}".format(operation)
            model = WorkspaceCalculatorModel(lhs_ws=self.event_lhs, rhs_ws=self.event_rhs, output_ws=output_ws, operation=operation)
            valid_lhs, valid_rhs, err_msg = model.performOperation()
            self.assertTrue(mtd[output_ws])
            self.check_validity(lhs_validation=valid_lhs, rhs_validation=valid_rhs, err_msg=err_msg, passed=True)

    def test_model_event_groups(self):
        """Tests binary operations on equal-size groups containing EventWorkspaces."""
        for operation in self.operations:
            output_ws = "test_model_event_groups_{}".format(operation)
            model = WorkspaceCalculatorModel(
                lhs_ws=self.event_group_lhs, rhs_ws=self.event_group_rhs, output_ws=output_ws, operation=operation
            )
            valid_lhs, valid_rhs, err_msg = model.performOperation()
            self.assertTrue(mtd[output_ws])
            self.check_validity(lhs_validation=valid_lhs, rhs_validation=valid_rhs, err_msg=err_msg, passed=True)

    def test_model_md(self):
        """Tests for binary operations on MDHisto workspaces."""
        for operation in self.operations:
            output_ws = "test_model_md_{}".format(operation)
            model = WorkspaceCalculatorModel(lhs_ws=self.md_lhs, rhs_ws=self.md_rhs, output_ws=output_ws, operation=operation)
            valid_lhs, valid_rhs, err_msg = model.performOperation()
            self.check_validity(lhs_validation=valid_lhs, rhs_validation=valid_rhs, err_msg=err_msg, passed=True)
            self.assertTrue(mtd[output_ws])

    def test_model_md_groups(self):
        """Tests for binary operations with equal-size groups containing MDHisto workspaces."""
        for operation in self.operations:
            output_ws = "test_model_md_groups_{}".format(operation)
            model = WorkspaceCalculatorModel(lhs_ws=self.md_group_lhs, rhs_ws=self.md_group_rhs, output_ws=output_ws, operation=operation)
            valid_lhs, valid_rhs, err_msg = model.performOperation()
            self.check_validity(lhs_validation=valid_lhs, rhs_validation=valid_rhs, err_msg=err_msg, passed=True)
            self.assertTrue(mtd[output_ws])

    def test_model_validate_md_mixed(self):
        """Tests for validation of a group containing both Matrix and MDHisto workspaces."""
        for operation in self.operations:
            output_ws = "test_model_validate_md_mixed_{}".format(operation)
            model = WorkspaceCalculatorModel()
            valid_lhs, valid_rhs, err_msg = model.validateInputs(lhs_ws=self.md_mixed_group)
            passed = False
            self.check_validity(lhs_validation=valid_lhs, rhs_validation=valid_rhs, err_msg=err_msg, passed=passed)
            if passed:
                self.assertTrue(mtd[output_ws])

    def test_model_single_and_groups(self):
        """Tests for binary operations between SingleValuedWorkspace and a a group containing MatrixWorkspaces."""
        for operation in self.operations:
            output_ws = "test_model_single_and_groups_{}".format(operation)
            model = WorkspaceCalculatorModel(
                lhs_ws=self.matrix_group_lhs, rhs_ws=self.single_value_rhs, output_ws=output_ws, operation=operation
            )
            valid_lhs, valid_rhs, err_msg = model.performOperation()
            passed = True
            if operation == "WM":
                passed = False
            self.check_validity(lhs_validation=valid_lhs, rhs_validation=valid_rhs, err_msg=err_msg, passed=passed)
            if passed:
                self.assertTrue(mtd[output_ws])

    def test_model_single_and_md(self):
        """Tests for binary operations between SingleValuedWorkspace and a MDHisto workspace."""
        for operation in self.operations:
            output_ws = "test_model_single_and_md_{}".format(operation)
            model = WorkspaceCalculatorModel(lhs_ws=self.md_lhs, rhs_ws=self.single_value_rhs, output_ws=output_ws, operation=operation)
            valid_lhs, valid_rhs, err_msg = model.performOperation()
            passed = True
            if operation == "WM":
                passed = False
            self.check_validity(lhs_validation=valid_lhs, rhs_validation=valid_rhs, err_msg=err_msg, passed=passed)
            if passed:
                self.assertTrue(mtd[output_ws])

    def test_model_single_and_md_group(self):
        """Tests for binary operations between SingleValuedWorkspace and a group containing MDHisto workspaces."""
        for operation in self.operations:
            output_ws = "test_model_single_and_md_{}".format(operation)
            model = WorkspaceCalculatorModel(
                lhs_ws=self.md_group_lhs, rhs_ws=self.single_value_rhs, output_ws=output_ws, operation=operation
            )
            valid_lhs, valid_rhs, err_msg = model.performOperation()
            passed = True
            if operation == "WM":
                passed = False
            self.check_validity(lhs_validation=valid_lhs, rhs_validation=valid_rhs, err_msg=err_msg, passed=passed)
            if passed:
                self.assertTrue(mtd[output_ws])


if __name__ == "__main__":
    unittest.main()
