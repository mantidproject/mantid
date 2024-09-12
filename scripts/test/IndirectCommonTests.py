# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import numpy as np
from os.path import join

from mantid import config
from mantid.api import AnalysisDataService, NumericAxis
from mantid.simpleapi import (
    AddSampleLog,
    CreateSampleWorkspace,
    CreateWorkspace,
    ConvertSpectrumAxis,
    CropWorkspace,
    Fit,
    LoadInstrument,
    LoadParameterFile,
    RenameWorkspace,
)
import IndirectCommon as indirect_common


class IndirectCommonTests(unittest.TestCase):

    def setUp(self):
        config["default.facility"] = "ISIS"

    def test_get_instrument_and_run_from_name(self):
        ws = self.make_dummy_QENS_workspace()
        (instrument, run_number) = indirect_common.get_instrument_and_run(ws)

        self.assertEqual(run_number, "1")
        self.assertEqual(instrument, "irs")

    def test_get_instrument_and_run_from_workspace(self):
        ws = self.make_dummy_QENS_workspace(add_logs=False)
        ws = RenameWorkspace(ws, OutputWorkspace="IRS26173")

        (instrument, run_number) = indirect_common.get_instrument_and_run(ws.name())

        self.assertEqual(run_number, "26173")
        self.assertEqual(instrument, "irs")

    def test_get_instrument_and_run_failure(self):
        ws = self.make_dummy_QENS_workspace(add_logs=False)
        self.assertRaises(RuntimeError, indirect_common.get_instrument_and_run, ws)

    def test_get_workspace_name_prefix_ISIS(self):
        config["default.facility"] = "ISIS"
        ws = self.make_dummy_QENS_workspace()

        ws_name = indirect_common.get_workspace_name_prefix(ws)

        self.assertEqual(ws_name, "irs1_graphite002_", "The workspace prefix does not match the expected value")

    def test_get_workspace_name_prefix_ILL(self):
        config["default.facility"] = "ILL"
        ws = self.make_dummy_QENS_workspace(instrument_name="IN16B")

        ws_name = indirect_common.get_workspace_name_prefix(ws)

        self.assertEqual(ws_name, "in16b_1_", "The workspace prefix does not match the expected value")

    def test_get_efixed(self):
        ws = CreateSampleWorkspace()
        ws = self.load_instrument(ws, "IRIS")

        e_fixed = indirect_common.get_efixed(ws.name())
        self.assertEqual(e_fixed, 1.8450, "The EFixed value does not match the expected value")

    def test_get_efixed_failure(self):
        ws = CreateSampleWorkspace()
        self.assertRaises(ValueError, indirect_common.get_efixed, ws.name())

    def test_get_efixed_with_no_instrument_but_efixed_sample_log(self):
        ws = self.make_dummy_workspace_without_instrument("test_ws1")
        AddSampleLog(ws, LogName="EFixed", LogType="Number", LogText="1.83")

        e_fixed = indirect_common.get_efixed(ws)
        self.assertEqual(e_fixed, 1.83, "The EFixed value does not match the expected value")

    def test_get_efixed_with_no_instrument_but_ei_sample_log(self):
        ws = self.make_dummy_workspace_without_instrument("test_ws1")
        AddSampleLog(ws, LogName="Ei", LogType="Number", LogText="1.83")

        e_fixed = indirect_common.get_efixed(ws)
        self.assertEqual(e_fixed, 1.83, "The EFixed value does not match the expected value")

    def test_get_two_theta_angles(self):
        ws = self.make_dummy_QENS_workspace()
        expected_result = [29.700000000000006, 32.32, 34.949999999999996, 37.58, 40.209999999999994]
        actual_result = indirect_common.get_two_theta_angles(ws)
        self.assert_lists_almost_match(expected_result, actual_result)

    def test_get_two_theta_and_q_with_spectra_axis(self):
        ws = self.make_dummy_QENS_workspace()
        expected_theta_result = [29.700000000000006, 32.32, 34.949999999999996, 37.58, 40.209999999999994]
        expected_Q_result = [0.48372274526965625, 0.5253047207470042, 0.5667692111215948, 0.6079351677527525, 0.6487809073399485]
        actual_theta_result, actual_Q_result = indirect_common.get_two_theta_and_q(ws)
        self.assert_lists_almost_match(expected_theta_result, actual_theta_result)
        self.assert_lists_almost_match(expected_Q_result, actual_Q_result)

    def test_get_two_theta_and_q_with_numeric_axis(self):
        q_values = [0.48372274526965625, 0.5253047207470042, 0.5667692111215948, 0.6079351677527525, 0.6487809073399485]

        ws = AnalysisDataService.retrieve(self.make_dummy_QENS_workspace())
        numeric_axis = NumericAxis.create(len(q_values))
        numeric_axis.setUnit("MomentumTransfer")
        for i, q_value in enumerate(q_values):
            numeric_axis.setValue(i, q_value)

        ws.replaceAxis(1, numeric_axis)

        actual_theta_values, actual_q_values = indirect_common.get_two_theta_and_q(ws)

        expected_theta_values = [29.700000000000006, 32.32, 34.949999999999996, 37.58, 40.209999999999994]
        self.assert_lists_almost_match(expected_theta_values, actual_theta_values)
        self.assert_lists_almost_match(q_values, actual_q_values)

    def test_extract_float(self):
        data = "0.0 1 .2 3e-3 4.3 -5.5 6.0"
        expected_result = [0, 1, 0.2, 3e-3, 4.3, -5.5, 6.0]
        actual_result = indirect_common.extract_float(data)
        self.assert_lists_almost_match(expected_result, actual_result)

    def test_extract_int(self):
        data = "-2 -1 0 1 2 3 4 5"
        expected_result = [-2, -1, 0, 1, 2, 3, 4, 5]
        actual_result = indirect_common.extract_int(data)
        self.assert_lists_match(expected_result, actual_result)

    def test_pad_array(self):
        data = [0, 1, 2, 3, 4, 5]
        expected_result = [0, 1, 2, 3, 4, 5, 0, 0, 0, 0]
        actual_result = indirect_common.pad_array(data, 10)
        self.assert_lists_match(expected_result, actual_result)

    def test_check_analysers_or_e_fixed(self):
        ws1 = self.make_dummy_QENS_workspace(output_name="ws1")
        ws2 = self.make_dummy_QENS_workspace(output_name="ws2")

        self.assert_does_not_raise(ValueError, indirect_common.check_analysers_or_e_fixed, ws1, ws2)

    def test_check_analysers_or_e_fixed_fails_on_analyser_mismatch(self):
        ws1 = self.make_dummy_QENS_workspace(output_name="ws1", analyser="graphite")
        ws2 = self.make_dummy_QENS_workspace(output_name="ws2", analyser="fmica")

        self.assertRaises(ValueError, indirect_common.check_analysers_or_e_fixed, ws1, ws2)

    def test_check_analysers_or_e_fixed_fails_on_reflection_mismatch(self):
        ws1 = self.make_dummy_QENS_workspace(output_name="ws1", reflection="002")
        ws2 = self.make_dummy_QENS_workspace(output_name="ws2", reflection="004")

        self.assertRaises(ValueError, indirect_common.check_analysers_or_e_fixed, ws1, ws2)

    def test_check_analysers_or_e_fixed_does_not_raise_error_with_no_inst_data(self):
        ws1 = self.make_dummy_workspace_without_instrument("test_ws1")
        ws2 = self.make_dummy_workspace_without_instrument("test_ws2")
        self.assert_does_not_raise(RuntimeError, indirect_common.check_analysers_or_e_fixed, ws1, ws2)

    def test_check_hist_zero(self):
        ws = self.make_dummy_QENS_workspace()
        self.assert_does_not_raise(ValueError, indirect_common.check_hist_zero, ws)

    def test_check_dimensions_equal(self):
        ws1 = self.make_dummy_QENS_workspace(output_name="ws1")
        ws2 = self.make_dummy_QENS_workspace(output_name="ws2")
        self.assert_does_not_raise(ValueError, indirect_common.check_dimensions_equal, ws1, "ws1", ws2, "ws2")

    def test_check_dimensions_equal_fails_on_x_range_mismatch(self):
        ws1 = self.make_dummy_QENS_workspace(output_name="ws1")
        ws2 = self.make_dummy_QENS_workspace(output_name="ws2")
        CropWorkspace(ws2, XMin=10, OutputWorkspace=ws2)

        self.assertRaises(ValueError, indirect_common.check_dimensions_equal, ws1, "ws1", ws2, "ws2")

    def test_check_dimensions_equal_fails_on_spectrum_range_mismatch(self):
        ws1 = self.make_dummy_QENS_workspace(output_name="ws1")
        ws2 = self.make_dummy_QENS_workspace(output_name="ws2")
        CropWorkspace(ws2, StartWorkspaceIndex=2, OutputWorkspace=ws2)

        self.assertRaises(ValueError, indirect_common.check_dimensions_equal, ws1, "ws1", ws2, "ws2")

    def test_check_x_range(self):
        x_range = [1, 10]
        self.assert_does_not_raise(ValueError, indirect_common.check_x_range, x_range, "A Range")

    def test_check_x_range_with_two_ranges(self):
        x_range = [1, 10, 15, 20]
        self.assert_does_not_raise(ValueError, indirect_common.check_x_range, x_range, "A Range")

    def test_check_x_range_lower_close_to_zero(self):
        x_range = [-5, 0]
        self.assertRaises(ValueError, indirect_common.check_x_range, x_range, "A Range")

    def test_check_x_range_upper_close_to_zero(self):
        x_range = [0, 5]
        self.assertRaises(ValueError, indirect_common.check_x_range, x_range, "A Range")

    def test_check_x_range_invalid_range(self):
        x_range = [10, 5]
        self.assertRaises(ValueError, indirect_common.check_x_range, x_range, "A Range")

    def test_convert_to_elastic_q(self):
        ws = self.make_dummy_QENS_workspace()
        indirect_common.convert_to_elastic_q(ws)
        self.assert_workspace_units_match_expected("MomentumTransfer", ws)
        self.assert_has_numeric_axis(ws)

    def test_convert_to_elastic_q_output_in_different_workspace(self):
        ws = self.make_dummy_QENS_workspace()
        output_workspace = "ws2"
        indirect_common.convert_to_elastic_q(ws, output_ws=output_workspace)

        # check original wasn't modified
        self.assert_workspace_units_match_expected("Label", ws)
        self.assert_has_spectrum_axis(ws)

        # check new workspace matches what we expect
        self.assert_workspace_units_match_expected("MomentumTransfer", output_workspace)
        self.assert_has_numeric_axis(output_workspace)

    def test_convert_to_elastic_q_workspace_already_in_Q(self):
        ws = self.make_dummy_QENS_workspace()
        e_fixed = indirect_common.get_efixed(ws)
        ConvertSpectrumAxis(ws, Target="ElasticQ", EMode="Indirect", EFixed=e_fixed, OutputWorkspace=ws)

        indirect_common.convert_to_elastic_q(ws)

        self.assert_workspace_units_match_expected("MomentumTransfer", ws)
        self.assert_has_numeric_axis(ws)

    def test_convert_to_elastic_q_with_numeric_axis_not_in_Q(self):
        ws = self.make_dummy_QENS_workspace()

        # convert spectrum axis to units of Q
        e_fixed = indirect_common.get_efixed(ws)
        ConvertSpectrumAxis(ws, Target="ElasticQ", EMode="Indirect", EFixed=e_fixed, OutputWorkspace=ws)
        # set the units to be something we didn't expect
        unit = AnalysisDataService.retrieve(ws).getAxis(1).setUnit("Label")
        unit.setLabel("Random Units", "")

        self.assertRaises(RuntimeError, indirect_common.convert_to_elastic_q, ws)

    def test_transpose_fit_parameters_table(self):
        ws = self.make_dummy_QENS_workspace()
        params_table = self.make_multi_domain_parameter_table(ws)
        indirect_common.transpose_fit_parameters_table(params_table)
        self.assert_table_workspace_dimensions(params_table, expected_row_count=5, expected_column_count=11)

    def test_transpose_fit_parameters_table_rename_output(self):
        ws = self.make_dummy_QENS_workspace()
        params_table = self.make_multi_domain_parameter_table(ws)
        output_name = "new_table"

        indirect_common.transpose_fit_parameters_table(params_table, output_name)

        self.assert_table_workspace_dimensions(params_table, expected_row_count=26, expected_column_count=3)
        self.assert_table_workspace_dimensions(output_name, expected_row_count=5, expected_column_count=11)

    def test_identify_non_zero_bin_range_returns_the_non_zero_bin_range(self):
        ws = self.make_dummy_QENS_workspace()
        e_min, e_max = indirect_common.identify_non_zero_bin_range(AnalysisDataService.retrieve(ws), 0)

        self.assertEqual(0.0, e_min)
        self.assertEqual(20000.0, e_max)

    def test_identify_non_zero_bin_range_returns_the_expected_range_for_leading_zeros(self):
        ws = CreateWorkspace(DataX="1,2,3,4,5", DataY="0,1,2,1,3", StoreInADS=False)
        e_min, e_max = indirect_common.identify_non_zero_bin_range(ws, 0)

        self.assertEqual(2.0, e_min)
        self.assertEqual(5.0, e_max)

    def test_identify_non_zero_bin_range_returns_the_expected_range_for_trailing_zeros(self):
        ws = CreateWorkspace(DataX="1,2,3,4,5", DataY="4,1,2,0,0", StoreInADS=False)
        e_min, e_max = indirect_common.identify_non_zero_bin_range(ws, 0)

        self.assertEqual(1.0, e_min)
        self.assertEqual(3.0, e_max)

    # -----------------------------------------------------------
    # Custom assertion functions
    # -----------------------------------------------------------

    def assert_lists_almost_match(self, expected, actual, decimal=6):
        self.assertTrue(isinstance(expected, list))
        np.testing.assert_array_almost_equal(expected, actual, decimal, "The results do not match")

    def assert_lists_match(self, expected, actual):
        self.assertTrue(isinstance(expected, list))
        np.testing.assert_array_equal(expected, actual, "The results do not match")

    def assert_does_not_raise(self, exception_type, func, *args):
        """Check if this function raises the expected exception"""
        try:
            func(*args)
        except exception_type:
            self.fail(f"{func.__name__} should not of raised anything but it did.")

    def assert_workspace_units_match_expected(self, expected_unit_ID, ws, axis_number=1):
        axis = AnalysisDataService.retrieve(ws).getAxis(axis_number)
        actual_unit_ID = axis.getUnit().unitID()
        self.assertEqual(expected_unit_ID, actual_unit_ID)

    def assert_has_spectrum_axis(self, ws, axis_number=1):
        axis = AnalysisDataService.retrieve(ws).getAxis(axis_number)
        self.assertTrue(axis.isSpectra())

    def assert_has_numeric_axis(self, ws, axis_number=1):
        axis = AnalysisDataService.retrieve(ws).getAxis(axis_number)
        self.assertTrue(axis.isNumeric())

    def assert_table_workspace_dimensions(self, workspace, expected_column_count, expected_row_count):
        actual_row_count = AnalysisDataService.retrieve(workspace).rowCount()
        actual_column_count = AnalysisDataService.retrieve(workspace).columnCount()
        self.assertEqual(
            expected_row_count,
            actual_row_count,
            f"Number of rows does not match expected ({expected_row_count} != {actual_row_count})",
        )
        self.assertEqual(
            expected_column_count,
            actual_column_count,
            f"Number of columns does not match expected ({expected_column_count} != {actual_column_count})",
        )

    def assert_matrix_workspace_dimensions(self, workspace, expected_num_histograms, expected_blocksize):
        actual_blocksize = AnalysisDataService.retrieve(workspace).blocksize()
        actual_num_histograms = AnalysisDataService.retrieve(workspace).getNumberHistograms()
        self.assertEqual(
            actual_num_histograms,
            expected_num_histograms,
            f"Number of histograms does not match expected ({expected_num_histograms} != {actual_num_histograms})",
        )
        self.assertEqual(
            expected_blocksize,
            actual_blocksize,
            f"Workspace blocksize does not match expected ({expected_blocksize} != {actual_blocksize})",
        )

    def assert_logs_match_expected(self, workspace, expected_logs):
        run = AnalysisDataService.retrieve(workspace).getRun()
        for log_name, log_value in expected_logs.items():
            self.assertTrue(run.hasProperty(log_name), f"The log {log_name} is missing from the workspace")
            self.assertEqual(
                str(run.getProperty(log_name).value),
                str(log_value),
                f"The expected value of log {log_name} did not match ({str(log_value)} != {run.getProperty(log_name).value})",
            )

    # -----------------------------------------------------------
    # Test helper functions
    # -----------------------------------------------------------

    def make_dummy_workspace_without_instrument(self, output_name):
        """
        Makes a workspace with no instrument information
        """
        ws = CreateWorkspace(OutputWorkspace=output_name, DataX="1,2,3,4,5", DataY="0,0,2,0,0")
        return ws.name()

    def make_dummy_QENS_workspace(self, output_name="ws", instrument_name="IRIS", analyser="graphite", reflection="002", add_logs=True):
        """Make a workspace that looks like QENS data"""
        ws = CreateSampleWorkspace(OutputWorkspace=output_name)
        self.load_instrument(ws, instrument_name, analyser, reflection)
        ws = CropWorkspace(ws, StartWorkspaceIndex=3, EndWorkspaceIndex=7, OutputWorkspace=output_name)

        if add_logs:
            AddSampleLog(ws, LogName="run_number", LogType="Number", LogText="00001")

        return ws.name()

    def make_multi_domain_function(self, ws, function):
        """Make a multi domain function from a regular function string"""
        multi_domain_composite = "composite=MultiDomainFunction,NumDeriv=1;"
        component = f"(composite=CompositeFunction,$domains=i;{function});"

        fit_kwargs = {}
        num_spectra = AnalysisDataService.retrieve(ws).getNumberHistograms()
        for i in range(0, num_spectra):
            multi_domain_composite += component
            if i > 0:
                fit_kwargs[f"WorkspaceIndex_{i}"] = i
                fit_kwargs[f"InputWorkspace_{i}"] = ws

        return multi_domain_composite, fit_kwargs

    def make_multi_domain_parameter_table(self, ws):
        """Fit a multi domain function to get a table of parameters"""
        table_name = "test_fit"
        function = "name=LinearBackground, A0=0, A1=0;"
        function += "name=Gaussian, Sigma=0.1, PeakCentre=0, Height=10;"
        multi_domain_function, fit_kwargs = self.make_multi_domain_function(ws, function)
        Fit(
            Function=multi_domain_function,
            InputWorkspace=ws,
            WorkspaceIndex=0,
            Output=table_name,
            CreateOutput=True,
            MaxIterations=0,
            **fit_kwargs,
        )
        return table_name + "_Parameters"

    def load_instrument(self, ws, instrument, analyser="graphite", reflection="002"):
        """Load an instrument parameter from the ipf directory"""
        LoadInstrument(ws, InstrumentName=instrument, RewriteSpectraMap=True)

        if config["default.facility"] != "ILL":
            parameter_file_name = f"{instrument}_{analyser}_{reflection}_Parameters.xml"
            ipf = join(config["instrumentDefinition.directory"], parameter_file_name)
            LoadParameterFile(ws, Filename=ipf)

        return ws


if __name__ == "__main__":
    unittest.main()
