# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=too-many-public-methods,invalid-name,no-init

"""
Unit test for Vesuvio corrections steps

Assumes that mantid can be imported and the data paths
are configured to find the Vesuvio data
"""

import systemtesting
import numpy as np

from mantid.api import *
import mantid.simpleapi as ms
from mantid import *

# ====================================Helper Functions=======================================


def setup():
    test_ws = ms.LoadVesuvio(Filename="15039-15045", InstrumentParFile="IP0004_10.par", Mode="SingleDifference", SpectrumList="135-136")
    test_container_ws = ms.LoadVesuvio(Filename="15036", InstrumentParFile="IP0004_10.par", Mode="SingleDifference", SpectrumList="135-136")

    return test_ws, test_container_ws


def setup_back_scattering():
    test_ws = ms.LoadVesuvio(Filename="15039-15045", InstrumentParFile="IP0004_10.par", Mode="SingleDifference", SpectrumList="3-6")
    test_container_ws = ms.LoadVesuvio(Filename="15036", InstrumentParFile="IP0004_10.par", Mode="SingleDifference", SpectrumList="3-6")

    return test_ws, test_container_ws


def tear_down():
    workspace_names = ["__Correction", "__Corrected", "__Output", "__LinearFit"]
    for name in workspace_names:
        if mtd.doesExist(name):
            mtd.remove(name)


def _create_algorithm(**kwargs):
    alg = AlgorithmManager.createUnmanaged("VesuvioCorrections")
    alg.initialize()
    alg.setChild(True)
    alg.setProperty("OutputWorkspace", "__Output")
    alg.setProperty("CorrectionWorkspaces", "__Correction")
    alg.setProperty("CorrectedWorkspaces", "__Corrected")
    alg.setProperty("LinearFitResult", "__LinearFit")
    for key, value in kwargs.items():
        alg.setProperty(key, value)
    return alg


def _create_dummy_fit_parameters_ws_index_1():
    params = ms.CreateEmptyTableWorkspace(OutputWorkspace="__VesuvioCorrections_test_fit_params")

    params.addColumn("str", "Name")
    params.addColumn("float", "Value")
    params.addColumn("float", "Error")

    params.addRow(["f0.Mass", 1.0079, 0.0])
    params.addRow(["f0.Width", 3.70295, 0.199336])
    params.addRow(["f0.FSECoeff", 0.436396, 0])
    params.addRow(["f0.C_0", 21.2616, 0.750185])
    params.addRow(["f1.Mass", 16.0, 0.0])
    params.addRow(["f1.Width", 10.0, 0.0])
    params.addRow(["f1.Intensity", 4.03064, 0.41762])
    params.addRow(["f2.Mass", 27.0, 0.0])
    params.addRow(["f2.Width", 13.0, 0.0])
    params.addRow(["f2.Intensity", 3.23823, 0.447593])
    params.addRow(["f3.Mass", 133.0, 0.0])
    params.addRow(["f3.Width", 30.0, 0.0])
    params.addRow(["f3.Intensity", 0.882613, 0.218913])
    params.addRow(["Cost function value", 3.19573, 0.0])

    return params


def _create_dummy_fit_parameters_ws_index_2():
    params = ms.CreateEmptyTableWorkspace(OutputWorkspace="__VesuvioCorrections_test_fit_params")

    params.addColumn("str", "Name")
    params.addColumn("float", "Value")
    params.addColumn("float", "Error")

    params.addRow(["f0.Mass", 1.0079, 0.0])
    params.addRow(["f0.Width", 4.34424, 0.205241])
    params.addRow(["f0.FSECoeff", 0.511974, 0])
    params.addRow(["f0.C_0", 21.6463, 0.726012])
    params.addRow(["f1.Mass", 16.0, 0.0])
    params.addRow(["f1.Width", 10.0, 0.0])
    params.addRow(["f1.Intensity", 4.49586, 0.438959])
    params.addRow(["f2.Mass", 27.0, 0.0])
    params.addRow(["f2.Width", 13.0, 0.0])
    params.addRow(["f2.Intensity", 2.60706, 0.482777])
    params.addRow(["f3.Mass", 133.0, 0.0])
    params.addRow(["f3.Width", 30.0, 0.0])
    params.addRow(["f3.Intensity", 1.11099, 0.232988])
    params.addRow(["Cost function value", 2.98231, 0.0])

    return params


