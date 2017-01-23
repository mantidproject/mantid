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
        self.assertRaises(ValueError, indirect_common.getEfixed, ws.name())

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

    def test_PadArray(self):
        data = [0,1,2,3,4,5]
        expected_result = [0,1,2,3,4,5,0,0,0,0]
        actual_result = indirect_common.PadArray(data, 10)
        self.assert_lists_match(expected_result, actual_result)

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

    def make_dummy_workspace_without_instrument(self, output_name):
        """
        Makes a workspace with no instrument information
        """
        ws = CreateWorkspace(OutputWorkspace=output_name, DataX='1,2,3,4,5', DataY='0,0,2,0,0')
        return ws.name()

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
        LoadInstrument(ws, InstrumentName=instrument, RewriteSpectraMap=True)

        if config['default.facility'] != 'ILL':
            parameter_file_name = '%s_%s_%s_Parameters.xml' % (instrument, analyser, reflection)
            ipf = os.path.join(config['instrumentDefinition.directory'],
                               parameter_file_name)
            LoadParameterFile(ws, Filename=ipf)

        return ws


if __name__=="__main__":
    unittest.main()
