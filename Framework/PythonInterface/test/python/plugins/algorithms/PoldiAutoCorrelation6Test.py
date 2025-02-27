# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.simpleapi import PoldiAutoCorrelation, AnalysisDataService
from mantid.api import FileFinder
from plugins.algorithms.poldi_utils import load_poldi


class PoldiAutoCorrelation6Test(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # load silicon data for 448x500 IDF with chopperspeed 5000 rpm
        fpath_data = FileFinder.getFullPath("poldi_448x500_chopper5k_silicon.txt")
        fpath_idf = FileFinder.getFullPath("POLDI_Definition_448_calibrated.xml")

        # load the raw data
        cls.ws = load_poldi(fpath_data, fpath_idf, chopper_speed=5000, t0=5.855e-02, t0_const=-9.00)

    @classmethod
    def tearDownClass(cls):
        AnalysisDataService.clear()

    def test_exec_default_wavelength_range(self):
        ws_corr = PoldiAutoCorrelation(InputWorkspace=self.ws, OutputWorkspace="ws_corr", Version=6)

        # assert min/max Q
        self.assertAlmostEqual(ws_corr.readX(0)[0], 1.5144, delta=1e-3)
        self.assertAlmostEqual(ws_corr.readX(0)[-1], 9.0832, delta=1e-3)
        # assert bin width/number bins
        self.assertEqual(ws_corr.blocksize(), 2460)
        # assert max y-value at Bragg peak Q
        _, imax = ws_corr.findY(ws_corr.readY(0).max())
        self.assertAlmostEqual(ws_corr.readX(0)[imax], 3.272, delta=1e-2)  # (220) peak @ d = 1.920 Ang

    def test_exec_cropped_wavelength_range(self):
        ws_corr = PoldiAutoCorrelation(InputWorkspace=self.ws, OutputWorkspace="ws_corr", WavelengthMin=2, WavelengthMax=4, Version=6)

        # assert min/max Q
        self.assertAlmostEqual(ws_corr.readX(0)[0], 1.8930, delta=1e-3)
        self.assertAlmostEqual(ws_corr.readX(0)[-1], 4.9958, delta=1e-3)
        # assert bin width/number bins
        self.assertEqual(ws_corr.blocksize(), 1470)


if __name__ == "__main__":
    unittest.main()
