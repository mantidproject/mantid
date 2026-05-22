# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
import unittest.mock
import numpy as np

from instrumentview.ComponentSelectionUtils import detector_indices_in_component_subtrees, get_beam_axis, reflect_points_in_axis


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

    def test_reflect_across_plane_perpendicular_to_x(self):
        """Reflecting across yz-plane (axis=[1,0,0]) negates x-component."""
        points = np.array([[1.0, 2.0, 3.0], [-1.0, 0.0, 5.0]])
        result = reflect_points_in_axis(points, np.array([1.0, 0.0, 0.0]))
        expected = np.array([[-1.0, 2.0, 3.0], [1.0, 0.0, 5.0]])
        np.testing.assert_array_almost_equal(result, expected)

    def test_reflect_across_plane_perpendicular_to_y(self):
        """Reflecting across xz-plane (axis=[0,1,0]) negates y-component."""
        points = np.array([[1.0, 2.0, 3.0]])
        result = reflect_points_in_axis(points, np.array([0.0, 1.0, 0.0]))
        expected = np.array([[1.0, -2.0, 3.0]])
        np.testing.assert_array_almost_equal(result, expected)

    def test_reflect_across_plane_perpendicular_to_z(self):
        """Reflecting across xy-plane (axis=[0,0,1]) negates z-component."""
        points = np.array([[1.0, 2.0, 3.0]])
        result = reflect_points_in_axis(points, np.array([0.0, 0.0, 1.0]))
        expected = np.array([[1.0, 2.0, -3.0]])
        np.testing.assert_array_almost_equal(result, expected)

    def test_reflect_twice_returns_original(self):
        """Reflecting a point twice across the same plane returns the original."""
        points = np.array([[3.0, -1.0, 2.0]])
        axis = np.array([1.0, 0.0, 0.0])
        result = reflect_points_in_axis(reflect_points_in_axis(points, axis), axis)
        np.testing.assert_array_almost_equal(result, points)

    def test_point_on_reflection_plane_is_unchanged(self):
        """A point lying in the reflection plane is unchanged."""
        points = np.array([[0.0, 3.0, -4.0]])
        result = reflect_points_in_axis(points, np.array([1.0, 0.0, 0.0]))
        np.testing.assert_array_almost_equal(result, points)

    def _make_workspace(self, vec):
        ws = unittest.mock.MagicMock()
        ws.getInstrument().getReferenceFrame().vecPointingAlongBeam.return_value = vec
        return ws

    def test_unit_vector_returned_unchanged(self):
        """A beam axis that is already a unit vector is returned as-is."""
        ws = self._make_workspace(np.array([0.0, 0.0, 1.0]))
        result = get_beam_axis(ws)
        np.testing.assert_array_almost_equal(result, np.array([0.0, 0.0, 1.0]))

    def test_non_unit_vector_is_normalised(self):
        """A beam axis with magnitude != 1 is normalised to a unit vector."""
        ws = self._make_workspace(np.array([0.0, 0.0, 3.0]))
        result = get_beam_axis(ws)
        np.testing.assert_almost_equal(np.linalg.norm(result), 1.0)
        np.testing.assert_array_almost_equal(result, np.array([0.0, 0.0, 1.0]))

    def test_diagonal_vector_is_normalised(self):
        """A diagonal beam axis is normalised to a unit vector."""
        ws = self._make_workspace(np.array([1.0, 1.0, 0.0]))
        result = get_beam_axis(ws)
        np.testing.assert_almost_equal(np.linalg.norm(result), 1.0)
        np.testing.assert_array_almost_equal(result, np.array([1.0 / np.sqrt(2), 1.0 / np.sqrt(2), 0.0]))

    def test_zero_vector_raises(self):
        """A zero beam axis raises ValueError."""
        ws = self._make_workspace(np.array([0.0, 0.0, 0.0]))
        with self.assertRaises(ValueError):
            get_beam_axis(ws)