def _create_dummy_fit_parameters_no_hydrogen():
    params = ms.CreateEmptyTableWorkspace(OutputWorkspace="__VesuvioCorrections_test_fit_params")

    params.addColumn("str", "Name")
    params.addColumn("float", "Value")
    params.addColumn("float", "Error")

    params.addRow(["f0.Mass", 16.0, 0.0])
    params.addRow(["f0.Width", 10.0, 0.0])
    params.addRow(["f0.Intensity", 4.03064, 0.41762])
    params.addRow(["f1.Mass", 27.0, 0.0])
    params.addRow(["f1.Width", 13.0, 0.0])
    params.addRow(["f1.Intensity", 3.23823, 0.447593])
    params.addRow(["f2.Mass", 133.0, 0.0])
    params.addRow(["f2.Width", 30.0, 0.0])
    params.addRow(["f2.Intensity", 0.882613, 0.218913])
    params.addRow(["Cost function value", 3.19573, 0.0])

    return params


def _create_dummy_masses():
    return [1.0079, 16.0, 27.0, 133.0]


def _create_dummy_profiles():
    return (
        "function=GramCharlier,hermite_coeffs=[1, 0, 0],k_free=0,sears_flag=1,"
        + "width=[2, 5, 7];function=Gaussian,width=10;function=Gaussian,width=13;"
        + "function=Gaussian,width=30"
    )


# ===========================================================================================
# ========================================Success cases======================================


class TestGammaAndMsCorrectWorkspaceIndexOne(systemtesting.MantidSystemTest):
    _algorithm = None
    _is_linux = None
    _is_rhel6 = None
    _input_bins = None

    def runTest(self):
        test_ws, _ = setup()
        self._input_bins = test_ws.blocksize()
        self._algorithm = _create_algorithm(
            InputWorkspace=test_ws,
            GammaBackground=True,
            FitParameters=_create_dummy_fit_parameters_ws_index_1(),
            Masses=_create_dummy_masses(),
            MassProfiles=_create_dummy_profiles(),
        )

        self._algorithm.execute()

    def validate(self):
        self.assertTrue(self._algorithm.isExecuted())
        # Test Corrections Workspaces
        corrections_wsg = self._algorithm.getProperty("CorrectionWorkspaces").value
        _validate_group_structure(self, corrections_wsg, 3)
        corrections_gb_peak = 0.013566
        corrections_ts_peak = 0.157938
        corrections_ms_peak = 0.000200
        corrections_ts_bin = 726
        corrections_ms_bin = 709

        _validate_matrix_peak_height(self, corrections_wsg.getItem(0), corrections_gb_peak, 458)
        _validate_matrix_peak_height(self, corrections_wsg.getItem(1), corrections_ts_peak, corrections_ts_bin)
        _validate_matrix_peak_height(self, corrections_wsg.getItem(2), corrections_ms_peak, corrections_ms_bin)

        # Test Corrected Workspaces
        corrected_wsg = self._algorithm.getProperty("CorrectedWorkspaces").value
        _validate_group_structure(self, corrected_wsg, 3)
        corrected_gb_peak = 0.63430494091
        corrected_ts_peak = 0.58823622842
        corrected_ms_peak = 0.62668268027
        _validate_matrix_peak_height(self, corrected_wsg.getItem(0), corrected_gb_peak, 325)
        _validate_matrix_peak_height(self, corrected_wsg.getItem(1), corrected_ts_peak, 220)
        _validate_matrix_peak_height(self, corrected_wsg.getItem(2), corrected_ms_peak, 325)

        # Test OutputWorkspace
        output_ws = self._algorithm.getProperty("OutputWorkspace").value
        _validate_matrix_structure(self, output_ws, 1, self._input_bins)
        output_expected_peak = 0.634264352261
        _validate_matrix_peak_height(self, output_ws, output_expected_peak, 325)

        # Test Linear fit Result Workspace
        linear_params = self._algorithm.getProperty("LinearFitResult").value
        _validate_table_workspace(self, linear_params, 7, 3)
        expected_values = [1.22397896029, 0.0, 1.0, 13.361671534, 0.0, 1.0, 3.1344868843]
        _validate_table_values_top_to_bottom(self, linear_params, expected_values)
        tear_down()


