# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from instrumentview.ComponentTreeView import ComponentTreeView
from instrumentview.ComponentTreeModel import ComponentTreeModel
from qtpy.QtCore import Qt
from qtpy.QtGui import QStandardItemModel, QStandardItem
from typing import Callable
import numpy as np

_COMPONENT_INDEX_ROLE = Qt.UserRole + 1


class ComponentTreePresenter:
    _PLACEHOLDER_TEXT = "##placeholder##"

    def __init__(
        self, view: ComponentTreeView, model: ComponentTreeModel, on_tree_selection_callback: Callable[[np.ndarray], None]
    ) -> None:
        self._view = view
        self._model = model
        self._on_tree_selection_callback = on_tree_selection_callback
        self._q_model = QStandardItemModel()
        self._q_model.setHorizontalHeaderLabels(["Component Tree"])
        root_idx = self._model.root_index()
        root_item = self._create_q_item(self._model.root_name(), root_idx)
        self._add_placeholder_if_has_children(root_item)
        self._q_model.invisibleRootItem().appendRow(root_item)

    @property
    def model_for_qt_tree(self) -> QStandardItemModel:
        return self._q_model

    def _create_q_item(self, name: str, component_index: int) -> QStandardItem:
        item = QStandardItem(name)
        item.setData(component_index, _COMPONENT_INDEX_ROLE)
        return item

    def _create_placeholder(self) -> QStandardItem:
        item = QStandardItem(self._PLACEHOLDER_TEXT)
        item.setData(-1, _COMPONENT_INDEX_ROLE)
        return item

    def _add_placeholder_if_has_children(self, item: QStandardItem) -> None:
        if self._model.has_children(item.data(_COMPONENT_INDEX_ROLE)):
            item.appendRow(self._create_placeholder())

    def on_item_expanded(self, item: QStandardItem) -> None:
        """Populate children of item on first expansion (lazy loading)."""
        if item.rowCount() == 1 and item.child(0).text() == self._PLACEHOLDER_TEXT:
            item.removeRow(0)
            component_index = item.data(_COMPONENT_INDEX_ROLE)
            for name, child_idx in self._model.get_children(component_index):
                child_item = self._create_q_item(name, child_idx)
                self._add_placeholder_if_has_children(child_item)
                item.appendRow(child_item)

    def on_selection_changed(self, q_items: list[QStandardItem]) -> None:
        component_indices = [item.data(_COMPONENT_INDEX_ROLE) for item in q_items if item.data(_COMPONENT_INDEX_ROLE) != -1]
        all_selected_component_indices = self._model.get_all_sub_component_indices(component_indices)
        self._on_tree_selection_callback(all_selected_component_indices)
