# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
from __future__ import (absolute_import, division, unicode_literals)

# std imports
import unittest
import json

# 3rdparty imports
from mantid.api import IPeak
from mantid.kernel import V3D
from mantid.py3compat.mock import create_autospec, ANY, MagicMock

# local imports
from mantidqt.widgets.sliceviewer.peaksviewer.representation \
    import PeakRepresentationNoShape, PeakRepresentationSphere, \
    create_peakrepresentation


class PeakRepresentationTest(unittest.TestCase):
    def test_representation_attributes_read_only(self):
        representation = PeakRepresentationNoShape(V3D(), 1.0, 'w')

        for name, value in (("alpha", 2), ("center", [1, 2, 3]), ('marker_color', 'b')):
            self.assertRaises(AttributeError, setattr, representation, name, value)

    def test_noshape_representation_draw_creates_scatter_point(self):
        center, alpha, marker_color = V3D(0.0, 1.0, -1.0), 0.5, 'b'
        no_shape = PeakRepresentationNoShape(center, alpha, marker_color)
        axes = MagicMock()

        no_shape.draw(axes)

        axes.scatter.assert_called_once_with(center.X(),
                                             center.Y(),
                                             alpha=alpha,
                                             color=marker_color,
                                             marker=ANY,
                                             s=ANY)
        self._verify_expected_attributes(no_shape, center, alpha, marker_color)

    def test_spherical_representation_draw_creates_circle_of_expected_radius(self):
        center, alpha, marker_color = V3D(0.0, 1.0, -1.0), 0.5, 'b'
        radius = 1.3
        sphere = PeakRepresentationSphere(radius, center, alpha, marker_color)
        axes = MagicMock()

        sphere.draw(axes)

        axes.add_patch.assert_called_once_with(ANY)
        self.assertEqual(radius, sphere.radius)
        self._verify_expected_attributes(sphere, center, alpha, marker_color)

    def test_create_returns_simple_representation_for_non_integrated_peak(self):
        self._verify_common_representation_attributes(expected_cls=PeakRepresentationNoShape,
                                                      peak_shape="none",
                                                      shape_json={})

    def test_create_returns_sphere_representation_for_spherically_integrated_peak(self):
        self._verify_common_representation_attributes(expected_cls=PeakRepresentationSphere,
                                                      peak_shape="spherical",
                                                      shape_json={'radius': 1.5})

    def test_create_is_case_insensitive_when_matching_shape_name(self):
        self._verify_common_representation_attributes(expected_cls=PeakRepresentationSphere,
                                                      peak_shape="SpHericaL",
                                                      shape_json={'radius': 1.5})

    # private
    def _verify_common_representation_attributes(self, expected_cls, peak_shape, shape_json):
        mock_peak = create_autospec(IPeak)
        center = V3D(-1, 2, 3)
        mock_peak.getQLabFrame.return_value = center
        mock_shape = MagicMock()
        mock_shape.shapeName.return_value = peak_shape
        mock_shape.toJSON.return_value = json.dumps(shape_json)
        mock_peak.getPeakShape.return_value = mock_shape
        marker_color = 'w'
        representation = create_peakrepresentation(mock_peak, marker_color)
        self.assertTrue(isinstance(representation, expected_cls))
        self._verify_expected_attributes(representation, center, 0.8, marker_color)
        return representation

    def _verify_expected_attributes(self, representation, center, alpha, marker_color):
        self.assertEqual(center, representation.center)
        self.assertEqual(alpha, representation.alpha)
        self.assertEqual(marker_color, representation.marker_color)


if __name__ == "__main__":
    unittest.main()
