import unittest

from mantid.kernel import *
from mantid.api import *
from testhelpers import run_algorithm

class ConjoinSpectraTest(unittest.TestCase):

    _aWS= None

    def setUp(self):
            dataX = range(0,6)
            dataY = range(0,5)
            dataE = [1] * 5

            createWSAlg = run_algorithm( "CreateWorkspace", DataX=dataX, DataY=dataY, DataE=dataE, NSpec=1,UnitX="Wavelength", OutputWorkspace="single_spectra_ws")
            self._aWS = createWSAlg.getPropertyValue("OutputWorkspace")


    def test_basicRun(self):
        conjoinAlg = run_algorithm( "ConjoinSpectra", InputWorkspaces="%s,%s" % (self._aWS, self._aWS), OutputWorkspace="conjoined" )
        conjoinedWS = mtd.retrieve('conjoined')

        wsIndex = 0
        inDataY = mtd[self._aWS].readY(wsIndex)
        inDataE = mtd[self._aWS].readE(wsIndex)
        print type(conjoinedWS)
        outDataY1 = conjoinedWS.readY(0)
        outDataY2 = conjoinedWS.readY(1)
        outDataE1 = conjoinedWS.readE(0)
        outDataE2 = conjoinedWS.readE(1)

        # Check output shape
        self.assertEqual(len(inDataY), len(outDataY1))
        self.assertEqual(len(inDataY), len(outDataY2))
        self.assertEqual(len(inDataE), len(outDataE1))
        self.assertEqual(len(inDataE), len(outDataE2))
        self.assertEqual(2, conjoinedWS.getNumberHistograms()) # Should always have 2 histograms

        self.assertEquals(set(inDataY), set(outDataY1))
        self.assertEquals(set(inDataY), set(outDataY2))
        self.assertEquals(set(inDataE), set(outDataE1))
        self.assertEquals(set(inDataE), set(outDataE2))


if __name__ == '__main__':
    unittest.main()
