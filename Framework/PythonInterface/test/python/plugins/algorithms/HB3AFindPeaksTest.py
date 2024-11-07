# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.simpleapi import ClearUB, HasUB, FindPeaksMD, HB3AAdjustSampleNorm, HB3AFindPeaks, mtd


class HB3AFindPeaksTest(unittest.TestCase):
    _files = ["HB3A_exp0724_scan0182.nxs", "HB3A_exp0724_scan0183.nxs"]
    _data_ws = None

    @classmethod
    def tearDownClass(cls):
        mtd.clear()

    def __load_data(self):
        # Loads in the testing files to workspaces to reduce testing overhead for multiple tests
        if self._data_ws is None:
            name = "HB3A_exp0724"
            HB3AAdjustSampleNorm(Filename=",".join(f for f in self._files), OutputWorkspace=name, NormaliseBy="None")
            self._data_ws = name

    def test_find_ub_peaks(self):
        # Make sure that this algorithm gives similar results to FindPeaksMD and creates a UB matrix

        self.__load_data()

        # Remove the UB matrix from the input data
        ClearUB(mtd[self._data_ws])

        peaks_md = FindPeaksMD(
            InputWorkspace=mtd[self._data_ws],
            PeakDistanceThreshold=0.25,
            DensityThresholdFactor=2000,
            CalculateGoniometerForCW=True,
            Wavelength=1.008,
            FlipX=True,
            InnerGoniometer=True,
        )

        peaks = HB3AFindPeaks(InputWorkspace=mtd[self._data_ws], CellType="Orthorhombic", Centering="F")

        # Verify that the algorithm found a UB matrix
        for i in range(peaks.size()):
            self.assertTrue(HasUB(peaks[i]))
            self.assertEqual(peaks_md[i].getNumberPeaks(), peaks[i].getNumberPeaks())

    def test_find_ub_lattice(self):
        # Test if the algorithm finds a UB matrix based on given lattice parameters

        self.__load_data()

        # Remove the UB matrix from the input data
        ClearUB(mtd[self._data_ws])

        peaks = HB3AFindPeaks(
            InputWorkspace=mtd[self._data_ws],
            CellType="Orthorhombic",
            Centering="F",
            UseLattice=True,
            LatticeA=5.2384,
            LatticeB=5.2384,
            LatticeC=19.6519,
            LatticeAlpha=90.0,
            LatticeBeta=90.0,
            LatticeGamma=90.0,
        )

        # Verify that the algorithm found a UB matrix from lattice params
        for ws in peaks:
            self.assertTrue(HasUB(ws))
            # Check that peaks were actually found
            self.assertTrue(ws.getNumberPeaks() > 0)


if __name__ == "__main__":
    unittest.main()
