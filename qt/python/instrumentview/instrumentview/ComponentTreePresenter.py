# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from instrumentview.ComponentTreeView import ComponentTreeView
from instrumentview.ComponentTreeModel import ComponentTreeModel, Node
from qtpy.QtGui import QStandardItemModel, QStandardItem
from typing import Callable
import numpy as np


class ComponentTreePresenter:
    def __init__(
        self, view: ComponentTreeView, model: ComponentTreeModel, on_tree_selection_callback: Callable[[np.ndarray], None]
    ) -> None:
        self._view = view
        self._model = model
        self._on_tree_selection_callback = on_tree_selection_callback
        self._q_model = QStandardItemModel()
        self._q_model.setHorizontalHeaderLabels(["Component Tree"])
        root = self._q_model.invisibleRootItem()
        parent = self._add_children_to_parent(self._model.tree, None)
        root.appendRow(parent)

    @property
    def model_for_qt_tree(self) -> QStandardItemModel:
        return self._q_model

    def _add_children_to_parent(self, node: Node, parent: QStandardItem | None) -> QStandardItem:
        if parent is None:
            parent = self._create_q_item_from_node(node)
        for child in node.children:
            q_child = self._create_q_item_from_node(child)
            parent.appendRow(q_child)
            self._add_children_to_parent(child, q_child)
        return parent

    def _create_q_item_from_node(self, node: Node) -> QStandardItem:
        item = QStandardItem(node.name)
        item.component_index = node.component_index
        return item

    def on_selection_changed(self, q_items: list[QStandardItem]) -> None:
        component_indices = [item.component_index for item in q_items]
        all_selected_component_indices = self._model.get_all_sub_component_indices(component_indices)
        self._on_tree_selection_callback(all_selected_component_indices)
