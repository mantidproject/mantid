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
        peak = Peak(0, 0, None, 0, (1.233333, 4.0, 36), 0, 0, 0, 0)
        self.assertEqual("(1.23, 4, 36)", peak.label)

    def test_location_in_unit(self):
        tof = 10
        wavelength = 15
        dspacing = 20
        q = 25
        peak = Peak(0, 0, None, 0, (1.233333, 4.0, 36), tof, dspacing, wavelength, q)
        self.assertEquals(tof, peak.location_in_unit("TOF"))
        self.assertEquals(wavelength, peak.location_in_unit("WAVELENGTH"))
        self.assertEquals(dspacing, peak.location_in_unit("dspacing"))
        self.assertEquals(q, peak.location_in_unit("Q"))

    def test_location_in_unit_wrong_unit(self):
        peak = Peak(0, 0, None, 0, (1.233333, 4.0, 36), 0, 0, 0, 0)
        self.assertRaisesRegex(RuntimeError, "Unknown unit Oops for peak location", peak.location_in_unit, "Oops")


if __name__ == "__main__":
    unittest.main()
