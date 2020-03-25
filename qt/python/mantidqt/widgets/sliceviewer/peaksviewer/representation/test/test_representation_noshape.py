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
from mantidqt.widgets.sliceviewer.peaksviewer.representation.test.representation_test_mixin \
    import PeakRepresentationMixin


class PeakRepresentationNoShapeTest(unittest.TestCase, PeakRepresentationMixin):
    REPR_CLS = PeakRepresentationNoShape

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


if __name__ == "__main__":
    unittest.main()
