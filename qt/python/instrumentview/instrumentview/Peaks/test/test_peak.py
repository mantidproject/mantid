# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from instrumentview.Peaks.Peak import Peak
import unittest


class TestPeak(unittest.TestCase):
    def test_label(self):
        peak = Peak(0, 0, (1.233333, 4.0, 36), 0, 0, 0, 0)
        self.assertEqual("(1.23, 4, 36)", peak.label)

    def test_location_in_unit(self):
        tof = 10
        wavelength = 15
        dspacing = 20
        q = 25
        peak = Peak(0, 0, (1.233333, 4.0, 36), tof, dspacing, wavelength, q)
        self.assertEqual(tof, peak.location_in_unit("TOF"))
        self.assertEqual(wavelength, peak.location_in_unit("WAVELENGTH"))
        self.assertEqual(dspacing, peak.location_in_unit("dspacing"))
        self.assertEqual(q, peak.location_in_unit("Q"))
        self.assertEqual(q, peak.location_in_unit("MomentumTransfer"))

    def test_can_be_located_in(self):
        for unit in ("TOF", "dSpacing", "WAVELENGTH", "q", "MomentumTransfer"):
            with self.subTest(unit=unit):
                self.assertTrue(Peak.can_be_located_in(unit))
        for unit in ("Energy", "Label", "Empty"):
            with self.subTest(unit=unit):
                self.assertFalse(Peak.can_be_located_in(unit))

    def test_can_be_located_in_agrees_with_location_in_unit(self):
        peak = Peak(0, 0, (1.233333, 4.0, 36), 10, 20, 15, 25)
        for unit in ("TOF", "dSpacing", "Wavelength", "Q", "MomentumTransfer", "Energy", "Label", "Empty"):
            with self.subTest(unit=unit):
                self.assertEqual(Peak.can_be_located_in(unit), peak.location_in_unit(unit) is not None)

    def test_location_in_unit_unknown_unit(self):
        """A workspace can be in a unit a peak has no position for, which is not an error:
        the instrument view still has to draw everything else."""
        peak = Peak(0, 0, (1.233333, 4.0, 36), 10, 20, 15, 25)
        self.assertIsNone(peak.location_in_unit("Oops"))
        self.assertIsNone(peak.location_in_unit("Empty"))
        self.assertIsNone(peak.location_in_unit("Energy"))


if __name__ == "__main__":
    unittest.main()