class TestGammaAndMsCorrectWorkspaceIndexTwo(systemtesting.MantidSystemTest):
    _algorithm = None
    _is_linux = None
    _is_rhel6 = None
    _input_bins = None

    def runTest(self):
        test_ws, _ = setup()
        self._input_bins = test_ws.blocksize()
        self._algorithm = _create_algorithm(
            InputWorkspace=test_ws,
            GammaBackground=True,
            FitParameters=_create_dummy_fit_parameters_ws_index_2(),
            Masses=_create_dummy_masses(),
            MassProfiles=_create_dummy_profiles(),
            WorkspaceIndex=1,
        )

        self._algorithm.execute()

    def validate(self):
        self.assertTrue(self._algorithm.isExecuted())
        # Test Corrections Workspaces
        corrections_wsg = self._algorithm.getProperty("CorrectionWorkspaces").value
        _validate_group_structure(self, corrections_wsg, 3)
        corrections_gb_peak = 0.010067042262
        corrections_ts_peak = 0.156099834417
        corrections_ms_peak = 0.000211000000
        correction_ms_bin = 709

        _validate_matrix_peak_height(self, corrections_wsg.getItem(0), corrections_gb_peak, 457)
        _validate_matrix_peak_height(self, corrections_wsg.getItem(1), corrections_ts_peak, 724)
        _validate_matrix_peak_height(self, corrections_wsg.getItem(2), corrections_ms_peak, correction_ms_bin)

        # Test Corrected Workspaces
        corrected_wsg = self._algorithm.getProperty("CorrectedWorkspaces").value
        _validate_group_structure(self, corrected_wsg, 3)
        corrected_gb_peak = 0.709089359209
        corrected_ts_peak = 0.651236285512
        corrected_ms_peak = 0.702045013627

        _validate_matrix_peak_height(self, corrected_wsg.getItem(0), corrected_gb_peak, 324)
        _validate_matrix_peak_height(self, corrected_wsg.getItem(1), corrected_ts_peak, 324)
        _validate_matrix_peak_height(self, corrected_wsg.getItem(2), corrected_ms_peak, 324)

        # Test OutputWorkspace
        output_ws = self._algorithm.getProperty("OutputWorkspace").value
        _validate_matrix_structure(self, output_ws, 1, self._input_bins)
        output_expected_peak = 0.709044962434
        _validate_matrix_peak_height(self, output_ws, output_expected_peak, 324)

        # Test Linear fit Result Workspace
        linear_params = self._algorithm.getProperty("LinearFitResult").value
        _validate_table_workspace(self, linear_params, 7, 3)
        expected_values = [1.04135754461, 0.0, 1.0, 13.9161945793, 0.0, 1.0, 2.93792229714]
        _validate_table_values_top_to_bottom(self, linear_params, expected_values)
        tear_down()


