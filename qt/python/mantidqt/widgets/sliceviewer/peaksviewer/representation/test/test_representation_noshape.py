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
    import (PeakRepresentationNoShape, PeakRepresentationEllipsoid,
            PeakRepresentationSphere, create_peakrepresentation)


class PeakRepresentationNoShapeTest(unittest.TestCase):
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


if __name__ == "__main__":
    unittest.main()
