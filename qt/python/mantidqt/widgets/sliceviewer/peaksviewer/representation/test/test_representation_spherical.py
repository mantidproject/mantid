# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
from __future__ import (absolute_import, division, unicode_literals)

# std imports
import json
import unittest

# 3rdparty imports
from mantid.kernel import V3D
from mantid.py3compat.mock import ANY, MagicMock, call

# local imports
from mantidqt.widgets.sliceviewer.peaksviewer.representation \
    import PeakRepresentationSphere


class PeakRepresentationSphereTest(unittest.TestCase):
    def test_draw_creates_circle_of_expected_radius_no_bkgd(self):
        center, alpha, marker_color = V3D(0.0, 1.0, -1.0), 0.5, 'b'
        radius = 1.3
        sphere = PeakRepresentationSphere(radius, center, alpha, marker_color)
        axes = MagicMock()

        sphere.draw(axes)

        axes.add_patch.assert_called_once_with(ANY)
        self.assertEqual(radius, sphere.radius)

    def test_draw_with_bkgd_creates_two_circles(self):
        center, alpha, marker_color = V3D(0.0, 1.0, -1.0), 0.5, 'b'
        radius = 1.3
        bkgd_radii = (2, 2.1)
        sphere = PeakRepresentationSphere(radius, center, alpha, marker_color, bkgd_radii)
        axes = MagicMock()

        sphere.draw(axes)

        axes.add_patch.assert_has_calls([call(ANY), call(ANY)])
        self.assertEqual(radius, sphere.radius)

    def test_create_from_peakshape_object_no_bkgd(self):
        center, marker_color = V3D(0.0, 1.0, -1.0), 'b'
        radius = 1.5
        peakshape_json = {"radius": radius}
        peak_shape = MagicMock()
        peak_shape.toJSON.return_value = json.dumps(peakshape_json)
        sphere = PeakRepresentationSphere.create(center, peak_shape, marker_color)

        self.assertEqual(radius, sphere.radius)
        self.assertEqual((0.0, 0.0), sphere.background_radii)

    def test_create_from_peakshape_object_with_bkgd(self):
        center, marker_color = V3D(0.0, 1.0, -1.0), 'b'
        radius = 1.5
        bkgd_radii = (2.0, 2.1)
        peakshape_json = {
            "radius": radius,
            "background_inner_radius": bkgd_radii[0],
            "background_outer_radius": bkgd_radii[1]
        }
        peak_shape = MagicMock()
        peak_shape.toJSON.return_value = json.dumps(peakshape_json)
        sphere = PeakRepresentationSphere.create(center, peak_shape, marker_color)

        self.assertEqual(radius, sphere.radius)
        self.assertEqual(bkgd_radii, sphere.background_radii)


if __name__ == "__main__":
    unittest.main()
