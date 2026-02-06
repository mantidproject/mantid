# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from instrumentview.ComponentTreeModel import ComponentTreeModel

import numpy as np
import unittest
from unittest.mock import MagicMock


class TestComponentTreeModel(unittest.TestCase):
    def setUp(self):
        self.workspace = MagicMock()
        self.component_info = MagicMock()
        self.workspace.componentInfo.return_value = self.component_info

        # Define a component tree structure:
        #
        # component 0 (root)
        #  ├─ 1
        #  │   └─ 3
        #  └─ 2

        self.component_info.root.return_value = 0

        self.component_info.children.side_effect = lambda idx: {0: [1, 2], 1: [3], 2: [], 3: []}[idx]

        self.component_info.name.side_effect = lambda idx: {0: "root", 1: "child1", 2: "child2", 3: "grandchild"}[idx]

        self.component_info.componentsInSubtree.side_effect = lambda idx: {
            0: np.array([0, 1, 3, 2]),
            1: np.array([1, 3]),
            2: np.array([2]),
            3: np.array([3]),
        }[idx]

        # Create model
        self.model = ComponentTreeModel(self.workspace)

    # ---------------------------

    def test_tree_root_created_correctly(self):
        """Root node should be created with correct name and index."""
        root = self.model.tree
        self.assertEqual(root.name, "root")
        self.assertEqual(root.component_index, 0)
        self.assertEqual(len(root.children), 2)

    def test_children_created_correctly(self):
        """Tree should contain correct children and grandchildren."""
        root = self.model.tree
        child1 = root.children[0]
        child2 = root.children[1]

        self.assertEqual(child1.name, "child1")
        self.assertEqual(child1.component_index, 1)
        self.assertEqual(len(child1.children), 1)
        self.assertEqual(child1.children[0].name, "grandchild")

        self.assertEqual(child2.name, "child2")
        self.assertEqual(child2.component_index, 2)
        self.assertEqual(len(child2.children), 0)

    # ---------------------------

    def test_get_all_sub_component_indices_single(self):
        """Single index should return the subtree for that component."""
        result = self.model.get_all_sub_component_indices([1])
        np.testing.assert_array_equal(result, np.array([1, 3]))

    def test_get_all_sub_component_indices_multiple(self):
        """Multiple indices should concatenate subtree results."""
        result = self.model.get_all_sub_component_indices([1, 2])
        np.testing.assert_array_equal(result, np.array([1, 3, 2]))

    def test_get_all_sub_component_indices_empty(self):
        """Empty input returns an empty array."""
        result = self.model.get_all_sub_component_indices([])
        np.testing.assert_array_equal(result, np.array([], dtype=int))


if __name__ == "__main__":
    unittest.main()
