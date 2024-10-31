# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.simpleapi import *
from mantid.api import WorkspaceGroup, ITableWorkspace


class IqtFitSequentialTest(unittest.TestCase):
    _iqt_ws = None
    _function = r"name=LinearBackground,A0=0,A1=0,ties=(A1=0);name=ExpDecay,Height=1,Lifetime=0.0247558;ties=(f1.Height=1-f0.A0)"

    def setUp(self):
        self._iqt_ws = Load(Filename="iris26176_graphite002_iqt.nxs", OutputWorkspace="iris26176_graphite002_iqt")

    # -----------------------------------Validation of result-------------------------------------

    def _validate_output(self, params, result, fit_group):
        self.assertTrue(isinstance(params, ITableWorkspace))
        self.assertTrue(isinstance(result, WorkspaceGroup))
        self.assertTrue(isinstance(fit_group, WorkspaceGroup))

        self._validate_table_shape(params)
        self._validate_matrix_shape(result.getItem(0))
        self._validate_group_shape(fit_group)

        self._validate_table_values(params)
        self._validate_matrix_values(result.getItem(0))
        self._validate_group_values(fit_group)

        self._validate_sample_log_values(result.getItem(0))
        self._validate_sample_log_values(fit_group.getItem(0))

    def _validate_table_shape(self, tableWS):
        # Check length of rows and columns
        rows = tableWS.rowCount()
        columns = tableWS.columnCount()
        self.assertEqual(rows, 17)
        self.assertEqual(columns, 10)

        # Check some column names
        column_names = tableWS.getColumnNames()
        self.assertEqual("axis-1", column_names[0])
        self.assertEqual("f0.A0", column_names[1])
        self.assertEqual("f0.A0_Err", column_names[2])

    def _validate_matrix_shape(self, matrixWS):
        # Check no. bins and no. hists
        nbins = matrixWS.blocksize()
        nhists = matrixWS.getNumberHistograms()
        self.assertEqual(nbins, 17)
        self.assertEqual(nhists, 5)

        # Check histogram names
        text_axis = matrixWS.getAxis(1)
        self.assertTrue(text_axis.isText())
        self.assertEqual("f0.A0", text_axis.label(0))
        self.assertEqual("f0.A1", text_axis.label(1))
        self.assertEqual("f1.Height", text_axis.label(2))
        self.assertEqual("f1.Lifetime", text_axis.label(3))
        self.assertEqual("Chi_squared", text_axis.label(4))

        # Check bin units
        self.assertEqual("MomentumTransfer", matrixWS.getAxis(0).getUnit().unitID())

    def _validate_group_shape(self, groupWS):
        # Check number of workspaces and size
        nitems = groupWS.getNumberOfEntries()
        self.assertEqual(nitems, 17)
        sub_ws = groupWS.getItem(0)
        nbins = sub_ws.blocksize()
        nhists = sub_ws.getNumberHistograms()
        self.assertEqual(nbins, 59)
        self.assertEqual(nhists, 3)

        # Check histogram names
        text_axis = sub_ws.getAxis(1)
        self.assertTrue(text_axis.isText())
        self.assertEqual("Data", text_axis.label(0))
        self.assertEqual("Calc", text_axis.label(1))
        self.assertEqual("Diff", text_axis.label(2))

        # Check bin units
        self.assertEqual("ns", str(sub_ws.getAxis(0).getUnit().symbol()))

    def _validate_table_values(self, tableWS):
        # Check column data
        column = tableWS.column(0)
        self.assertEqual(round(column[0], 6), 0.483619)
        self.assertEqual(round(column[1], 6), 0.607871)
        self.assertEqual(round(column[-1], 5), 1.84519)

        # Check row data
        row = tableWS.row(0)
        self.assertEqual(round(row["axis-1"], 6), 0.483619)
        self.assertEqual(round(row["f1.Height"], 6), 0.966344)
        self.assertEqual(round(row["f1.Lifetime"], 7), 0.0287491)

    def _validate_matrix_values(self, matrixWS):
        # Check f0.A0
        a0 = matrixWS.readY(0)
        self.assertEqual(round(a0[0], 7), 0.0336564)
        self.assertEqual(round(a0[-1], 7), 0.0182411)

        # Check f1.Height
        height = matrixWS.readY(2)
        self.assertEqual(round(height[0], 6), 0.966344)
        self.assertEqual(round(height[-1], 6), 0.981759)

        # Check f1.Lifetime
        lifetime = matrixWS.readY(3)
        self.assertEqual(round(lifetime[0], 7), 0.0287491)
        self.assertEqual(round(lifetime[-1], 7), 0.0034427)

    def _validate_group_values(self, groupWS):
        sub_ws = groupWS.getItem(0)
        # Check Data
        data = sub_ws.readY(0)
        self.assertEqual(round(data[0], 6), 1.0)
        self.assertEqual(round(data[-1], 6), 0.039044)
        # Check Calc
        calc = sub_ws.readY(1)
        self.assertEqual(round(calc[0], 6), 1.0)
        self.assertEqual(round(calc[-1], 6), 0.033886)
        # Check Diff
        diff = sub_ws.readY(2)
        self.assertEqual(round(diff[0], 6), 0.0)
        self.assertEqual(round(diff[-1], 6), 0.005157)

    def _validate_sample_log_values(self, matrixWS):
        run = matrixWS.getRun()
        # Check additionally added logs
        self.assertEqual(float(run.getProperty("end_x").value), 0.24)
        self.assertEqual(float(run.getProperty("start_x").value), 0.0)

        # Check copied logs from input
        self.assertEqual(run.getProperty("current_period").value, 1)
        self.assertEqual(run.getProperty("iqt_resolution_workspace").value, "iris26173_graphite002_res")
        self.assertEqual(run.getProperty("iqt_sample_workspace").value, "iris26176_graphite002_red")

    # ---------------------------------------Success cases--------------------------------------

    def test_basic(self):
        """
        Tests a basic run of IqtfitSequential.
        """
        result, params, fit_group = IqtFitSequential(
            InputWorkspace=self._iqt_ws, Function=self._function, StartX="0.0", EndX="0.24", SpecMin=0, SpecMax=16
        )
        self._validate_output(params, result, fit_group)

    # ----------------------------------------Failure cases-------------------------------------

    def test_minimum_spectra_number_less_than_0(self):
        self.assertRaises(
            ValueError,
            IqtFitSequential,
            InputWorkspace=self._iqt_ws,
            Function=self._function,
            startX="0.0",
            EndX="0.2",
            SpecMin=-1,
            SpecMax=16,
            OutputWorkspace="result",
            OutputParameterWorkspace="table",
            OutputWorkspaceGroup="fit_group",
        )

    def test_maximum_spectra_more_than_workspace_spectra(self):
        self.assertRaises(
            RuntimeError,
            IqtFitSequential,
            InputWorkspace=self._iqt_ws,
            Function=self._function,
            startX="0.0",
            EndX="0.2",
            SpecMin=0,
            SpecMax=20,
            OutputWorkspace="result",
            OutputParameterWorkspace="table",
            OutputWorkspaceGroup="fit_group",
        )

    def test_minimum_spectra_more_than_maximum_spectra(self):
        self.assertRaisesRegex(
            RuntimeError,
            "SpecMin must be less than or equal to SpecMax.",
            IqtFitSequential,
            InputWorkspace=self._iqt_ws,
            Function=self._function,
            startX="0.0",
            EndX="0.2",
            SpecMin=10,
            SpecMax=5,
            OutputWorkspace="result",
            OutputParameterWorkspace="table",
            OutputWorkspaceGroup="fit_group",
        )

    def test_minimum_x_less_than_0(self):
        self.assertRaisesRegex(
            RuntimeError,
            "StartX must be greater than or equal to 0.",
            IqtFitSequential,
            InputWorkspace=self._iqt_ws,
            Function=self._function,
            StartX="-0.2",
            EndX="0.2",
            SpecMin=0,
            SpecMax=16,
            OutputWorkspace="result",
            OutputParameterWorkspace="table",
            OutputWorkspaceGroup="fit_group",
        )

    def test_maximum_x_more_than_workspace_max_x(self):
        self.assertRaises(
            RuntimeError,
            IqtFitSequential,
            InputWorkspace=self._iqt_ws,
            Function=self._function,
            StartX="0",
            EndX="0.4",
            SpecMin=0,
            SpecMax=16,
            OutputWorkspace="result",
            OutputParameterWorkspace="table",
            OutputWorkspaceGroup="fit_group",
        )


if __name__ == "__main__":
    unittest.main()
