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
import numpy as np

# local imports
from mantidqt.widgets.sliceviewer.peaksviewer.representation.ellipsoid \
    import EllipsoidalIntergratedPeakRepresentation, slice_ellipsoid

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
            cross_width=0.3,
            signal_width=2.6,
            signal_height=3.0,
            angle=90,
            alpha=fake_alpha,
            fg_color=fg_color,
            bkgd_width=4.8,
            bkgd_height=5.2,
            thickness=0.1 / 2.6,
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
            cross_width=0.192,
            signal_width=1.92,
            signal_height=1.79,
            angle=90,
            alpha=fake_alpha,
            fg_color=fg_color,
            bkgd_width=4.73,
            bkgd_height=4.55,
            thickness=0.1 / 2.6,
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


class EllipsoidalIntergratedPeakRepresentationSliceEllipsoidTest(unittest.TestCase):
    def test_slice_ellipsoid_zp(self):
        origin = (0, 0, 0)
        axis_a = (1, 0, 0)
        axis_b = (0, 1, 0)
        axis_c = (0, 0, 1)
        a = 1
        b = 2
        c = 1
        zp = 0

        slice_transform = lambda x: (x[1], x[0], x[2]) # Y out of plane

        expected_slice_origin = (0, 0, 0)
        expected_major_radius = 2
        expected_minor_radius = 1
        expected_angle = 0

        self._run_slice_ellipsoid_and_compare((origin, axis_a, axis_b, axis_c, a, b, c, zp, slice_transform),
                                              (*expected_slice_origin,
                                               expected_major_radius,
                                               expected_minor_radius,
                                               expected_angle))

        zp = np.sin(np.pi / 3)
        expected_slice_origin = (0, 0, zp)
        expected_major_radius = 1.0  # 2*cos(pi/3)
        expected_minor_radius = 0.5  # cos(pi/3)

        self._run_slice_ellipsoid_and_compare((origin, axis_a, axis_b, axis_c, a, b, c, zp, slice_transform),
                                              (*expected_slice_origin,
                                               expected_major_radius,
                                               expected_minor_radius,
                                               expected_angle))

        # This causes negative eignevalues there np.sqrt(eignevalues) gives NaN radius
        zp = 2
        expected_slice_origin = (0, 0, zp)
        expected_major_radius = np.nan
        expected_minor_radius = np.nan
        expected_angle = 90  # flips to 90 as x-axis (Y on view) has negative eigenvalue and is first in sorted list

        self._run_slice_ellipsoid_and_compare((origin, axis_a, axis_b, axis_c, a, b, c, zp, slice_transform),
                                              (*expected_slice_origin,
                                               expected_major_radius,
                                               expected_minor_radius,
                                               expected_angle))

        # This causes `eigvalues, eigvectors = linalg.eig(MM)` to throw np.linalg.LinAlgError
        zp = 1
        expected_slice_origin = (0, 0, 0)
        expected_major_radius = np.nan
        expected_minor_radius = np.nan
        expected_angle = 0  # returns 0 is could not diag ellipMatrix

        self._run_slice_ellipsoid_and_compare((origin, axis_a, axis_b, axis_c, a, b, c, zp, slice_transform),
                                              (*expected_slice_origin,
                                               expected_major_radius,
                                               expected_minor_radius,
                                               expected_angle))

    def _run_slice_ellipsoid_and_compare(self, input_values, expectecd):
        slice_origin, major_radius, minor_radius, angle = slice_ellipsoid(*input_values)
        print(slice_origin, major_radius, minor_radius, angle)

        self.assertAlmostEqual(slice_origin[0], expectecd[0])
        self.assertAlmostEqual(slice_origin[1], expectecd[1])
        self.assertAlmostEqual(slice_origin[2], expectecd[2])
        # numpy correctly handles NaN
        np.testing.assert_almost_equal(major_radius, expectecd[3])
        np.testing.assert_almost_equal(minor_radius, expectecd[4])
        self.assertAlmostEqual(angle, expectecd[5])


if __name__ == "__main__":
    unittest.main()