class TestMsCorrectWithContainer(systemtesting.MantidSystemTest):
    _algorithm = None
    _input_bins = None

    def runTest(self):
        test_ws, test_container_ws = setup()
        self._input_bins = test_ws.blocksize()
        self._algorithm = _create_algorithm(
            InputWorkspace=test_ws,
            ContainerWorkspace=test_container_ws,
            GammaBackground=False,
            FitParameters=_create_dummy_fit_parameters_ws_index_1(),
            Masses=_create_dummy_masses(),
            MassProfiles=_create_dummy_profiles(),
        )

        self._algorithm.execute()

    def validate(self):
        self.assertTrue(self._algorithm.isExecuted())

        # Test Corrections Workspaces
        corrections_wsg = self._algorithm.getProperty("CorrectionWorkspaces").value
        _validate_group_structure(self, corrections_wsg, 3)
        corrections_gb_peak = 0.026998
        corrections_ts_peak = 0.138476
        corrections_ms_peak = 0.000176
        corrections_ts_bin = 726
        corrections_ms_bin = 709

        _validate_matrix_peak_height(self, corrections_wsg.getItem(0), corrections_gb_peak, 3)
        _validate_matrix_peak_height(self, corrections_wsg.getItem(1), corrections_ts_peak, corrections_ts_bin)
        _validate_matrix_peak_height(self, corrections_wsg.getItem(2), corrections_ms_peak, corrections_ms_bin)

        # Test Corrected Workspaces
        corrected_wsg = self._algorithm.getProperty("CorrectedWorkspaces").value
        _validate_group_structure(self, corrected_wsg, 3)
        corrected_gb_peak = 0.621791229741
        corrected_ts_peak = 0.589544590004
        corrected_ms_peak = 0.626687681632

        _validate_matrix_peak_height(self, corrected_wsg.getItem(0), corrected_gb_peak, 325)
        _validate_matrix_peak_height(self, corrected_wsg.getItem(1), corrected_ts_peak, 220)
        _validate_matrix_peak_height(self, corrected_wsg.getItem(2), corrected_ms_peak, 325)

        # Test OutputWorkspace
        output_ws = self._algorithm.getProperty("OutputWorkspace").value
        _validate_matrix_structure(self, output_ws, 1, self._input_bins)
        output_expected_peak = 0.621675084409
        _validate_matrix_peak_height(self, output_ws, output_expected_peak, 325)

        # Test Linear fit Result Workspace
        linear_params = self._algorithm.getProperty("LinearFitResult").value
        _validate_table_workspace(self, linear_params, 7, 3)
        expected_values = [0.0592426949865, 0.0, 1.0, 11.7152355823, 0.0, 1.0, 3.16613507481]
        _validate_table_values_top_to_bottom(self, linear_params, expected_values)
        tear_down()


class TestGammaAndMsCorrectWithContainer(systemtesting.MantidSystemTest):
    _algorithm = None
    _input_bins = None

    def runTest(self):
        test_ws, test_container_ws = setup()
        self._input_bins = test_ws.blocksize()
        self._algorithm = _create_algorithm(
            InputWorkspace=test_ws,
            ContainerWorkspace=test_container_ws,
            FitParameters=_create_dummy_fit_parameters_ws_index_1(),
            Masses=_create_dummy_masses(),
            MassProfiles=_create_dummy_profiles(),
        )

    def validate(self):
        self._algorithm.execute()
        self.assertTrue(self._algorithm.isExecuted())

        # Test Corrections Workspaces
        corrections_wsg = self._algorithm.getProperty("CorrectionWorkspaces").value
        _validate_group_structure(self, corrections_wsg, 4)
        corrections_gb_peak = 0.0249370393881
        corrections_ts_peak = 0.0131222459575
        corrections_ms_peak = 0.147875290632
        corrections_ms_bin = 726

        _validate_matrix_peak_height(self, corrections_wsg.getItem(0), corrections_gb_peak, 3)
        _validate_matrix_peak_height(self, corrections_wsg.getItem(1), corrections_ts_peak, 458)
        _validate_matrix_peak_height(self, corrections_wsg.getItem(2), corrections_ms_peak, corrections_ms_bin)

        # Test Corrected Workspaces
        corrected_wsg = self._algorithm.getProperty("CorrectedWorkspaces").value
        _validate_group_structure(self, corrected_wsg, 4)
        corrected_gb_peak = 0.622167753069
        corrected_ts_peak = 0.634057192209
        corrected_ms_peak = 0.588912707162

        _validate_matrix_peak_height(self, corrected_wsg.getItem(0), corrected_gb_peak, 325)
        _validate_matrix_peak_height(self, corrected_wsg.getItem(1), corrected_ts_peak, 325)
        _validate_matrix_peak_height(self, corrected_wsg.getItem(2), corrected_ms_peak, 220)

        # Test OutputWorkspace
        output_ws = self._algorithm.getProperty("OutputWorkspace").value
        _validate_matrix_structure(self, output_ws, 1, self._input_bins)
        output_expected_peak = 0.629463673633
        _validate_matrix_peak_height(self, output_ws, output_expected_peak, 325)

        # Test Linear fit Result Workspace
        linear_params = self._algorithm.getProperty("LinearFitResult").value
        _validate_table_workspace(self, linear_params, 10, 3)
        expected_values = [0.0547199700231, 0.0, 1.0, 1.18398261145, 0.0, 1.0, 12.5103939279]
        _validate_table_values_top_to_bottom(self, linear_params, expected_values)
        tear_down()


