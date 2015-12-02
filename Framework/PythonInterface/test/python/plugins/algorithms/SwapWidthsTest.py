import unittest
from mantid.simpleapi import *
from mantid.api import MatrixWorkspace, WorkspaceGroup

class SwapWidthsTest(unittest.TestCase):

    _input_ws = None


    def setUp(self):
        self._input_ws = Load(Filename='IN16B_125878_QLd_Result.nxs')


    def _validate_result(self, result):
        """
        Validates that the result workspace is of the correct type, units and shape.

        @param result Result workspace from SwapWidths algorithm
        """

        self.assertTrue(isinstance(result, MatrixWorkspace))
        self.assertEquals(result.getNumberHistograms(), 2)
        self.assertEquals(result.blocksize(), self._input_ws.getNumberHistograms())
        self.assertEquals(result.getAxis(0).getUnit().unitID(), 'MomentumTransfer')
        self.assertEquals(result.getAxis(1).label(0), 'f2.f1.FWHM')
        self.assertEquals(result.getAxis(1).label(1), 'f2.f2.FWHM')


    def test_basic(self):
        """
        Tests a basic run of SwapWidths.
        """
        result = SwapWidths(InputWorkspace=self._input_ws,
                            SwapPoint=5)
        self._validate_result(result)

if __name__=="__main__":
    unittest.main()
