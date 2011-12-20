import unittest
###############################################################################
# This has to be tested through a workspace as it cannot be created in 
# Python
###############################################################################
from testhelpers import run_algorithm
from mantid.geometry import Instrument
from mantid.api import Sample, Run

class ExperimentInfoTest(unittest.TestCase):
  
    _expt_ws = None
  
    def setUp(self):
        if self.__class__._expt_ws is None:
            alg = run_algorithm('Load', Filename='LOQ48127.raw', SpectrumMax=1, child=True)
            self.__class__._expt_ws = alg.getProperty("OutputWorkspace").value
  
    def test_information_access(self):
        inst = self._expt_ws.getInstrument()
        self.assertTrue(isinstance(inst, Instrument))
        self.assertEquals(self._expt_ws.getRunNumber(), 48127)
        
    def test_sample_access_returns_sample_object(self):
        sample = self._expt_ws.sample()
        self.assertTrue(isinstance(sample, Sample))
        
    def test_run_access_returns_run_object(self):
        run = self._expt_ws.run()
        self.assertTrue(isinstance(run, Run))