class TestGammaAndMsCorrectWithContainerFixedScaling(systemtesting.MantidSystemTest):
    _algorithm = None
    _input_bins = None

    def runTest(self):
        test_ws, test_container_ws = setup()
        self._input_bins = test_ws.blocksize()
        self._algorithm = _create_algorithm(
            InputWorkspace=test_ws,
            ContainerWorkspace=test_container_ws,
            GammaBackground=True,
            FitParameters=_create_dummy_fit_parameters_ws_index_1(),
            Masses=_create_dummy_masses(),
            MassProfiles=_create_dummy_profiles(),
            ContainerScale=0.1,
            GammaBackgroundScale=0.2,
        )

        self._algorithm.execute()

    def validate(self):
        self.assertTrue(self._algorithm.isExecuted())

        # Test Corrections Workspaces
        corrections_wsg = self._algorithm.getProperty("CorrectionWorkspaces").value
        _validate_group_structure(self, corrections_wsg, 4)
        corrections_gb_peak = 0.045572099871
        corrections_ts_peak = 0.002216628155
        corrections_ms_peak = 0.132649056625
        corrections_ms_bin = 726

        _validate_matrix_peak_height(self, corrections_wsg.getItem(0), corrections_gb_peak, 3)
        _validate_matrix_peak_height(self, corrections_wsg.getItem(1), corrections_ts_peak, 458)
        _validate_matrix_peak_height(self, corrections_wsg.getItem(2), corrections_ms_peak, corrections_ms_bin)

        # Test Corrected Workspaces
        corrected_wsg = self._algorithm.getProperty("CorrectedWorkspaces").value
        _validate_group_structure(self, corrected_wsg, 4)
        corrected_gb_peak = 0.61839812575
        corrected_ts_peak = 0.62796212550
        corrected_ms_peak = 0.58993635557

        _validate_matrix_peak_height(self, corrected_wsg.getItem(0), corrected_gb_peak, 325)
        _validate_matrix_peak_height(self, corrected_wsg.getItem(1), corrected_ts_peak, 325)
        _validate_matrix_peak_height(self, corrected_wsg.getItem(2), corrected_ms_peak, 220)

        # Test OutputWorkspace
        output_ws = self._algorithm.getProperty("OutputWorkspace").value
        _validate_matrix_structure(self, output_ws, 1, self._input_bins)
        output_expected_peak = 0.619602892622
        _validate_matrix_peak_height(self, output_ws, output_expected_peak, 325)

        # Test Linear fit Result Workspace
        linear_params = self._algorithm.getProperty("LinearFitResult").value
        _validate_table_workspace(self, linear_params, 10, 3)
        expected_table_values = [0.1, 0.0, 1.0, 0.2, 0.0, 1.0, "skip", 0.0, 1.0]
        _validate_table_values_top_to_bottom(self, linear_params, expected_table_values)
        tear_down()


