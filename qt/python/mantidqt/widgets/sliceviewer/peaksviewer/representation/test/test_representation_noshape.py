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

# 3rdparty imports
from mantid.kernel import V3D
from mantid.py3compat.mock import ANY, MagicMock

# local imports
from mantidqt.widgets.sliceviewer.peaksviewer.representation \
    import PeakRepresentationNoShape


class PeakRepresentationNoShapeTest(unittest.TestCase):
    def test_constructor_stores_expected_attributes(self):
        x, y, z, alpha, marker_color = 0.0, 1.0, -1.0, 0.5, 'b'

        representation = PeakRepresentationNoShape(x, y, z, alpha, marker_color)

        self._verify_expected_attributes(representation, V3D(x, y, z), alpha, marker_color)

    def test_representation_attributes_read_only(self):
        representation = PeakRepresentationNoShape(0, 0, 0, 1.0, 'w')

        for name, value in (("alpha", 2), ("x", 2), ("y", 0.1), ("z", 3), ('marker_color', 'b')):
            self.assertRaises(AttributeError, setattr, representation, name, value)

    def test_snap_to_calls_painter_method_with_peak_center(self):
        x, y = 1.5, 2.5
        representation = PeakRepresentationNoShape(x, y, 0, 1.0, 'w')
        mock_painter = MagicMock()

        representation.snap_to(mock_painter)

        mock_painter.assert_called_once_with(x, y)

    def test_noshape_representation_draw_creates_scatter_point(self):
        x, y, z, alpha, marker_color = 0.0, 1.0, -1.0, 0.5, 'b'
        no_shape = PeakRepresentationNoShape(x, y, z, alpha, marker_color)
        painter = MagicMock()

        no_shape.draw(painter)

        painter.scatter.assert_called_once_with(
            x, y, alpha=alpha, color=marker_color, marker=ANY, s=ANY)

    def test_noshape_create_computes_alpha(self):
        x, y, z = -1, 2, 3
        # center = V3D(x, y, z)
        mock_shape = MagicMock()
        marker_color = 'w'
        slicepoint, dimwidth = 3.2, 30

        representation = PeakRepresentationNoShape.create(x, y, z, slicepoint, dimwidth, mock_shape,
                                                          marker_color)

        self.assertAlmostEqual(0.4444, representation.alpha, places=4)

    # private
    def _verify_expected_attributes(self, representation, center, alpha, marker_color):
        self.assertEqual(center.X(), representation.x)
        self.assertEqual(center.Y(), representation.y)
        self.assertEqual(center.Z(), representation.z)
        self.assertAlmostEqual(alpha, representation.alpha, places=4)
        self.assertEqual(marker_color, representation.marker_color)


if __name__ == "__main__":
    unittest.main()
