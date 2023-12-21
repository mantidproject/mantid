# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from mantid.simpleapi import DeleteWorkspace, HFIRGoniometerIndependentBackground, CreateMDHistoWorkspace
import numpy as np
import scipy.ndimage


class HFIRGoniometerIndependentBackgroundTest(unittest.TestCase):
    def setUp(self):
        self.signal = np.random.random((100, 100, 100))
        _max = self.signal.max()
        _min = self.signal.min()

        self.workspace = CreateMDHistoWorkspace(
            SignalInput=self.signal,
            ErrorInput=np.ones_like(self.signal),
            Dimensionality=3,
            Extents=f"{_min},{_max},{_min},{_max},{_min},{_max}",
            Names="x,y,z",
            NumberOfBins="100,100,100",
            Units="number,number,number",
            OutputWorkspace="input",
        )

    def tearDown(self):
        DeleteWorkspace(self.workspace)

    def test_generate_background_pf(self):
        signal = self.workspace.getSignalArray().copy()
        expected = scipy.ndimage.percentile_filter(signal, 50, size=(1, 1, 25), mode="nearest")

        outputWS = HFIRGoniometerIndependentBackground(self.workspace, BackgroundLevel=50, BackgroundWindowSize=25)
        result = outputWS.getSignalArray().copy()
        self.assertTrue(np.array_equal(expected, result))

    def test_generate_background_np(self):
        signal = self.workspace.getSignalArray().copy()
        bkg = np.percentile(signal, 50, axis=2)
        expected = np.repeat(bkg[:, :, np.newaxis], signal.shape[2], axis=2)

        outputWS = HFIRGoniometerIndependentBackground(self.workspace)
        result = outputWS.getSignalArray().copy()
        self.assertTrue(np.array_equal(expected, result))


if __name__ == "__main__":
    unittest.main()
