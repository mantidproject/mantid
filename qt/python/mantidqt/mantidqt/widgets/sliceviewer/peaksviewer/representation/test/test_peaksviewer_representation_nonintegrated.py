# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

# std imports
import unittest
from unittest.mock import MagicMock, patch

# local imports
from mantidqt.widgets.sliceviewer.peaksviewer.representation.noshape \
    import NonIntegratedPeakRepresentation
from mantidqt.widgets.sliceviewer.peaksviewer.test.modeltesthelpers \
    import create_slice_info
from mantidqt.widgets.sliceviewer.peaksviewer.representation.test.shapetesthelpers \
    import FuzzyMatch


class NonIntegratedPeakRepresentationTest(unittest.TestCase):
    @patch("mantidqt.widgets.sliceviewer.peaksviewer.representation.noshape.compute_alpha")
    def test_draw_creates_cross_with_expected_properties_when_alpha_gt_zero(
            self, compute_alpha_mock):
        peak_origin, fg_color = (1, 3, 5), "r"
        peak_shape, painter = MagicMock(), MagicMock()
        painter.axes.get_xlim.return_value = (-10, 10)
        alpha = 0.5
        compute_alpha_mock.return_value = alpha
        slice_info = create_slice_info(lambda x: x, slice_value=3., slice_width=10.)

        painted = NonIntegratedPeakRepresentation.draw(peak_origin, peak_shape, slice_info, painter,
                                                       fg_color, "unused")

        width = 0.15
        painter.cross.assert_called_with(peak_origin[0],
                                         peak_origin[1],
                                         FuzzyMatch(width, atol=1e-2),
                                         alpha=alpha,
                                         color=fg_color)
        self.assertTrue(painted is not None)

    @patch("mantidqt.widgets.sliceviewer.peaksviewer.representation.noshape.compute_alpha")
    def test_draw_creates_nothing_when_alpha_lt_zero(self, compute_alpha_mock):
        peak_origin, fg_color = (1, 3, 5), "r"
        peak_shape, painter = MagicMock(), MagicMock()
        alpha = -0.1
        compute_alpha_mock.return_value = alpha
        slice_info = create_slice_info(lambda x: x, slice_value=3., slice_width=10.)

        painted = NonIntegratedPeakRepresentation.draw(peak_origin, peak_shape, slice_info, painter,
                                                       fg_color, "unused")

        painter.cross.assert_not_called()
        self.assertTrue(painted is None)


if __name__ == "__main__":
    unittest.main()
