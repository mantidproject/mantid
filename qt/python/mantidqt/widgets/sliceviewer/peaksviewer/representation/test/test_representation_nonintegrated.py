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
from mantid.py3compat.mock import MagicMock

# local imports
from mantidqt.widgets.sliceviewer.peaksviewer.representation \
    import NonIntegratedPeakRepresentation
from mantidqt.widgets.sliceviewer.peaksviewer.representation.test.representation_test_mixin \
    import PeakRepresentationMixin


class NonIntegratedPeakRepresentationTest(unittest.TestCase, PeakRepresentationMixin):
    REPR_CLS = NonIntegratedPeakRepresentation

    def create_test_object(self):
        self.x, self.y, self.z, self.alpha, shape, self.fg_color = 0.0, 1.0, -1.0, 0.5, None, 'b'

        return self.REPR_CLS.create(self.x, self.y, self.z, self.alpha, shape, self.fg_color)

    def test_noshape_representation_draw_creates_cross(self):
        x, y, z, alpha, marker_color = 0.0, 1.0, -1.0, 0.5, 'b'
        drawables = [MagicMock()]
        no_shape = NonIntegratedPeakRepresentation(x, y, z, alpha, marker_color, drawables)
        painter = MagicMock()

        no_shape.draw(painter)

        drawables[0].draw.assert_called_once_with(painter, no_shape)


if __name__ == "__main__":
    unittest.main()
