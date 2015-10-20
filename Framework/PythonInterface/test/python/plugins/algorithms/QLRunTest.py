import unittest
from mantid.simpleapi import *
from mantid.api import MatrixWorkspace, WorkspaceGroup

class QLRunTest(unittest.TestCase):

    _res_ws = None
    _sample_ws = None
    _num_bins = None

    def setUp(self):
        self._res_ws = Load(Filename='irs26173_graphite002_res.nxs',
                            OutputWorkspace='__QLRunTest_Resolution')
        self._sample_ws = Load(Filename='irs26173_graphite002_red.nxs',
                            OutputWorkspace='__QLRunTest_Vanadium')
        self._num_bins = self._sample_ws.blocksize()
        
                            
    def _validate_QLr_result(self, result, probability, group):
        """
        Validates that the result workspace is of the correct type, units and shape.

        @param result Result workspace from QLRun
        @param prob Probability workspace from QLRun
        @param group Group workspace of fitted spectra from QLRun
        """


        # Test size/shape of result
        self.assertTrue(isinstance(result, MatrixWorkspace))
        self.assertEquals(result.getNumberHistograms(), 21)
        self.assertEquals(result.blocksize(), self._num_bins)
        self.assertEquals(result.getAxis(0).getUnit().unitID(), 'q')

        # Test size/shape of probability
        self.assertTrue(isinstance(probability, MatrixWorkspace))
        self.assertEquals(probability.getNumberHistograms(), 3)
        self.assertEquals(probability.blocksize(), self._num_bins)
        self.assertEquals(result.getAxis(0).getUnit().unitID(), 'q')

        # Test size/shape of group fitting workspaces
        self.assertTrue(isinstance(group, WorkspaceGroup))
        self.assertEquals(group.getNumberOfEntries(), self._sample_ws.getNumberHistograms())

        # Test sub workspaces
        for i in range (group.getNumberOfEntries()):
            sub_ws = group.getItem(i)
            self.assertTrue(isinstance(sub_ws, MatrixWorkspace))
            self.assertEqual(sub_ws.getNumberHistograms(), 5)
            self.assertEqual(sub_ws.blocksize(), self._num_bins)
            self.assertEquals(sub_ws.getAxis(0).getUnit().unitID(), 'MomentumTransfer')


    def _validate_QSe_result(self, result, group):
        """
        Validates that the result workspace is of the correct type, units and shape.

        @param result Result workspace from QLRun
        @param group Group workspace of fitted spectra from QLRun
        """


        # Test size/shape of result
        self.assertTrue(isinstance(result, MatrixWorkspace))
        self.assertEquals(result.getNumberHistograms(), 21)
        self.assertEquals(result.blocksize(), self._num_bins)
        self.assertEquals(result.getAxis(0).getUnit().unitID(), 'q')

        # Test size/shape of group fitting workspaces
        self.assertTrue(isinstance(group, WorkspaceGroup))
        self.assertEquals(group.getNumberOfEntries(), self._sample_ws.getNumberHistograms())

        # Test sub workspaces
        for i in range (group.getNumberOfEntries()):
            sub_ws = group.getItem(i)
            self.assertTrue(isinstance(sub_ws, MatrixWorkspace))
            self.assertEqual(sub_ws.getNumberHistograms(), 3)
            self.assertEqual(sub_ws.blocksize(), self._num_bins)
            self.assertEquals(sub_ws.getAxis(0).getUnit().unitID(), 'MomentumTransfer')
            
    def test_QLr_Run(self):
        """
        Test Lorentzian fit for QLRun
        """
        fit_group, prob, result = QLRun(Program='QL',
                                        SampleWorkspace=self._sample_ws,
                                        ResolutionWorkspace=self._res_ws,
                                        MinRange=-0.547607,
                                        MaxRange=0.543216,
                                        SampleBins=1,
                                        ResolutionBins=1,
                                        Elastic=False,
                                        Background='Sloping'
                                        FixedWidth=False,
                                        UseResNorm=False,
                                        WidthFile='',
                                        Loop=True,
                                        Save=False,
                                        Plot='None')
        self._validate_QLr_result(result, prob, fit_group)

if __name__=="__main__":
    unittest.main()