class TestCorrectionsInBackScatteringSpectra(systemtesting.MantidSystemTest):
    _algorithm = None
    _input_bins = None

    def runTest(self):
        test_ws, test_container_ws = setup_back_scattering()
        self._input_bins = test_ws.blocksize()
        index_to_symbol = {"0": "H", "1": "O"}
        hydrogen_constraints = {"O": {"factor": 2.0}}

        self._algorithm = _create_algorithm(
            InputWorkspace=test_ws,
            ContainerWorkspace=test_container_ws,
            GammaBackground=True,
            FitParameters=_create_dummy_fit_parameters_no_hydrogen(),
            Masses=_create_dummy_masses(),
            MassProfiles=_create_dummy_profiles(),
            MassIndexToSymbolMap=index_to_symbol,
            HydrogenConstraints=hydrogen_constraints,
            ContainerScale=0.1,
            GammaBackgroundScale=0.2,
        )

        self._algorithm.execute()

    def validate(self):
        self.assertTrue(self._algorithm.isExecuted())

        # Test Corrections Workspaces
        corrections_wsg = self._algorithm.getProperty("CorrectionWorkspaces").value
        _validate_group_structure(self, corrections_wsg, 3)
        corrections_ts_peak = 0.131359579675
        corrections_ms_peak = 0.001551
        corrections_ts_bin = 701
        corrections_ms_bin = 48

        _validate_matrix_peak_height(
            self, corrections_wsg.getItem(1), corrections_ts_peak, corrections_ts_bin, tolerance=0.2, bin_tolerance=5
        )
        _validate_matrix_peak_height(
            self, corrections_wsg.getItem(2), corrections_ms_peak, corrections_ms_bin, tolerance=0.2, bin_tolerance=5
        )

        # Test Corrected Workspaces
        corrected_wsg = self._algorithm.getProperty("CorrectedWorkspaces").value
        _validate_group_structure(self, corrected_wsg, 3)
        corrected_ts_peak = 0.24119589358
        corrected_ms_peak = 0.24119589358
        corrected_ts_bin = 17
        correction_ms_bin = 17

        _validate_matrix_peak_height(self, corrected_wsg.getItem(1), corrected_ts_peak, corrected_ts_bin, tolerance=0.2, bin_tolerance=3)
        _validate_matrix_peak_height(self, corrected_wsg.getItem(2), corrected_ms_peak, correction_ms_bin, tolerance=0.2, bin_tolerance=3)

        # Test OutputWorkspace
        output_ws = self._algorithm.getProperty("OutputWorkspace").value
        _validate_matrix_structure(self, output_ws, 1, self._input_bins)
        output_expected_peak = 0.226039019062
        _validate_matrix_peak_height(self, output_ws, output_expected_peak, 17, tolerance=0.2, bin_tolerance=0.3)

        # Test Linear fit Result Workspace
        linear_params = self._algorithm.getProperty("LinearFitResult").value
        _validate_table_workspace(self, linear_params, 7, 3)
        expected_table_values = [0.1, 0.0, 1.0, "skip", 0.0, 1.0, "skip"]
        _validate_table_values_top_to_bottom(self, linear_params, expected_table_values)
        tear_down()


# ========================================Failure cases======================================


class TestRunningWithoutFitParamsRaisesError(systemtesting.MantidSystemTest):
    _algorithm = None

    def runTest(self):
        test_ws, _ = setup()
        self._algorithm = _create_algorithm(InputWorkspace=test_ws, Masses=_create_dummy_masses(), MassProfiles=_create_dummy_profiles())

    def validate(self):
        self.assertRaises(RuntimeError, self._algorithm.execute)


class TestRunningWithoutMassesRaisesError(systemtesting.MantidSystemTest):
    _algorithm = None

    def runTest(self):
        test_ws, _ = setup()
        self._algorithm = _create_algorithm(
            InputWorkspace=test_ws, FitParameters=_create_dummy_fit_parameters_ws_index_1(), MassProfiles=_create_dummy_profiles()
        )

    def validate(self):
        self.assertRaises(RuntimeError, self._algorithm.execute)


class TestRunningWithoutProfilesRaisesError(systemtesting.MantidSystemTest):
    _algorithm = None

    def runTest(self):
        test_ws, _ = setup()
        self._algorithm = _create_algorithm(
            InputWorkspace=test_ws, FitParameters=_create_dummy_fit_parameters_ws_index_1(), Masses=_create_dummy_masses()
        )

    def validate(self):
        self.assertRaises(RuntimeError, self._algorithm.execute)


# =========================================Validation======================================
# =========================================Structure=======================================


