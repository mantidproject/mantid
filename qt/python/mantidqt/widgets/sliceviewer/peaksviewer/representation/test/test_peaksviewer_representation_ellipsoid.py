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
from mantidqt.widgets.sliceviewer.peaksviewer.representation.ellipsoid \
    import EllipsoidalIntergratedPeakRepresentation

from mantidqt.widgets.sliceviewer.peaksviewer.representation.test.shapetesthelpers \
    import FuzzyMatch, create_ellipsoid_info, draw_representation


def create_test_ellipsoid():
    return create_ellipsoid_info(
        radii=(1.5, 1.3, 1.4),
        axes=("1 0 0", "0 1 0", "0 0 1"),
        bkgd_radii=(((2.5, 2.3, 2.4)), ((2.6, 2.4, 2.5))))


@patch("mantidqt.widgets.sliceviewer.peaksviewer.representation.ellipsoid.compute_alpha")
class EllipsoidalIntergratedPeakRepresentationTest(unittest.TestCase):
    def test_draw_creates_nothing_when_alpha_lt_zero(self, compute_alpha_mock):
        ellipsoid = create_test_ellipsoid()
        painter = MagicMock()
        fake_alpha = -0.1
        compute_alpha_mock.return_value = fake_alpha

        painted = draw_representation(EllipsoidalIntergratedPeakRepresentation, (1, 2, 3),
                                      ellipsoid, painter, 'r', 'g')

        self.assertTrue(painted is None)
        painter.cross.assert_not_called()
        painter.ellipse.assert_not_called()
        painter.elliptical_shell.assert_not_called()

    def test_draw_creates_ellipse_with_expected_properties_with_nonzero_alpha(
            self, compute_alpha_mock):
        peak_center = (1, 2, 3)
        ellipsoid = create_test_ellipsoid()
        painter = MagicMock()
        fg_color, bg_color = 'r', 'g'
        fake_alpha = 0.5
        compute_alpha_mock.return_value = fake_alpha

        painted = draw_representation(EllipsoidalIntergratedPeakRepresentation, peak_center,
                                      ellipsoid, painter, fg_color, bg_color)

        self.assertTrue(painted is not None)
        self._assert_painter_calls(
            painter,
            peak_center[:2],
            cross_width=0.26,
            signal_width=2.6,
            signal_height=3.0,
            angle=90,
            alpha=fake_alpha,
            fg_color=fg_color,
            bkgd_width=4.8,
            bkgd_height=5.2,
            thickness=0.1,
            bg_color=bg_color)

    def test_draw_respects_transform(self, compute_alpha_mock):
        def slice_transform(x):
            # set slice(x)=data(z)
            return (x[2], x[0], x[1])

        peak_center = (1, 2, 3)
        ellipsoid = create_test_ellipsoid()
        painter = MagicMock()
        fg_color, bg_color = 'r', 'g'
        fake_alpha = 0.5
        compute_alpha_mock.return_value = fake_alpha

        painted = draw_representation(EllipsoidalIntergratedPeakRepresentation, peak_center,
                                      ellipsoid, painter, fg_color, bg_color, slice_transform)

        self.assertTrue(painted is not None)
        self._assert_painter_calls(
            painter, (peak_center[2], peak_center[0]),
            cross_width=0.182,
            signal_width=1.82,
            signal_height=2.1,
            angle=90,
            alpha=fake_alpha,
            fg_color=fg_color,
            bkgd_width=4.4,
            bkgd_height=4.77,
            thickness=0.1,
            bg_color=bg_color)

    # private
    def _assert_painter_calls(self,
                              painter,
                              center,
                              cross_width,
                              signal_width,
                              signal_height,
                              angle,
                              alpha,
                              fg_color,
                              bkgd_width=None,
                              bkgd_height=None,
                              thickness=None,
                              bg_color=None):
        x, y = center
        # center
        painter.cross.assert_called_once_with(
            x, y, FuzzyMatch(cross_width, atol=1e-3), alpha=alpha, color=fg_color)
        # signal circle
        painter.ellipse.assert_called_once_with(
            x,
            y,
            FuzzyMatch(signal_width, atol=1e-2),
            FuzzyMatch(signal_height, atol=1e-2),
            FuzzyMatch(angle, atol=1e-2),
            alpha=alpha,
            edgecolor=fg_color,
            facecolor='none',
            linestyle='--')
        # background
        if bkgd_width is None:
            painter.elliptical_shell.assert_not_called()
        else:
            painter.elliptical_shell.assert_called_once_with(
                x,
                y,
                FuzzyMatch(bkgd_width, atol=1e-2),
                FuzzyMatch(bkgd_height, atol=1e-2),
                FuzzyMatch(thickness, atol=1e-2),
                FuzzyMatch(angle, atol=1e-2),
                alpha=alpha,
                edgecolor="none",
                facecolor=bg_color,
                linestyle='--')


if __name__ == "__main__":
    unittest.main()
