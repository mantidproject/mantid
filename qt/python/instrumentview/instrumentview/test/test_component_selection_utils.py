# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
import unittest.mock
import numpy as np

from instrumentview.ComponentSelectionUtils import detector_indices_in_component_subtrees


class TestComponentSelectionUtils(unittest.TestCase):
    def setUp(self):
        self.component_info = unittest.mock.MagicMock()
        self.component_info.detectorsInSubtree.side_effect = lambda idx: {
            0: np.array([10, 11, 12]),
            1: np.array([10, 11]),
            2: np.array([12]),
            3: np.array([11]),
        }[idx]

    def test_get_all_sub_detector_indices_single(self):
        """Single index should return detectors in that subtree."""
        result = detector_indices_in_component_subtrees([1], self.component_info)
        np.testing.assert_array_equal(result, np.array([10, 11]))

    def test_get_all_sub_detector_indices_multiple(self):
        """Multiple indices should concatenate detector subtree results."""
        result = detector_indices_in_component_subtrees([1, 2], self.component_info)
        np.testing.assert_array_equal(result, np.array([10, 11, 12]))

    def test_get_all_sub_detector_indices_empty(self):
        """Empty input returns an empty detector-index array."""
        result = detector_indices_in_component_subtrees([], self.component_info)
        np.testing.assert_array_equal(result, np.array([], dtype=int))
