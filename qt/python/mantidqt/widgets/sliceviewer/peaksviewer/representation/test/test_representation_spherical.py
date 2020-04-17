# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

# std imports
import json
import unittest
from unittest.mock import MagicMock

# local imports
from mantidqt.widgets.sliceviewer.model import SliceInfo
from mantidqt.widgets.sliceviewer.peaksviewer.representation \
    import SphericallyIntergratedPeakRepresentation
from mantidqt.widgets.sliceviewer.peaksviewer.representation.test.representation_test_mixin \
    import PeakRepresentationMixin


class PeakRepresentationSphericalTest(unittest.TestCase, PeakRepresentationMixin):
    REPR_CLS = SphericallyIntergratedPeakRepresentation

    def create_test_object(self):
        self.radius = 0.8
        shape = MagicMock()
        shape.toJSON.return_value = json.dumps({"radius": self.radius})
        self.x, self.y, self.z, shape, \
            self.fg_color, self.bg_color = 0.0, 1.0, -1.0, shape, 'b', 'r'
        sliceinfo = MagicMock(spec=SliceInfo)
        sliceinfo.transform.return_value = (self.x, self.y, self.z)
        sliceinfo.value = -1.05
        sliceinfo.width = 10.
        self.alpha = 0.53333

        return self.REPR_CLS.create((self.x, self.y, self.z), shape, sliceinfo, self.fg_color,
                                    self.bg_color)


if __name__ == "__main__":
    unittest.main()
