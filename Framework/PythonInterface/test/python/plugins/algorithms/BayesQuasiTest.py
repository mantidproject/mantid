import unittest
import platform
from mantid.simpleapi import *
from mantid.api import MatrixWorkspace, WorkspaceGroup

if platform.system() == "Windows":
    class BayesQuasiTest(unittest.TestCase):

        _res_ws = None
        _sample_ws = None
        _num_bins = None
        _num_hists = None

        def setUp(self):
            self._res_ws = Load(Filename='irs26173_graphite002_res.nxs',
                                OutputWorkspace='__BayesQuasiTest_Resolution')
            self._sample_ws = Load(Filename='irs26176_graphite002_red.nxs',
                                OutputWorkspace='__BayesQuasiTest_Sample')
            self._num_bins = self._sample_ws.blocksize()
            self._num_hists = self._sample_ws.getNumberHistograms()


        def _validate_QLr_shape(self, result, probability, group):
            """
            Validates that the output workspaces are of the correct type, units and shape.

            @param result Result workspace from BayesQuasi
            @param prob Probability workspace from BayesQuasi
            @param group Group workspace of fitted spectra from BayesQuasi
            """

            # Test size/shape of result
            self.assertTrue(isinstance(result, MatrixWorkspace))
            self.assertEquals(result.getNumberHistograms(), 21)
            self.assertEquals(result.blocksize(), self._num_hists)
            self.assertEquals(result.getAxis(0).getUnit().unitID(), 'MomentumTransfer')

            # Test size/shape of probability
            self.assertTrue(isinstance(probability, MatrixWorkspace))
            self.assertEquals(probability.getNumberHistograms(), 3)
            self.assertEquals(probability.blocksize(), self._num_hists)
            self.assertEquals(result.getAxis(0).getUnit().unitID(), 'MomentumTransfer')

            # Test size/shape of group fitting workspaces
            self.assertTrue(isinstance(group, WorkspaceGroup))
            self.assertEquals(group.getNumberOfEntries(), self._sample_ws.getNumberHistograms())

            # Test sub workspaces
            for i in range (group.getNumberOfEntries()):
                sub_ws = group.getItem(i)
                self.assertTrue(isinstance(sub_ws, MatrixWorkspace))
                self.assertEqual(sub_ws.getNumberHistograms(), 5)
                self.assertEquals(sub_ws.getAxis(0).getUnit().unitID(), 'DeltaE')


        def _validate_Qlr_value(self, result, probability, group):
            """
            Validates that the output workspaces have expected values
            with values from the last known correct version

            @param result Result workspace from BayesQuasi
            @param prob Probability workspace from BayesQuasi
            @param group Group workspace of fitted spectra from BayesQuasi
            """

            # Test values of result
            result_y = result.dataY(0)
            self.assertEquals(round(result.dataY(0)[0], 5), 0.92237)
            self.assertEquals(round(result.dataY(1)[0], 4), 6.9651)
            self.assertEquals(round(result.dataY(2)[0], 7), 0.0620143)
            self.assertEquals(round(result.dataY(3)[0], 7), 0.1169424)

            # Test values of probability
            prob_y = probability.dataY(0)
            self.assertEquals(round(probability.dataY(0)[0], 1), -65487.5)
            self.assertEquals(round(probability.dataY(1)[0], 3), -375.124)
            self.assertEquals(round(probability.dataY(2)[0], 6), 0)

            # Test values of group
            sub_ws = group.getItem(0)
            sub_y = sub_ws.dataY(0)
            self.assertEquals(round(sub_ws.dataY(0)[0], 5), 0.02540)
            self.assertEquals(round(sub_ws.dataY(1)[0], 5), 0.01887)
            self.assertEquals(round(sub_ws.dataY(2)[0], 5), -0.00653)
            self.assertEquals(round(sub_ws.dataY(3)[0], 5), 0.01605)
            self.assertEquals(round(sub_ws.dataY(4)[0], 5), -0.00935)


        def _validate_QSe_shape(self, result, group):
            """
            Validates that the output workspaces are of the correct type, units and shape.
            with values from the last known correct version

            @param result Result workspace from BayesQuasi
            @param group Group workspace of fitted spectra from BayesQuasi
            """

            # Test size/shape of result
            self.assertTrue(isinstance(result, MatrixWorkspace))
            self.assertEquals(result.getNumberHistograms(), 3)
            self.assertEquals(result.blocksize(), self._num_hists)
            self.assertEquals(result.getAxis(0).getUnit().unitID(), 'MomentumTransfer')

            # Test size/shape of group fitting workspaces
            self.assertTrue(isinstance(group, WorkspaceGroup))
            self.assertEquals(group.getNumberOfEntries(), self._sample_ws.getNumberHistograms())

            # Test sub workspaces
            for i in range (group.getNumberOfEntries()):
                sub_ws = group.getItem(i)
                self.assertTrue(isinstance(sub_ws, MatrixWorkspace))
                self.assertEqual(sub_ws.getNumberHistograms(), 3)
                self.assertEquals(sub_ws.getAxis(0).getUnit().unitID(), 'DeltaE')


        def _validate_QSe_value(self, result, group):
            """
            Validates that the output workspaces have expected values

            @param result Result workspace from BayesQuasi
            @param prob Probability workspace from BayesQuasi
            @param group Group workspace of fitted spectra from BayesQuasi
            """

            # Test values of result
            result_y = result.dataY(0)
            self.assertEquals(round(result.dataY(0)[0], 5), 8.28044)
            self.assertEquals(round(result.dataY(1)[0], 7), 0.0335993)
            self.assertEquals(round(result.dataY(2)[0], 5), 0.77844)

            # Test values of group
            sub_ws = group.getItem(0)
            sub_y = sub_ws.dataY(0)
            self.assertEquals(round(sub_ws.dataY(0)[0], 5), 0.02540)
            self.assertEquals(round(sub_ws.dataY(1)[0], 5), 0.01656)
            self.assertEquals(round(sub_ws.dataY(2)[0], 5), -0.00884)


        def test_QLr_Run(self):
            """
            Test Lorentzian fit for BayesQuasi
            """
            fit_group, result, prob= BayesQuasi(Program='QL',
                                              SampleWorkspace=self._sample_ws,
                                              ResolutionWorkspace=self._res_ws,
                                              MinRange=-0.547607,
                                              MaxRange=0.543216,
                                              SampleBins=1,
                                              ResolutionBins=1,
                                              Elastic=False,
                                              Background='Sloping',
                                              FixedWidth=False,
                                              UseResNorm=False,
                                              WidthFile='',
                                              Loop=True,
                                              Save=False,
                                              Plot='None')
            self._validate_QLr_shape(result, prob, fit_group)
            self._validate_Qlr_value(result, prob, fit_group)


        def test_QSe_Run(self):
            """
            Test Stretched Exponential fit for BayesQuasi
            """
            fit_group, result = BayesQuasi(Program='QSe',
                                      SampleWorkspace=self._sample_ws,
                                      ResolutionWorkspace=self._res_ws,
                                      MinRange=-0.547607,
                                      MaxRange=0.543216,
                                      SampleBins=1,
                                      ResolutionBins=1,
                                      Elastic=False,
                                      Background='Sloping',
                                      FixedWidth=False,
                                      UseResNorm=False,
                                      WidthFile='',
                                      Loop=True,
                                      Save=False,
                                      Plot='None')
            self._validate_QSe_shape(result, fit_group)
            self._validate_QSe_value(result, fit_group)

    if __name__=="__main__":
        unittest.main()
