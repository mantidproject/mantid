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

    _default_facility = ''

    def setUp(self):
        self._config_defaults = config

    def tearDown(self):
        config = self._config_defaults

    def test_loadInst(self):
        indirect_common.loadInst('IRIS')

        ws_name = '__empty_IRIS'
        ws = mtd[ws_name]
        instrument = ws.getInstrument()
        self.assertEqual(instrument.getName(), 'IRIS')

    def test_loadNexus(self):
        ws_name = indirect_common.loadNexus('IRS26173_ipg.nxs')
        self.assertEqual(ws_name, 'IRS26173_ipg')
        self.assertTrue(mtd.doesExist(ws_name))

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
        self.assert_lists_match(expected_result, actual_result)        

    def test_GetWSangles(self):
        ws = self.make_dummy_QENS_workspace()
        expected_result = [29.700000000000006, 32.32, 34.949999999999996, 37.58, 40.209999999999994]
        actual_result = indirect_common.GetWSangles(ws)
        self.assert_lists_match(expected_result, actual_result)

    def test_GetThetaQ(self):
        ws = self.make_dummy_QENS_workspace()
        expected_theta_result = [29.700000000000006, 32.32, 34.949999999999996, 37.58, 40.209999999999994]
        expected_Q_result = [0.48372274526965625, 0.5253047207470042, 0.5667692111215948, 0.6079351677527525, 0.6487809073399485]
        actual_theta_result, actual_Q_result =  indirect_common.GetThetaQ(ws)
        self.assert_lists_match(expected_theta_result, actual_theta_result)
        self.assert_lists_match(expected_Q_result, actual_Q_result)

    def test_ExtractFloat(self):
        data = "0.0 1 .2 3e-3 4.3 -5.5 6.0"
        expected_result = [0, 1, 0.2, 3e-3, 4.3, -5.5, 6.0]
        actual_result = indirect_common.ExtractFloat(data)
        self.assert_lists_match(expected_result, actual_result)

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


    #-----------------------------------------------------------
    # Test helper functions
    #-----------------------------------------------------------

    def assert_lists_match(self, expected, actual):
        self.assertTrue(isinstance(expected, list))
        np.testing.assert_array_equal(expected, actual, "The results do not match")

    def make_dummy_QENS_workspace(self, instrument_name='IRIS', add_logs=True):
        """ Make a workspace that looks like QENS data """
        ws = CreateSampleWorkspace(OutputWorkspace="ws")
        self.load_instrument(ws, instrument_name)
        ws = CropWorkspace(ws, StartWorkspaceIndex=3, EndWorkspaceIndex=7)

        if add_logs:
            AddSampleLog(ws, LogName='run_number', LogType='Number', LogText='00001')
    
        return ws.name()

    def load_instrument(self, ws, instrument):
        """Load an instrument parameter from the ipf directory"""
        LoadInstrument(ws, InstrumentName=instrument)

        if config['default.facility'] != 'ILL':
            ipf = os.path.join(config['instrumentDefinition.directory'], 
                               instrument+'_graphite_002_Parameters.xml')
            LoadParameterFile(ws, Filename=ipf)
    
        return ws

if __name__=="__main__":
    unittest.main()