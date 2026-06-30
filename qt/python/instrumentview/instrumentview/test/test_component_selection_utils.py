# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
import unittest.mock
import numpy as np

from instrumentview.ComponentSelectionUtils import detector_component_indices_in_subtrees, get_beam_axis, reflect_points_in_axis


class TestComponentSelectionUtils(unittest.TestCase):
    def setUp(self):
        self.component_info = unittest.mock.MagicMock()
        # component index -> parent index
        self.component_info.parent.side_effect = lambda idx: {
            10: 100,
            11: 100,
            20: 200,
            30: 300,
        }[idx]
        # parent index -> detector indices in subtree
        self.component_info.detectorsInSubtree.side_effect = lambda idx: {
            100: np.array([10, 11, 12]),
            200: np.array([20, 21]),
            300: np.array([30]),
        }[idx]

    def test_subtrees_empty_input_returns_empty_list(self):
        """Empty input returns an empty list."""
        result = detector_component_indices_in_subtrees(np.array([], dtype=int), self.component_info)
        self.assertEqual(result, [])

    def test_subtrees_single_index_returns_one_subtree(self):
        """Single component index returns the subtree of its parent."""
        result = detector_component_indices_in_subtrees(np.array([10]), self.component_info)
        self.assertEqual(len(result), 1)
        np.testing.assert_array_equal(result[0], np.array([10, 11, 12]))

    def test_subtrees_different_parents_returns_multiple_subtrees(self):
        """Indices with different parents each contribute a subtree."""
        result = detector_component_indices_in_subtrees(np.array([10, 20]), self.component_info)
        self.assertEqual(len(result), 2)
        np.testing.assert_array_equal(result[0], np.array([10, 11, 12]))
        np.testing.assert_array_equal(result[1], np.array([20, 21]))

    def test_subtrees_same_parent_deduplicates(self):
        """Indices sharing a parent produce only one subtree entry."""
        result = detector_component_indices_in_subtrees(np.array([10, 11]), self.component_info)
        self.assertEqual(len(result), 1)
        np.testing.assert_array_equal(result[0], np.array([10, 11, 12]))

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
