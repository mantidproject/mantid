# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from instrumentview.ComponentTreePresenter import ComponentTreePresenter, _COMPONENT_INDEX_ROLE

from qtpy.QtGui import QStandardItem
import numpy as np
import unittest
from unittest.mock import MagicMock


class TestComponentTreePresenter(unittest.TestCase):
    def setUp(self):
        # Tree structure:
        # root (0)
        #  ├── child1 (1)  -- no children
        #  └── child2 (2)  -- no children

        self.model = MagicMock()
        self.model.root_index.return_value = 0
        self.model.root_name.return_value = "root"
        self.model.has_children.side_effect = lambda idx: {0: True, 1: False, 2: False}.get(idx, False)
        self.model.get_children.side_effect = lambda idx: {
            0: [("child1", 1), ("child2", 2)],
            1: [],
            2: [],
        }[idx]
        self.model.get_all_sub_component_indices = MagicMock(return_value=np.array([10, 20]))

        self.view = MagicMock()
        self.callback = MagicMock()
        self.presenter = ComponentTreePresenter(self.view, self.model, self.callback)

    def test_constructor_builds_root_with_placeholder(self):
        """Constructor should build only the root node plus a placeholder for lazy loading."""
        q_model = self.presenter.model_for_qt_tree
        root_item = q_model.invisibleRootItem().child(0)
        self.assertIsNotNone(root_item)
        self.assertEqual(root_item.text(), "root")
        self.assertEqual(root_item.data(_COMPONENT_INDEX_ROLE), 0)
        # Root has children, so it should have exactly one placeholder child
        self.assertEqual(root_item.rowCount(), 1)
        self.assertEqual(root_item.child(0).text(), ComponentTreePresenter._PLACEHOLDER_TEXT)

    def test_on_item_expanded_replaces_placeholder_with_children(self):
        """Expanding the root should replace the placeholder with real children."""
        root_item = self.presenter._q_model.invisibleRootItem().child(0)
        self.presenter.on_item_expanded(root_item)

        self.assertEqual(root_item.rowCount(), 2)
        self.assertEqual(root_item.child(0).text(), "child1")
        self.assertEqual(root_item.child(0).data(_COMPONENT_INDEX_ROLE), 1)
        self.assertEqual(root_item.child(1).text(), "child2")
        self.assertEqual(root_item.child(1).data(_COMPONENT_INDEX_ROLE), 2)

    def test_on_item_expanded_leaf_children_have_no_placeholder(self):
        """Children with no sub-children should not receive a placeholder."""
        root_item = self.presenter._q_model.invisibleRootItem().child(0)
        self.presenter.on_item_expanded(root_item)

        self.assertEqual(root_item.child(0).rowCount(), 0)
        self.assertEqual(root_item.child(1).rowCount(), 0)

    def test_on_item_expanded_is_idempotent(self):
        """Expanding an already-expanded node should not reload children."""
        root_item = self.presenter._q_model.invisibleRootItem().child(0)
        self.presenter.on_item_expanded(root_item)
        self.presenter.on_item_expanded(root_item)  # second expand should be a no-op

        self.model.get_children.assert_called_once_with(0)
        self.assertEqual(root_item.rowCount(), 2)

    def test_on_selection_changed_calls_callback_with_indices(self):
        """on_selection_changed should gather component indices and call callback."""
        mock_item1 = MagicMock()
        mock_item1.data.return_value = 5
        mock_item2 = MagicMock()
        mock_item2.data.return_value = 6

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

    def test_on_selection_changed_ignores_placeholder_items(self):
        """Placeholder items (component_index == -1) must not be passed to the model."""
        placeholder = QStandardItem(ComponentTreePresenter._PLACEHOLDER_TEXT)
        placeholder.setData(-1, _COMPONENT_INDEX_ROLE)
        real_item = MagicMock()
        real_item.data.return_value = 3

        self.presenter.on_selection_changed([placeholder, real_item])
        self.model.get_all_sub_component_indices.assert_called_once_with([3])


if __name__ == "__main__":
    unittest.main()
