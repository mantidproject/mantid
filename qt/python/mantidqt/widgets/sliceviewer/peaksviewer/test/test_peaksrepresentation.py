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
from mantid.api import IPeak
from mantid.kernel import V3D
from mantid.py3compat.mock import create_autospec

# local imports
from mantidqt.widgets.sliceviewer.peaksviewer.peakrepresentation \
    import PeakRepresentation, create_peakrepresentation


class PeakRepresentationTest(unittest.TestCase):

    def test_create_returns_simple_representation_for_non_integrated_peak(self):
        mock_peak = create_autospec(IPeak)
        center = V3D(-1, 2, 3)
        mock_peak.getQLabFrame.return_value = center

        representation = create_peakrepresentation(mock_peak)

        self.assertTrue(isinstance(representation,
                                   PeakRepresentation))
        self.assertEqual(center, representation.center)
        self.assertEqual(1.0, representation.opacity)


if __name__ == "__main__":
    unittest.main()
