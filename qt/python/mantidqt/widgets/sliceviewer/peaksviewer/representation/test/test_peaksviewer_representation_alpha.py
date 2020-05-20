# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# std imports
import unittest

# local imports
from mantidqt.widgets.sliceviewer.peaksviewer.representation.alpha \
    import ALPHA_MAX, compute_alpha


class AlphaTest(unittest.TestCase):
    def test_compute_alpha_gives_max_value_when_z_equals_slice_point(self):
        slice_pt = 4.8

        alpha = compute_alpha(slice_pt, slice_pt, slicedim_width=10)

        self.assertAlmostEqual(ALPHA_MAX, alpha, places=5)

    def test_compute_alpha_symmetric_wrt_slice_pt_peak_center_distance(self):
        peak_center_z, slice_pt = 4.7, 4.8

        alpha_p_z = compute_alpha(peak_center_z, -slice_pt, slicedim_width=10)
        alpha_m_z = compute_alpha(-peak_center_z, slice_pt, slicedim_width=10)

        self.assertAlmostEqual(alpha_m_z, alpha_p_z, places=5)

    def test_compute_alpha_allowed_negative(self):
        peak_center_z, slice_pt = 1.81, 1

        alpha = compute_alpha(peak_center_z, slice_pt, slicedim_width=10)

        self.assertTrue(alpha < 0)


if __name__ == "__main__":
    unittest.main()
