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
    import PeakRepresentationEllipsoid


class PeakRepresentationEllipsoidTest(unittest.TestCase):
    def test_ellipsoid_representation_draw_creates_patch(self):
        center, alpha, marker_color = V3D(0.0, 1.0, -1.0), 0.5, 'b'
        width, height = 1, 1.3
        bkgd_widths, bkgd_heights = (1.9, 2.0), (2.5, 2.7)
        ellipse = PeakRepresentationEllipsoid(center, width, height, center, alpha, marker_color,
                                              bkgd_widths, bkgd_heights)
        axes = MagicMock()

        ellipse.draw(axes)

        axes.add_patch.assert_has_calls([call(ANY), call(ANY)])

    def test_create_from_peakshape_object_no_bkgd(self):
        center, marker_color = V3D(0.0, 1.0, -1.0), 'b'
        width, height = 1.0, 1.5
        peakshape_json = {"radius0": 0.5 * width, "radius1": 0.5 * height}
        peak_shape = MagicMock()
        peak_shape.toJSON.return_value = json.dumps(peakshape_json)
        ellipse = PeakRepresentationEllipsoid.create(center, peak_shape, marker_color)

        self.assertEqual(width, ellipse.width)
        self.assertEqual(height, ellipse.height)
        self.assertEqual((0.0, 0.0), ellipse.background_widths)
        self.assertEqual((0.0, 0.0), ellipse.background_heights)

    def test_create_from_peakshape_object_with_bkgd(self):
        center, marker_color = V3D(0.0, 1.0, -1.0), 'b'
        width, height = 1.0, 1.5
        bkgd_widths, bkgd_heights = (2.0, 2.1), (1.7, 1.8)
        peakshape_json = {
            "radius0": 0.5 * width,
            "radius1": 0.5 * height,
            "background_inner_radius0": 0.5 * bkgd_widths[0],
            "background_outer_radius0": 0.5 * bkgd_widths[1],
            "background_inner_radius1": 0.5 * bkgd_heights[0],
            "background_outer_radius1": 0.5 * bkgd_heights[1]
        }
        peak_shape = MagicMock()
        peak_shape.toJSON.return_value = json.dumps(peakshape_json)
        ellipse = PeakRepresentationEllipsoid.create(center, peak_shape, marker_color)

        self.assertEqual(width, ellipse.width)
        self.assertEqual(height, ellipse.height)
        self.assertEqual(bkgd_widths, ellipse.background_widths)
        self.assertEqual(bkgd_heights, ellipse.background_heights)


if __name__ == "__main__":
    unittest.main()
