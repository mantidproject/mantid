import unittest
###############################################################################
# This has to be tested through a workspace as it cannot be created in 
# Python
###############################################################################
from testhelpers import run_algorithm
from mantid.geometry import Instrument

class ExperimentInfoTest(unittest.TestCase):
  
    def test_information_access(self):
        alg = run_algorithm('Load', Filename='LOQ48127.raw', SpectrumMax=2, child=True)
        expt = alg.getProperty("OutputWorkspace")
        inst = expt.getInstrument()
        self.assertTrue(isinstance(Instrument))
        self.assertEquals(expt.getRunNumber(), 48127)