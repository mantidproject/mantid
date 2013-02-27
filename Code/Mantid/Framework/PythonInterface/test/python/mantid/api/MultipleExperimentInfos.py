import unittest
###############################################################################
# This has to be tested through a workspace as it cannot be created in 
# Python
###############################################################################
from testhelpers import run_algorithm
from mantid.api import ExperimentInfo

class MultipleExperimentInfoTest(unittest.TestCase):
  
    _expt_ws = None
  
    def setUp(self):
        if self.__class__._expt_ws is None:
            alg = run_algorithm('Load', Filename='TOPAZ_3680_5_sec_MDEW.nxs', child=True)
            self.__class__._expt_ws = alg.getProperty("OutputWorkspace").value
  
    def test_information_access(self):
        expinfo = self._expt_ws.getExperimentInfo(0)
        self.assertTrue(isinstance(expinfo, ExperimentInfo))
	self.assertEquals(1, self._expt_ws.getNumExperimentInfo())

