# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from numpy.testing import assert_array_almost_equal, assert_array_equal
import numpy as np
from mantid.simpleapi import PoldiAutoCorrelation, AnalysisDataService, CloneWorkspace, MaskDetectors
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

        # mask the first 20 spectra (lowest two-theta) to test masking support
        cls.nmasked = 20
        cls.ws_masked = cls._make_masked_ws("ws_masked", range(cls.nmasked))
        cls.masked_detids = set()
        for ispec in range(cls.nmasked):
            cls.masked_detids.update(cls.ws.getSpectrum(ispec).getDetectorIDs())

    @classmethod
    def _make_masked_ws(cls, output_workspace, ispecs_to_mask):
        ws_masked = CloneWorkspace(InputWorkspace=cls.ws, OutputWorkspace=output_workspace)
        MaskDetectors(Workspace=ws_masked, WorkspaceIndexList=list(ispecs_to_mask))
        return ws_masked

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
        self.assertAlmostEqual(ws_corr.x(0)[0], 1.8930, delta=1e-3)
        self.assertAlmostEqual(ws_corr.x(0)[-1], 4.9958, delta=1e-3)
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
        # assert number of detectors associated with each spectrum
        ndets = [len(ws_corr.getSpectrum(ispec).getDetectorIDs()) for ispec in range(ws_corr.getNumberHistograms())]
        assert_array_equal(ndets, np.array([150, 149, 149]))

    def test_exec_masked_spectra_excluded_from_dspacing_range(self):
        ws_corr = PoldiAutoCorrelation(InputWorkspace=self.ws_masked, OutputWorkspace="ws_corr_masked", Version=6)

        # the masked spectra have the lowest two-theta, so excluding them reduces the d-spacing
        # range considered (2833 bins when nothing is masked)
        self.assertEqual(ws_corr.blocksize(), 2823)
        self.assertTrue(np.all(np.isfinite(ws_corr.y(0))))

    def test_exec_masked_detectors_not_in_output_spectrum(self):
        ws_corr = PoldiAutoCorrelation(InputWorkspace=self.ws_masked, OutputWorkspace="ws_corr_masked", Version=6)

        detids = set(ws_corr.getSpectrum(0).getDetectorIDs())
        self.assertEqual(len(detids), self.ws.getNumberHistograms() - self.nmasked)
        self.assertFalse(detids & self.masked_detids)

    def test_exec_ngroups_grouping_mode_all(self):
        ngroups = 3
        ws_corr = PoldiAutoCorrelation(
            InputWorkspace=self.ws_masked, OutputWorkspace="ws_corr_masked", NGroups=ngroups, GroupingMode="All", Version=6
        )

        # group boundaries are fixed over the whole detector, so only the group containing the
        # masked spectra loses detectors (cf. [150, 149, 149] when nothing is masked)
        ndets = [len(ws_corr.getSpectrum(ispec).getDetectorIDs()) for ispec in range(ngroups)]
        assert_array_equal(ndets, np.array([150 - self.nmasked, 149, 149]))
        # two-theta of the first group increases as it has lost its lowest two-theta detectors
        si = ws_corr.spectrumInfo()
        tths = np.degrees([si.twoTheta(ispec) for ispec in range(ngroups)])
        assert_array_almost_equal(tths, np.array([79.87, 89.73, 100.26]), decimal=2)

    def test_exec_ngroups_grouping_mode_unmasked(self):
        ngroups = 3
        ws_corr = PoldiAutoCorrelation(
            InputWorkspace=self.ws_masked, OutputWorkspace="ws_corr_masked", NGroups=ngroups, GroupingMode="Unmasked", Version=6
        )

        # masked spectra are removed before splitting, so the unmasked detectors are shared evenly
        ndets = [len(ws_corr.getSpectrum(ispec).getDetectorIDs()) for ispec in range(ngroups)]
        assert_array_equal(ndets, np.array([143, 143, 142]))
        self.assertEqual(sum(ndets), self.ws.getNumberHistograms() - self.nmasked)
        si = ws_corr.spectrumInfo()
        tths = np.degrees([si.twoTheta(ispec) for ispec in range(ngroups)])
        assert_array_almost_equal(tths, np.array([80.33, 90.45, 100.51]), decimal=2)

    def test_exec_group_with_all_spectra_masked_is_zero(self):
        ngroups = 3
        # mask all spectra in the first group (groups are 150, 149, 149 spectra)
        ws_masked_group = self._make_masked_ws("ws_masked_group", range(150))
        ws_corr = PoldiAutoCorrelation(
            InputWorkspace=ws_masked_group, OutputWorkspace="ws_corr_masked_group", NGroups=ngroups, GroupingMode="All", Version=6
        )

        ndets = [len(ws_corr.getSpectrum(ispec).getDetectorIDs()) for ispec in range(ngroups)]
        assert_array_equal(ndets, np.array([0, 149, 149]))
        # the empty group produces a zero spectrum rather than an error or NaNs
        self.assertFalse(ws_corr.spectrumInfo().hasDetectors(0))
        assert_array_equal(ws_corr.y(0), np.zeros(ws_corr.blocksize()))
        self.assertTrue(np.any(ws_corr.y(1) > 0))

    def test_exec_raises_if_all_spectra_masked(self):
        ws_all_masked = self._make_masked_ws("ws_all_masked", range(self.ws.getNumberHistograms()))

        with self.assertRaisesRegex(RuntimeError, "fewer unmasked spectra"):
            PoldiAutoCorrelation(InputWorkspace=ws_all_masked, OutputWorkspace="ws_corr_all_masked", Version=6)

    def test_exec_raises_if_ngroups_exceeds_unmasked_spectra(self):
        nunmasked = self.ws.getNumberHistograms() - self.nmasked

        with self.assertRaisesRegex(RuntimeError, "fewer unmasked spectra"):
            PoldiAutoCorrelation(InputWorkspace=self.ws_masked, OutputWorkspace="ws_corr_masked", NGroups=nunmasked + 1, Version=6)

    def _assert_auto_corr_workspace(self, ws_corr):
        # assert min/max Q
        self.assertAlmostEqual(ws_corr.x(0)[0], 1.5144, delta=1e-3)
        self.assertAlmostEqual(ws_corr.x(0)[-1], 9.0832, delta=1e-3)
        # assert bin width/number bins
        self.assertEqual(ws_corr.blocksize(), 2833)
        # assert max y-value at Bragg peak Q
        _, imax = ws_corr.findY(ws_corr.y(0).max())
        self.assertAlmostEqual(ws_corr.x(0)[imax], 3.272, delta=1e-2)  # (220) peak @ d = 1.920 Ang
        # assert spectra have detectorIDs
        self.assertTrue(ws_corr.spectrumInfo().hasDetectors(0))


if __name__ == "__main__":
    unittest.main()
