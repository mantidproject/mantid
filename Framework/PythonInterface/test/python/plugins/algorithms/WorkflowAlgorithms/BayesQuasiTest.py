# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import numpy as np
from mantid.simpleapi import *
from mantid.api import MatrixWorkspace, WorkspaceGroup


class BayesQuasiTest(unittest.TestCase):
    _res_ws = None
    _sample_ws = None
    _resnorm_ws = None
    _num_bins = None
    _num_hists = None

    def setUp(self):
        self._res_ws = Load(Filename="irs26173_graphite002_res.nxs", OutputWorkspace="__BayesQuasiTest_Resolution")
        self._sample_ws = Load(Filename="irs26176_graphite002_red.nxs", OutputWorkspace="__BayesQuasiTest_Sample")
        self._resnorm_ws = Load(Filename="irs26173_graphite002_ResNorm.nxs", OutputWorkspace="irs26173_graphite002_ResNorm")
        self._num_bins = self._sample_ws.blocksize()
        self._num_hists = self._sample_ws.getNumberHistograms()

    def tearDown(self):
        """
        Remove workspaces from ADS.
        """
        DeleteWorkspace(self._sample_ws)
        DeleteWorkspace(self._res_ws)
        DeleteWorkspace(self._resnorm_ws)

    # ----------------------------------Algorithm tests----------------------------------------

    def test_QLr_Run(self):
        """
        Test Lorentzian fit for BayesQuasi
        """
        fit_group, result, prob = BayesQuasi(
            Program="QL",
            SampleWorkspace=self._sample_ws,
            ResolutionWorkspace=self._res_ws,
            MinRange=-0.547607,
            MaxRange=0.543216,
            SampleBins=1,
            ResolutionBins=1,
            Elastic=False,
            Background="Sloping",
            FixedWidth=False,
            UseResNorm=False,
            WidthFile="",
            Loop=True,
        )
        self._validate_QLr_shape(result, prob, fit_group)
        self._validate_Qlr_value(result, prob, fit_group)

    def test_QSe_Run(self):
        """
        Test Stretched Exponential fit for BayesQuasi
        """
        fit_group, result = BayesQuasi(
            Program="QSe",
            SampleWorkspace=self._sample_ws,
            ResolutionWorkspace=self._res_ws,
            MinRange=-0.547607,
            MaxRange=0.543216,
            SampleBins=1,
            ResolutionBins=1,
            Elastic=False,
            Background="Sloping",
            FixedWidth=False,
            UseResNorm=False,
            WidthFile="",
            Loop=True,
        )
        self._validate_QSe_shape(result, fit_group)
        self._validate_QSe_value(result, fit_group)

    def test_run_with_resNorm_file(self):
        """
        Test a simple lorentzian fit with a ResNorm file
        """
        fit_group, result, prob = BayesQuasi(
            Program="QL",
            SampleWorkspace=self._sample_ws,
            ResolutionWorkspace=self._res_ws,
            ResNormWorkspace=self._resnorm_ws,
            MinRange=-0.547607,
            MaxRange=0.543216,
            SampleBins=1,
            ResolutionBins=1,
            Elastic=False,
            Background="Sloping",
            FixedWidth=False,
            UseResNorm=True,
            WidthFile="",
            Loop=True,
        )
        self._validate_QLr_shape(result, prob, fit_group)
        self._validate_QLr_value_with_resnorm(result, prob, fit_group)

    def test_run_with_zero_at_the_end_of_data(self):
        """
        Test that the algorithm handles data with appended zeros correctly
        """
        sample = self._create_sample_with_trailing_zero()
        fit_group, result, prob = BayesQuasi(
            SampleWorkspace=sample, ResolutionWorkspace=self._res_ws, MinRange=-0.54, MaxRange=0.50, FixedWidth=False
        )

        self.assertTrue(isinstance(fit_group, WorkspaceGroup))
        self.assertTrue(isinstance(result, MatrixWorkspace))
        self.assertTrue(isinstance(prob, MatrixWorkspace))

    # --------------------------------Validate results------------------------------------------------

    def _validate_QLr_shape(self, result, probability, group):
        """
        Validates that the output workspaces are of the correct type, units and shape.

        @param result Result workspace from BayesQuasi
        @param prob Probability workspace from BayesQuasi
        @param group Group workspace of fitted spectra from BayesQuasi
        """

        # Test size/shape of result
        self.assertTrue(isinstance(result, MatrixWorkspace))
        self.assertEqual(result.getNumberHistograms(), 21)
        self.assertEqual(result.blocksize(), self._num_hists)
        self.assertEqual(result.getAxis(0).getUnit().unitID(), "MomentumTransfer")

        # Test size/shape of probability
        self.assertTrue(isinstance(probability, MatrixWorkspace))
        self.assertEqual(probability.getNumberHistograms(), 4)
        self.assertEqual(probability.blocksize(), self._num_hists)
        self.assertEqual(result.getAxis(0).getUnit().unitID(), "MomentumTransfer")

        # Test size/shape of group fitting workspaces
        self.assertTrue(isinstance(group, WorkspaceGroup))
        self.assertEqual(group.getNumberOfEntries(), self._sample_ws.getNumberHistograms())

        # Test sub workspaces
        for i in range(group.getNumberOfEntries()):
            sub_ws = group.getItem(i)
            self.assertTrue(isinstance(sub_ws, MatrixWorkspace))
            self.assertEqual(sub_ws.getNumberHistograms(), 7)
            self.assertEqual(sub_ws.getAxis(0).getUnit().unitID(), "DeltaE")

    def _validate_Qlr_value(self, result, probability, group):
        """
        Validates that the output workspaces have expected values
        with values from the last known correct version

        @param result Result workspace from BayesQuasi
        @param prob Probability workspace from BayesQuasi
        @param group Group workspace of fitted spectra from BayesQuasi
        """

        # Test values of result
        self.assertAlmostEqual(result.dataY(0)[0], 6.06105, delta=1e-4)
        self.assertAlmostEqual(result.dataY(1)[0], 68.5744, delta=1e-4)
        self.assertAlmostEqual(result.dataY(2)[0], 0.0589315, delta=1e-4)
        self.assertAlmostEqual(result.dataY(3)[0], 0.0812087, delta=1e-4)

        # Test values of probability
        self.assertAlmostEqual(probability.dataY(0)[0], -74176.1, delta=1e-1)
        self.assertAlmostEqual(probability.dataY(1)[0], -404.884, delta=1e-2)
        self.assertAlmostEqual(probability.dataY(2)[0], -0.222565, delta=1e-2)

        # Test values of group
        sub_ws = group.getItem(0)
        self.assertAlmostEqual(sub_ws.dataY(0)[0], 0.02540, delta=1e-5)
        self.assertAlmostEqual(sub_ws.dataY(1)[0], 0.01903, delta=1e-5)
        self.assertAlmostEqual(sub_ws.dataY(2)[0], -0.00638, delta=1e-5)
        self.assertAlmostEqual(sub_ws.dataY(3)[0], 0.01614, delta=1e-5)
        self.assertAlmostEqual(sub_ws.dataY(4)[0], -0.00926, delta=1e-5)

    def _validate_QLr_value_with_resnorm(self, result, probability, group):
        """
        Validates that the output workspaces have the expected values
        with values from the last known correct version
        @param result Result workspace from BayesQuasi
        @param prob Probability workspace from BayesQuasi
        @param group Group workspace of fitted spectra from BayesQuasi
        """

        # Test values of result
        self.assertAlmostEqual(result.dataY(0)[0], 153.471, delta=1e-3)
        self.assertAlmostEqual(result.dataY(1)[0], 1785.06, delta=1e-2)
        self.assertAlmostEqual(result.dataY(2)[0], 0.0588549, delta=1e-4)
        self.assertAlmostEqual(result.dataY(3)[0], 0.0791689, delta=1e-4)

        # Test values of probability
        self.assertAlmostEqual(probability.dataY(0)[0], -74887.1, delta=1e-1)
        self.assertAlmostEqual(probability.dataY(1)[0], -407.593, delta=1e-2)
        self.assertAlmostEqual(probability.dataY(2)[0], -0.480316, delta=1e-2)

        # Test values of group
        sub_ws = group.getItem(0)
        self.assertAlmostEqual(sub_ws.dataY(0)[0], 0.652046, delta=1e-4)
        self.assertAlmostEqual(sub_ws.dataY(1)[0], 0.48846, delta=1e-4)
        self.assertAlmostEqual(sub_ws.dataY(2)[0], -0.163586, delta=1e-4)
        self.assertAlmostEqual(sub_ws.dataY(3)[0], 0.414406, delta=1e-4)
        self.assertAlmostEqual(sub_ws.dataY(4)[0], -0.23764, delta=1e-4)

    def _validate_QSe_shape(self, result, group):
        """
        Validates that the output workspaces are of the correct type, units and shape.
        with values from the last known correct version
        @param result Result workspace from BayesQuasi
        @param group Group workspace of fitted spectra from BayesQuasi
        """

        # Test size/shape of result
        self.assertTrue(isinstance(result, MatrixWorkspace))
        self.assertEqual(result.getNumberHistograms(), 3)
        self.assertEqual(result.blocksize(), self._num_hists)
        self.assertEqual(result.getAxis(0).getUnit().unitID(), "MomentumTransfer")

        # Test size/shape of group fitting workspaces
        self.assertTrue(isinstance(group, WorkspaceGroup))
        self.assertEqual(group.getNumberOfEntries(), self._sample_ws.getNumberHistograms())

        # Test sub workspaces
        for i in range(group.getNumberOfEntries()):
            sub_ws = group.getItem(i)
            self.assertTrue(isinstance(sub_ws, MatrixWorkspace))
            self.assertEqual(sub_ws.getNumberHistograms(), 3)
            self.assertEqual(sub_ws.getAxis(0).getUnit().unitID(), "DeltaE")

    def _validate_QSe_value(self, result, group):
        """
        Validates that the output workspaces have expected values

        @param result Result workspace from BayesQuasi
        @param prob Probability workspace from BayesQuasi
        @param group Group workspace of fitted spectra from BayesQuasi
        """

        # Test values of result
        self.assertAlmostEqual(result.dataY(0)[0], 81.12644, delta=1e-4)
        self.assertAlmostEqual(result.dataY(1)[0], 0.0319747, delta=1e-4)
        self.assertAlmostEqual(result.dataY(2)[0], 0.77168, delta=1e-4)

        # Test values of group
        sub_ws = group.getItem(0)
        self.assertAlmostEqual(sub_ws.dataY(0)[0], 0.02540, delta=1e-4)
        self.assertAlmostEqual(sub_ws.dataY(1)[0], 0.01632, delta=1e-4)
        self.assertAlmostEqual(sub_ws.dataY(2)[0], -0.00908, delta=1e-4)

    # --------------------------------Helper functions--------------------------------------

    def _create_sample_with_trailing_zero(self):
        """
        Creates a sample and resolution workspace that have an additional trailing 0

        """
        x_data = np.append(self._sample_ws.readX(0), 0.5500)
        y_data1 = np.append(self._sample_ws.readY(0), 0)
        y_data2 = np.append(self._sample_ws.readY(1), 0)
        y_data = np.concatenate((y_data1, y_data2), axis=0)
        e_data1 = np.append(self._sample_ws.readE(0), 0)
        e_data2 = np.append(self._sample_ws.readE(1), 0)
        e_data = np.concatenate((e_data1, e_data2), axis=0)
        sample = CreateWorkspace(x_data, y_data, e_data, NSpec=2, ParentWorkspace=self._sample_ws)
        sample.getSpectrum(0).setDetectorID(1)
        sample.getSpectrum(1).setDetectorID(2)
        return sample


if __name__ == "__main__":
    unittest.main()
