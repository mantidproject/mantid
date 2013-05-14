import unittest

from mantid.kernel import *
from mantid.api import *
from testhelpers import run_algorithm

class ConjoinSpectraTest(unittest.TestCase):

    _aWS= None

    def setUp(self):
            dataX = range(0,6)
            dataY = range(0,5)
            dataE = range(0,5)
            
            createWSAlg = run_algorithm( "CreateWorkspace", DataX=dataX, DataY=dataY, DataE=dataE, NSpec=1,UnitX="Wavelength", OutputWorkspace="single_spectra_ws") 
            self._aWS = createWSAlg.getPropertyValue("OutputWorkspace")
            
    def test_basicRun(self):
        conjoinAlg = run_algorithm( "ConjoinSpectra", InputWorkspaces="%s,%s" % (self._aWS, self._aWS), OutputWorkspace="conjoined" )
        spectrum_workspace = mtd.retrieve('conjoined')
        
        
if __name__ == '__main__':
    unittest.main()