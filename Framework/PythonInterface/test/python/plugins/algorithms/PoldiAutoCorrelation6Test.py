# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from numpy.testing import assert_array_almost_equal, assert_array_equal
import numpy as np
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
        self._assert_auto_corr_workspace(ws_corr)

    def test_exec_nearest_interpolation(self):
        ws_corr = PoldiAutoCorrelation(InputWorkspace=self.ws, OutputWorkspace="ws_corr_nearest", InterpolationMethod="Nearest", Version=6)
        self._assert_auto_corr_workspace(ws_corr)

    def test_exec_cropped_wavelength_range(self):
        ws_corr = PoldiAutoCorrelation(InputWorkspace=self.ws, OutputWorkspace="ws_corr", WavelengthMin=2, WavelengthMax=4, Version=6)

        # assert min/max Q
        self.assertAlmostEqual(ws_corr.readX(0)[0], 1.8930, delta=1e-3)
        self.assertAlmostEqual(ws_corr.readX(0)[-1], 4.9958, delta=1e-3)
        # assert bin width/number bins
        self.assertEqual(ws_corr.blocksize(), 1748)

    def test_exec_ngroups(self):
        ngroups = 3
        ws_corr = PoldiAutoCorrelation(InputWorkspace=self.ws, OutputWorkspace="ws_corr", NGroups=ngroups, Version=6)

        # assert two-theta of spectra
        self.assertEqual(ws_corr.getNumberHistograms(), ngroups)
        si = ws_corr.spectrumInfo()
        tths = np.degrees([si.twoTheta(ispec) for ispec in range(ws_corr.getNumberHistograms())])
        assert_array_almost_equal(tths, np.array([79.19, 89.73, 100.26]), decimal=2)
        # assert number fo detectors associated with each spectrum
        ndets = [len(ws_corr.getSpectrum(ispec).getDetectorIDs()) for ispec in range(ws_corr.getNumberHistograms())]
        assert_array_equal(ndets, np.array([150, 149, 149]))

    def _assert_auto_corr_workspace(self, ws_corr):
        # assert min/max Q
        self.assertAlmostEqual(ws_corr.readX(0)[0], 1.5144, delta=1e-3)
        self.assertAlmostEqual(ws_corr.readX(0)[-1], 9.0832, delta=1e-3)
        # assert bin width/number bins
        self.assertEqual(ws_corr.blocksize(), 2833)
        # assert max y-value at Bragg peak Q
        _, imax = ws_corr.findY(ws_corr.readY(0).max())
        self.assertAlmostEqual(ws_corr.readX(0)[imax], 3.272, delta=1e-2)  # (220) peak @ d = 1.920 Ang
        # assert spectra have detectorIDs
        self.assertTrue(ws_corr.spectrumInfo().hasDetectors(0))


if __name__ == "__main__":
    unittest.main()
