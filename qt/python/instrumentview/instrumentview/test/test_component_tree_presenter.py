# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from instrumentview.ComponentTreePresenter import ComponentTreePresenter
from instrumentview.ComponentTreeModel import Node

from qtpy.QtGui import QStandardItem
import numpy as np
import unittest
from unittest.mock import MagicMock


class TestComponentTreePresenter(unittest.TestCase):
    def setUp(self):
        # Build test tree:
        # root
        #  ├── child1
        #  └── child2
        self.root = Node("root", 0, children=[Node("child1", 1, []), Node("child2", 2, [])])

        self.model = MagicMock()
        self.model.tree = self.root
        self.model.get_all_sub_component_indices = MagicMock(return_value=np.array([10, 20]))

        self.view = MagicMock()
        self.callback = MagicMock()
        self.presenter = ComponentTreePresenter(self.view, self.model, self.callback)

    def test_constructor_builds_qt_tree_model(self):
        """Constructor should create a QStandardItemModel with the correct root structure."""
        q_model = self.presenter.model_for_qt_tree
        root_item = q_model.invisibleRootItem().child(0)
        self.assertIsNotNone(root_item)
        self.assertEqual(root_item.text(), "root")
        self.assertEqual(root_item.component_index, 0)
        self.assertEqual(root_item.rowCount(), 2)
        self.assertEqual(root_item.child(0).text(), "child1")
        self.assertEqual(root_item.child(0).component_index, 1)
        self.assertEqual(root_item.child(1).text(), "child2")
        self.assertEqual(root_item.child(1).component_index, 2)

    def test_create_q_item_from_node(self):
        """_create_q_item_from_node should set name and component_index."""
        node = Node("testnode", 42, [])
        item = self.presenter._create_q_item_from_node(node)

        self.assertIsInstance(item, QStandardItem)
        self.assertEqual(item.text(), "testnode")
        self.assertEqual(item.component_index, 42)

    def test_add_children_to_parent_creates_hierarchy(self):
        """_add_children_to_parent should recursively build QStandardItem hierarchy."""
        q_root = self.presenter._add_children_to_parent(self.root, None)

        self.assertEqual(q_root.text(), "root")
        self.assertEqual(q_root.rowCount(), 2)
        self.assertEqual(q_root.child(0).text(), "child1")
        self.assertEqual(q_root.child(1).text(), "child2")

    def test_on_selection_changed_calls_callback_with_indices(self):
        """on_selection_changed should gather component indices and call callback."""
        mock_item1 = MagicMock()
        mock_item1.component_index = 5
        mock_item2 = MagicMock()
        mock_item2.component_index = 6

        self.presenter.on_selection_changed([mock_item1, mock_item2])
        self.model.get_all_sub_component_indices.assert_called_once_with([5, 6])
        args, _ = self.callback.call_args
        np.testing.assert_array_equal(args[0], np.array([10, 20]))

    def test_on_selection_changed_empty_list(self):
        """Empty selection should still call callback with the model's result."""
        self.presenter.on_selection_changed([])

        self.model.get_all_sub_component_indices.assert_called_once_with([])
        args, _ = self.callback.call_args
        np.testing.assert_array_equal(args[0], np.array([10, 20]))


if __name__ == "__main__":
    unittest.main()
