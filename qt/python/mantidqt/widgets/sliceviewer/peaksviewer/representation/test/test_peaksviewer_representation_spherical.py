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
from mantidqt.widgets.sliceviewer.peaksviewer.representation.spherical \
    import SphericallyIntergratedPeakRepresentation

from mantidqt.widgets.sliceviewer.peaksviewer.representation.test.shapetesthelpers \
    import FuzzyMatch, create_sphere_info, draw_representation


def create_test_sphere(background=False):
    signal_radius = 0.4
    if background:
        return create_sphere_info(radius=signal_radius, bkgd_radii=(0.8, 0.9))
    else:
        return create_sphere_info(radius=signal_radius)


@patch("mantidqt.widgets.sliceviewer.peaksviewer.representation.spherical.compute_alpha")
class SphericallyIntergratedPeakRepresentationTest(unittest.TestCase):
    def test_draw_creates_nothing_when_alpha_lt_zero(self, compute_alpha_mock):
        sphere = create_test_sphere()
        painter = MagicMock()
        fake_alpha = -0.1
        compute_alpha_mock.return_value = fake_alpha

        painted = draw_representation(SphericallyIntergratedPeakRepresentation, (1, 2, 3), sphere,
                                      painter, 'r', 'g')

        self.assertTrue(painted is None)
        painter.cross.assert_not_called()
        painter.circle.assert_not_called()

    def test_draw_creates_circle_with_expected_properties_with_nonzero_alpha_and_no_background(
            self, compute_alpha_mock):
        peak_center = (1, 2, 3)
        sphere = create_test_sphere()
        painter = MagicMock()
        fg_color, bg_color = 'r', 'unused'
        fake_alpha = 0.5
        compute_alpha_mock.return_value = fake_alpha

        painted = draw_representation(SphericallyIntergratedPeakRepresentation, peak_center, sphere,
                                      painter, fg_color, bg_color)

        self.assertTrue(painted is not None)
        self._assert_painter_calls(
            painter,
            peak_center[:2],
            cross_width=0.04,
            signal_slice_radius=0.4,
            alpha=fake_alpha,
            fg_color=fg_color)

    def test_draw_creates_circle_and_shell_with_expected_properties_with_nonzero_alpha_and_background(
            self, compute_alpha_mock):
        peak_center = (1, 2, 3)
        sphere = create_test_sphere(background=True)
        painter = MagicMock()
        fg_color, bg_color = 'r', 'g'
        fake_alpha = 0.5
        compute_alpha_mock.return_value = fake_alpha

        painted = draw_representation(SphericallyIntergratedPeakRepresentation, peak_center, sphere,
                                      painter, fg_color, bg_color)

        self.assertTrue(painted is not None)
        self._assert_painter_calls(
            painter,
            peak_center[:2],
            cross_width=0.04,
            signal_slice_radius=0.4,
            alpha=fake_alpha,
            fg_color=fg_color,
            bkgd_slice_radius=0.9,
            bkgd_thickness=0.09,
            bg_color=bg_color)

    def test_draw_respects_slice_transformation(self, compute_alpha_mock):
        def slice_transform(x):
            # set slice(x)=data(y)
            return (x[1], x[0], x[2])

        peak_center = (1, 2, 3)
        sphere = create_test_sphere(background=True)
        painter = MagicMock()
        fg_color, bg_color = 'r', 'g'
        fake_alpha = 0.5
        compute_alpha_mock.return_value = fake_alpha

        painted = draw_representation(SphericallyIntergratedPeakRepresentation, peak_center, sphere,
                                      painter, fg_color, bg_color, slice_transform)

        self.assertTrue(painted is not None)
        self._assert_painter_calls(
            painter, (peak_center[1], peak_center[0]),
            cross_width=0.04,
            signal_slice_radius=0.4,
            alpha=fake_alpha,
            fg_color=fg_color,
            bkgd_slice_radius=0.9,
            bkgd_thickness=0.09,
            bg_color=bg_color)

    # private
    def _assert_painter_calls(self,
                              painter,
                              center,
                              cross_width,
                              signal_slice_radius,
                              alpha,
                              fg_color,
                              bkgd_slice_radius=None,
                              bkgd_thickness=None,
                              bg_color=None):
        x, y = center
        # center
        painter.cross.assert_called_once_with(
            x, y, FuzzyMatch(cross_width, atol=1e-3), alpha=alpha, color=fg_color)
        # signal circle
        painter.circle.assert_called_once_with(
            x,
            y,
            FuzzyMatch(signal_slice_radius, atol=1e-2),
            alpha=alpha,
            edgecolor=fg_color,
            facecolor='none',
            linestyle='--')
        # background
        if bkgd_slice_radius is None:
            painter.shell.assert_not_called()
        else:
            painter.shell.assert_called_once_with(
                x,
                y,
                FuzzyMatch(bkgd_slice_radius, atol=1e-2),
                FuzzyMatch(bkgd_thickness, atol=1e-2),
                alpha=alpha,
                edgecolor="none",
                facecolor=bg_color,
                linestyle='--')


if __name__ == "__main__":
    unittest.main()
