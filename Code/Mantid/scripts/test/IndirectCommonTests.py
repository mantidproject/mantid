"""Test suite for the utility functions in the IndirectCommon script file

These scripts are used by the ISIS Indirect geometry interfaces such as Indirect Convert to Energy,
Data Analysis, and Bayes.   
"""
from mantid.simpleapi import *
import IndirectCommon as ic
import unittest
import os

class IndirectCommonTests(unittest.TestCase):

    _default_facility = ''

    def setUp(self):
        self._default_facility = config['default.facility']

    def tearDown(self):
        config['default.facility'] = self._default_facility

    def test_loadInst(self):
        ic.loadInst('IRIS')

        ws_name = '__empty_IRIS'
        ws = mtd[ws_name]
        instrument = ws.getInstrument()
        self.assertEqual(instrument.getName(), 'IRIS')

    def test_loadNexus(self):
        ws_name = ic.loadNexus('IRS26173_ipg.nxs')
        self.assertEqual(ws_name, 'IRS26173_ipg')
        self.assertTrue(mtd.doesExist(ws_name))

    def test_getInstrRun_from_name(self):
        ws = LoadNexus('IRS26173_ipg.nxs', OutputWorkspace='IRS26173_ipg')
        (instrument, run_number) = ic.getInstrRun(ws.name())

        self.assertEqual(run_number, '26173')
        self.assertEqual(instrument, 'irs')

    def test_getInstrRun_from_workspace(self):
        ws = LoadNexus('INTER00013464.nxs', OutputWorkspace='AWorkspace')
        (instrument, run_number) = ic.getInstrRun(ws.name())

        self.assertEqual(run_number, '13464')
        self.assertEqual(instrument, 'inter')

    def test_getInstrRun_failure(self):
        ws = LoadNexus('IRS26173_ipg.nxs', OutputWorkspace='AWorkspace')
        self.assertRaises(RuntimeError, ic.getInstrRun, ws.name())

    def test_getWSprefix_ISIS(self):
        config['default.facility'] = 'ISIS'
        ws = Load(Filename='IRS21360.raw', OutputWorkspace='IRS21360')
        ws = self.load_instrument(ws,'IRIS')
        ws_name = ic.getWSprefix(ws.name())

        self.assertEqual(ws_name, 'irs21360_graphite002_')

    def test_getWSprefix_ILL(self):
        config['default.facility'] = 'ILL'

        ws = Load(Filename='ILLIN16B_034745.nxs', OutputWorkspace='ILLIN16B_034745')
        ws_name = ic.getWSprefix(ws.name())

        self.assertEqual(ws_name, 'in16b_34745_')

    def test_getEFixed(self):
        ws = CreateSampleWorkspace()
        ws = self.load_instrument(ws,'IRIS')

        e_fixed = ic.getEfixed(ws.name())
        self.assertEqual(e_fixed, 1.8450)

    def test_getEFixed_failure(self):
        ws = CreateSampleWorkspace()
        self.assertRaises(IndexError, ic.getEfixed, ws.name())

    #-----------------------------------------------------------
    # Test helper functions
    #-----------------------------------------------------------

    def load_instrument(self, ws, instrument):
        """Load an instrument parameter from the ipf directory"""
        LoadInstrument(ws, InstrumentName=instrument)
        ipf = os.path.join(config['instrumentDefinition.directory'], 
                           instrument+'_graphite_002_Parameters.xml')
        LoadParameterFile(ws, Filename=ipf)
        return ws



if __name__=="__main__":
    unittest.main()