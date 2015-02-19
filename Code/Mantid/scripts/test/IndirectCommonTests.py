"""Test suite for the utility functions in the IndirectCommon script file

These scripts are used by the ISIS Indirect geometry interfaces such as Indirect Convert to Energy,
Data Analysis, and Bayes.
"""
import os
import unittest
import numpy as np

from mantid.simpleapi import *
import IndirectCommon as indirect_common

class IndirectCommonTests(unittest.TestCase):

    _default_config = {}

    def setUp(self):
        self._config_defaults = config
        config['default.facility'] = 'ISIS'

    def tearDown(self):
        config = self._config_defaults

    def test_getInstrRun_from_name(self):
        ws = self.make_dummy_QENS_workspace()
        (instrument, run_number) = indirect_common.getInstrRun(ws)

        self.assertEqual(run_number, '1')
        self.assertEqual(instrument, 'irs')

    def test_getInstrRun_from_workspace(self):
        ws = self.make_dummy_QENS_workspace(add_logs=False)
        ws = RenameWorkspace(ws, OutputWorkspace="IRS26173")

        (instrument, run_number) = indirect_common.getInstrRun(ws.name())

        self.assertEqual(run_number, '26173')
        self.assertEqual(instrument, 'irs')

    def test_getInstrRun_failure(self):
        ws = self.make_dummy_QENS_workspace(add_logs=False)
        self.assertRaises(RuntimeError, indirect_common.getInstrRun, ws)

    def test_getWSprefix_ISIS(self):
        config['default.facility'] = 'ISIS'
        ws = self.make_dummy_QENS_workspace()

        ws_name = indirect_common.getWSprefix(ws)

        self.assertEqual(ws_name, 'irs1_graphite002_',
                        "The workspace prefix does not match the expected value")

    def test_getWSprefix_ILL(self):
        config['default.facility'] = 'ILL'
        ws = self.make_dummy_QENS_workspace(instrument_name='IN16B')

        ws_name = indirect_common.getWSprefix(ws)

        self.assertEqual(ws_name, 'in16b_1_',
                         "The workspace prefix does not match the expected value")

    def test_getEFixed(self):
        ws = CreateSampleWorkspace()
        ws = self.load_instrument(ws,'IRIS')

        e_fixed = indirect_common.getEfixed(ws.name())
        self.assertEqual(e_fixed, 1.8450,
                         "The EFixed value does not match the expected value")

    def test_getEFixed_failure(self):
        ws = CreateSampleWorkspace()
        self.assertRaises(IndexError, indirect_common.getEfixed, ws.name())

    def test_getDefaultWorkingDirectory(self):
        config['defaultsave.directory'] = os.path.expanduser('~')
        workdir = indirect_common.getDefaultWorkingDirectory()
        self.assertEquals(os.path.expanduser('~'), workdir,
                          "The working directory does not match the expected one")

    def test_getDefaultWorkingDirectory_failure(self):
        config['defaultsave.directory'] = ''
        self.assertRaises(IOError, indirect_common.getDefaultWorkingDirectory)

    def test_createQaxis(self):
        ws = self.make_dummy_QENS_workspace()
        expected_result = [0.48372274526965614, 0.5253047207470043, 0.5667692111215948, 0.6079351677527526, 0.6487809073399486]
        actual_result = indirect_common.createQaxis(ws)
        self.assert_lists_almost_match(expected_result, actual_result)

    def test_GetWSangles(self):
        ws = self.make_dummy_QENS_workspace()
        expected_result = [29.700000000000006, 32.32, 34.949999999999996, 37.58, 40.209999999999994]
        actual_result = indirect_common.GetWSangles(ws)
        self.assert_lists_almost_match(expected_result, actual_result)

    def test_GetThetaQ(self):
        ws = self.make_dummy_QENS_workspace()
        expected_theta_result = [29.700000000000006, 32.32, 34.949999999999996, 37.58, 40.209999999999994]
        expected_Q_result = [0.48372274526965625, 0.5253047207470042, 0.5667692111215948, 0.6079351677527525, 0.6487809073399485]
        actual_theta_result, actual_Q_result =  indirect_common.GetThetaQ(ws)
        self.assert_lists_almost_match(expected_theta_result, actual_theta_result)
        self.assert_lists_almost_match(expected_Q_result, actual_Q_result)

    def test_ExtractFloat(self):
        data = "0.0 1 .2 3e-3 4.3 -5.5 6.0"
        expected_result = [0, 1, 0.2, 3e-3, 4.3, -5.5, 6.0]
        actual_result = indirect_common.ExtractFloat(data)
        self.assert_lists_almost_match(expected_result, actual_result)

    def test_ExtractInt(self):
        data = "-2 -1 0 1 2 3 4 5"
        expected_result = [-2, -1, 0, 1, 2, 3, 4, 5]
        actual_result = indirect_common.ExtractInt(data)
        self.assert_lists_match(expected_result, actual_result)

    def test_PadArray(self):
        data = [0,1,2,3,4,5]
        expected_result = [0,1,2,3,4,5,0,0,0,0]
        actual_result = indirect_common.PadArray(data, 10)
        self.assert_lists_match(expected_result, actual_result)

    def test_CheckAnalysers(self):
        ws1 = self.make_dummy_QENS_workspace(output_name="ws1")
        ws2 = self.make_dummy_QENS_workspace(output_name="ws2")

        self.assert_does_not_raise(ValueError, indirect_common.CheckAnalysers, ws1, ws2)

    def test_CheckAnalysers_fails_on_analyser_mismatch(self):
        ws1 = self.make_dummy_QENS_workspace(output_name="ws1", analyser='graphite')
        ws2 = self.make_dummy_QENS_workspace(output_name="ws2", analyser='fmica')

        self.assertRaises(ValueError, indirect_common.CheckAnalysers, ws1, ws2)

    def test_CheckAnalysers_fails_on_reflection_mismatch(self):
        ws1 = self.make_dummy_QENS_workspace(output_name="ws1", reflection='002')
        ws2 = self.make_dummy_QENS_workspace(output_name="ws2", reflection='004')

        self.assertRaises(ValueError, indirect_common.CheckAnalysers, ws1, ws2)

    def test_CheckHistZero(self):
        ws = self.make_dummy_QENS_workspace()
        self.assert_does_not_raise(ValueError, indirect_common.CheckHistZero, ws)

    def test_CheckHistSame(self):
        ws1 = self.make_dummy_QENS_workspace(output_name='ws1')
        ws2 = self.make_dummy_QENS_workspace(output_name='ws2')
        self.assert_does_not_raise(ValueError, indirect_common.CheckHistSame, ws1, 'ws1', ws2, 'ws2')

    def test_CheckHistSame_fails_on_x_range_mismatch(self):
        ws1 = self.make_dummy_QENS_workspace(output_name='ws1')
        ws2 = self.make_dummy_QENS_workspace(output_name='ws2')
        CropWorkspace(ws2, XMin=10, OutputWorkspace=ws2)

        self.assertRaises(ValueError, indirect_common.CheckHistSame, ws1, 'ws1', ws2, 'ws2')

    def test_CheckHistSame_fails_on_spectrum_range_mismatch(self):
        ws1 = self.make_dummy_QENS_workspace(output_name='ws1')
        ws2 = self.make_dummy_QENS_workspace(output_name='ws2')
        CropWorkspace(ws2, StartWorkspaceIndex=2, OutputWorkspace=ws2)

        self.assertRaises(ValueError, indirect_common.CheckHistSame, ws1, 'ws1', ws2, 'ws2')

    def test_CheckXrange(self):
        x_range = [1,10]
        self.assert_does_not_raise(ValueError, indirect_common.CheckXrange, x_range, 'A Range')

    def test_CheckXrange_with_two_ranges(self):
        x_range = [1,10,15,20]
        self.assert_does_not_raise(ValueError, indirect_common.CheckXrange, x_range, 'A Range')

    def test_CheckXrange_lower_close_to_zero(self):
        x_range = [-5,0]
        self.assertRaises(ValueError, indirect_common.CheckXrange, x_range, 'A Range')

    def test_CheckXrange_upper_close_to_zero(self):
        x_range = [0,5]
        self.assertRaises(ValueError, indirect_common.CheckXrange, x_range, 'A Range')

    def test_CheckXrange_invalid_range(self):
        x_range = [10,5]
        self.assertRaises(ValueError, indirect_common.CheckXrange, x_range, 'A Range')

    def test_CheckElimits(self):
        energy_range = [-0.5, 0.5]
        x_range = np.arange(-0.6, 0.61, 0.01)
        self.assert_does_not_raise(ValueError, indirect_common.CheckElimits, energy_range, x_range)

    def test_CheckElimits_lower_bound(self):
        energy_range = [-0.5, 0.4]
        x_range = np.arange(-0.49, 0.5, 0.01)
        self.assertRaises(ValueError, indirect_common.CheckElimits, energy_range, x_range)

    def test_CheckElimits_upper_bound(self):
        energy_range = [-0.5, 0.5]
        x_range = np.arange(-0.5, 0.5, 0.01)
        self.assertRaises(ValueError, indirect_common.CheckElimits, energy_range, x_range)

    def test_CheckElimits_invalid_range(self):
        energy_range = [0.5, -0.5]
        x_range = np.arange(-0.5, 0.51, 0.01)
        self.assertRaises(ValueError, indirect_common.CheckElimits, energy_range, x_range)

    def test_convertToElasticQ(self):
        ws = self.make_dummy_QENS_workspace()
        indirect_common.convertToElasticQ(ws)
        self.assert_workspace_units_match_expected('MomentumTransfer', ws)
        self.assert_has_numeric_axis(ws)

    def test_convertToElasticQ_output_in_different_workspace(self):
        ws = self.make_dummy_QENS_workspace()
        output_workspace = 'ws2'
        indirect_common.convertToElasticQ(ws, output_ws=output_workspace)

        #check original wasn't modified
        self.assert_workspace_units_match_expected('Label', ws)
        self.assert_has_spectrum_axis(ws)

        #check new workspace matches what we expect
        self.assert_workspace_units_match_expected('MomentumTransfer', output_workspace)
        self.assert_has_numeric_axis(output_workspace)

    def test_convertToElasticQ_workspace_already_in_Q(self):
        ws = self.make_dummy_QENS_workspace()
        e_fixed = indirect_common.getEfixed(ws)
        ConvertSpectrumAxis(ws,Target='ElasticQ',EMode='Indirect',EFixed=e_fixed,OutputWorkspace=ws)

        indirect_common.convertToElasticQ(ws)

        self.assert_workspace_units_match_expected('MomentumTransfer', ws)
        self.assert_has_numeric_axis(ws)

    def test_convertToElasticQ_with_numeric_axis_not_in_Q(self):
        ws = self.make_dummy_QENS_workspace()

        #convert spectrum axis to units of Q
        e_fixed = indirect_common.getEfixed(ws)
        ConvertSpectrumAxis(ws,Target='ElasticQ',EMode='Indirect',EFixed=e_fixed,OutputWorkspace=ws)
        #set the units to be something we didn't expect
        unit = mtd[ws].getAxis(1).setUnit("Label")
        unit.setLabel('Random Units', '')

        self.assertRaises(RuntimeError, indirect_common.convertToElasticQ, ws)

    def test_transposeFitParametersTable(self):
        ws = self.make_dummy_QENS_workspace()
        params_table = self.make_multi_domain_parameter_table(ws)
        indirect_common.transposeFitParametersTable(params_table)
        self.assert_table_workspace_dimensions(params_table, expected_row_count=5, expected_column_count=11)

    def test_transposeFitParametersTable_rename_output(self):
        ws = self.make_dummy_QENS_workspace()
        params_table = self.make_multi_domain_parameter_table(ws)
        output_name = "new_table"

        indirect_common.transposeFitParametersTable(params_table, output_name)

        self.assert_table_workspace_dimensions(params_table, expected_row_count=26, expected_column_count=3)
        self.assert_table_workspace_dimensions(output_name, expected_row_count=5, expected_column_count=11)

    def test_search_for_fit_params(self):
        ws = self.make_dummy_QENS_workspace()

        #make a parameter table to search in
        function = "name=LinearBackground, A0=0, A1=0;"
        function += "name=Gaussian, Sigma=0.1, PeakCentre=0, Height=10;"
        table_ws = PlotPeakByLogValue(Input=ws+",v", Function=function)

        params = indirect_common.search_for_fit_params("Sigma", table_ws.name())

        self.assertEqual(len(params), 1)
        self.assertEqual(params[0], "f1.Sigma")

    def test_convertParametersToWorkspace(self):
        ws = self.make_dummy_QENS_workspace()

        #make a parameter table to search in
        function = "name=LinearBackground, A0=0, A1=0;"
        function += "name=Gaussian, Sigma=0.1, PeakCentre=0, Height=10;"
        table_ws = PlotPeakByLogValue(Input=ws + ",v0:10", Function=function)

        param_names = ['A0', 'Sigma', 'PeakCentre']
        indirect_common.convertParametersToWorkspace(table_ws.name(), 'axis-1', param_names, "params_workspace")
        params_workspace = mtd["params_workspace"]

        self.assert_matrix_workspace_dimensions(params_workspace.name(),
                                                expected_num_histograms=3, expected_blocksize=5)

    def test_addSampleLogs(self):
        ws = CreateSampleWorkspace()
        logs = {}
        logs['FloatLog'] = 3.149
        logs['IntLog'] = 42
        logs['StringLog'] = "A String Log"
        logs['BooleanLog'] = True

        indirect_common.addSampleLogs(ws, logs)

        self.assert_logs_match_expected(ws.name(), logs)

    def test_addSampleLogs_empty_dict(self):
        ws = CreateSampleWorkspace()
        logs = {}
        self.assert_does_not_raise(Exception, indirect_common.addSampleLogs, ws, logs)

    #-----------------------------------------------------------
    # Custom assertion functions
    #-----------------------------------------------------------

    def assert_lists_almost_match(self, expected, actual,decimal=6):
        self.assertTrue(isinstance(expected, list))
        np.testing.assert_array_almost_equal(expected, actual, decimal, "The results do not match")

    def assert_lists_match(self, expected, actual):
        self.assertTrue(isinstance(expected, list))
        np.testing.assert_array_equal(expected, actual, "The results do not match")

    def assert_does_not_raise(self, exception_type, func, *args):
        """ Check if this function raises the expected exception """
        try:
            func(*args)
        except exception_type:
            self.fail("%s should not of raised anything but it did." % func.__name__)

    def assert_workspace_units_match_expected(self, expected_unit_ID, ws, axis_number=1):
        axis = mtd[ws].getAxis(axis_number)
        actual_unit_ID = axis.getUnit().unitID()
        self.assertEquals(expected_unit_ID, actual_unit_ID)

    def assert_has_spectrum_axis(self, ws, axis_number=1):
        axis = mtd[ws].getAxis(axis_number)
        self.assertTrue(axis.isSpectra())

    def assert_has_numeric_axis(self, ws, axis_number=1):
        axis = mtd[ws].getAxis(axis_number)
        self.assertTrue(axis.isNumeric())

    def assert_table_workspace_dimensions(self, workspace, expected_column_count, expected_row_count):
        actual_row_count = mtd[workspace].rowCount()
        actual_column_count = mtd[workspace].columnCount()
        self.assertEquals(expected_row_count, actual_row_count,
                          "Number of rows does not match expected (%d != %d)"
                          % (expected_row_count, actual_row_count))
        self.assertEquals(expected_column_count, actual_column_count,
                          "Number of columns does not match expected (%d != %d)"
                          % (expected_column_count, actual_column_count))

    def assert_matrix_workspace_dimensions(self, workspace, expected_num_histograms, expected_blocksize):
        actual_blocksize = mtd[workspace].blocksize()
        actual_num_histograms = mtd[workspace].getNumberHistograms()
        self.assertEqual(actual_num_histograms, expected_num_histograms,
                         "Number of histograms does not match expected (%d != %d)"
                         % (expected_num_histograms, actual_num_histograms))
        self.assertEqual(expected_blocksize, actual_blocksize,
                         "Workspace blocksize does not match expected (%d != %d)"
                         % (expected_blocksize, actual_blocksize))

    def assert_logs_match_expected(self, workspace, expected_logs):
        run = mtd[workspace].getRun()
        for log_name, log_value in expected_logs.iteritems():
            self.assertTrue(run.hasProperty(log_name),
                            "The log %s is missing from the workspace" % log_name)
            self.assertEqual(str(run.getProperty(log_name).value), str(log_value),
                             "The expected value of log %s did not match (%s != %s)" %
                             (log_name, str(log_value), run.getProperty(log_name).value))

    #-----------------------------------------------------------
    # Test helper functions
    #-----------------------------------------------------------

    def make_dummy_QENS_workspace(self, output_name="ws", instrument_name='IRIS',
                                  analyser='graphite', reflection='002', add_logs=True):
        """ Make a workspace that looks like QENS data """
        ws = CreateSampleWorkspace(OutputWorkspace=output_name)
        self.load_instrument(ws, instrument_name, analyser, reflection)
        ws = CropWorkspace(ws, StartWorkspaceIndex=3, EndWorkspaceIndex=7, OutputWorkspace=output_name)

        if add_logs:
            AddSampleLog(ws, LogName='run_number', LogType='Number', LogText='00001')

        return ws.name()

    def make_multi_domain_function(self, ws, function):
        """ Make a multi domain function from a regular function string """
        multi_domain_composite = 'composite=MultiDomainFunction,NumDeriv=1;'
        component =  '(composite=CompositeFunction,$domains=i;%s);' % function

        fit_kwargs = {}
        num_spectra = mtd[ws].getNumberHistograms()
        for i in range(0, num_spectra):
            multi_domain_composite += component
            if i > 0:
                fit_kwargs['WorkspaceIndex_%d' % i] = i
                fit_kwargs['InputWorkspace_%d' % i] = ws

        return multi_domain_composite, fit_kwargs

    def make_multi_domain_parameter_table(self, ws):
        """ Fit a multi domain function to get a table of parameters """
        table_name = "test_fit"
        function = "name=LinearBackground, A0=0, A1=0;"
        function += "name=Gaussian, Sigma=0.1, PeakCentre=0, Height=10;"
        multi_domain_function, fit_kwargs = self.make_multi_domain_function(ws, function)
        Fit(Function=multi_domain_function, InputWorkspace=ws, WorkspaceIndex=0,
            Output=table_name, CreateOutput=True, MaxIterations=0, **fit_kwargs)
        return table_name + "_Parameters"

    def load_instrument(self, ws, instrument, analyser='graphite', reflection='002'):
        """Load an instrument parameter from the ipf directory"""
        LoadInstrument(ws, InstrumentName=instrument)

        if config['default.facility'] != 'ILL':
            parameter_file_name = '%s_%s_%s_Parameters.xml' % (instrument, analyser, reflection)
            ipf = os.path.join(config['instrumentDefinition.directory'],
                               parameter_file_name)
            LoadParameterFile(ws, Filename=ipf)

        return ws


if __name__=="__main__":
    unittest.main()
