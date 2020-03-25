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
from mantid.kernel import V3D
from mantid.py3compat.mock import MagicMock

# local imports
from mantidqt.widgets.sliceviewer.peaksviewer.representation \
    import (NonIntegratedPeakRepresentation, create_peakrepresentation)


class CreatePeakRepresentationTest(unittest.TestCase):
    def test_create_returns_simple_representation_for_non_integrated_peak(self):
        self._verify_common_representation_attributes(
            expected_cls=NonIntegratedPeakRepresentation, peak_shape="none", shape_json={})

    def test_create_is_case_insensitive_when_matching_shape_name(self):
        self._verify_common_representation_attributes(
            expected_cls=NonIntegratedPeakRepresentation,
            peak_shape="NoNe",
            shape_json={
                'radius': 1.5
            })

    # private
    def _verify_common_representation_attributes(self, expected_cls, peak_shape, shape_json):
        x, y, z = -1, 2, 3
        center = V3D(x, y, z)
        mock_shape = MagicMock()
        mock_shape.shapeName.return_value = peak_shape
        mock_shape.toJSON.return_value = json.dumps(shape_json)
        marker_color = 'w'
        slicepoint, dimwidth = 3.2, 30
        representation = create_peakrepresentation(x, y, z, slicepoint, dimwidth, mock_shape,
                                                   marker_color)
        self.assertTrue(isinstance(representation, expected_cls))
        self._verify_expected_attributes(representation, center, 0.4444, marker_color)
        return representation

    def _verify_expected_attributes(self, representation, center, alpha, marker_color):
        self.assertEqual(center.X(), representation.x)
        self.assertEqual(center.Y(), representation.y)
        self.assertEqual(center.Z(), representation.z)
        self.assertAlmostEqual(alpha, representation.alpha, places=4)
        self.assertEqual(marker_color, representation.fg_color)


if __name__ == "__main__":
    unittest.main()