def _validate_group_structure(self, ws_group, expected_entries):
    """
    Checks that a workspace is a group and has the correct number of entries
    ws_group            :: Workspace to be validated
    expected_entries    :: Expected number of entries
    """
    self.assertTrue(isinstance(ws_group, WorkspaceGroup))
    num_entries = ws_group.getNumberOfEntries()
    self.assertEqual(num_entries, expected_entries)


def _validate_matrix_structure(self, matrix_ws, expected_hist, expected_bins):
    """
    Checks that a workspace is a matrix workspace and has the correct number of histograms and bins
    matrix_ws           :: Workspace to be validated
    expected_hist       :: Expected number of histograms
    expected_bins       :: Expected number of bins
    """
    self.assertTrue(isinstance(matrix_ws, MatrixWorkspace))
    num_hists = matrix_ws.getNumberHistograms()
    num_bins = matrix_ws.blocksize()
    self.assertEqual(
        num_hists,
        expected_hist,
        msg="Expected Number of Histograms: " + str(expected_hist) + "\nActual Number of Histograms: " + str(num_hists),
    )
    self.assertEqual(
        num_bins, expected_bins, msg="Expected Number of Bins: " + str(expected_bins) + "\nActual Number of Bins: " + str(num_bins)
    )


def _validate_table_workspace(self, table_ws, expected_rows, expected_columns):
    """
    Checks that a workspace is a table workspace and has the correct number of rows and columns
    table_ws            :: Workspace to be validated
    expected_rows       :: Expected number of rows
    ecpected_columns    :: Expected number of columns
    """
    self.assertTrue(isinstance(table_ws, ITableWorkspace))
    num_rows = table_ws.rowCount()
    num_columns = table_ws.columnCount()
    self.assertEqual(
        num_rows, expected_rows, msg="Expected Number of Rows: " + str(expected_rows) + "\nActual Number of Rows: " + str(num_rows)
    )
    self.assertEqual(
        num_columns,
        expected_columns,
        msg="Expected Number of Columns: " + str(expected_columns) + "\nActual Number of Columns: " + str(num_columns),
    )


# =======================================Values===========================================


def _validate_table_values_top_to_bottom(self, table_ws, expected_values, tolerance=0.05):
    """
    Checks that a table workspace has the expected values from top to bottom
    table_ws            :: Workspace to validate
    expected_values     :: The expected values to be in the table workspace,
                           if any values contained in the list are skip then
                           that value will not be tested.
    """
    for i in range(0, len(expected_values)):
        if expected_values[i] != "skip":
            tolerance_value = expected_values[i] * tolerance
            abs_difference = abs(expected_values[i] - table_ws.cell(i, 1))
            self.assertLessEqual(
                abs_difference,
                abs(tolerance_value),
                msg="Expected Value in Cell "
                + str(i)
                + ": "
                + str(expected_values[i])
                + "\nActual Value in Cell "
                + str(i)
                + ": "
                + str(table_ws.cell(i, 1)),
            )


# pylint: disable=too-many-arguments


def _validate_matrix_peak_height(self, matrix_ws, expected_height, expected_bin, ws_index=0, tolerance=0.05, bin_tolerance=1):
    """
    Checks that the heightest peak value is as expected
    matrix_ws       :: Workspace to validate
    expected_height :: Expected maximum y value (peak height)
    expected_bin    :: Expected bin index of max y value
    ws_index        :: The Index to read from the workspace
    tolerance       :: Percentage of allowed value offset
    """
    y_data = matrix_ws.readY(ws_index)
    peak_height = np.amax(y_data)
    peak_bin = np.argmax(y_data)
    tolerance_value = expected_height * tolerance
    abs_difference = abs(expected_height - peak_height)
    self.assertLessEqual(
        abs_difference, abs(tolerance_value), msg="abs({:.6f} - {:.6f}) > {:.6f}".format(expected_height, peak_height, tolerance_value)
    )
    self.assertTrue(
        abs(peak_bin - expected_bin) <= bin_tolerance, msg="abs({:.6f} - {:.6f}) > {:.6f}".format(peak_bin, expected_bin, bin_tolerance)
    )
