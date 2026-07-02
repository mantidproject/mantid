# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from qtpy.QtWidgets import QTreeView
from mantidqt.utils.qt.qappthreadcall import QAppThreadCall


class ComponentTreeView(QTreeView):
    def subscribe_presenter(self, presenter) -> None:
        QAppThreadCall(self._subscribe_presenter)(presenter)

    def _subscribe_presenter(self, presenter) -> None:
        self._presenter = presenter
        self.setModel(self._presenter.model_for_qt_tree)
        self.selectionModel().selectionChanged.connect(self.on_selection_changed)
        self.expanded.connect(self.on_item_expanded)

    def on_item_expanded(self, index) -> None:
        QAppThreadCall(self._on_item_expanded)(index)

    def _on_item_expanded(self, index) -> None:
        item = self.model().itemFromIndex(index)
        self._presenter.on_item_expanded(item)

    def on_selection_changed(self, selected, deselected) -> None:
        QAppThreadCall(self._on_selection_changed)(selected, deselected)

    def _on_selection_changed(self, _selected, _deselected):
        items = [self.model().itemFromIndex(index) for index in self.selectionModel().selectedIndexes()]
        self._presenter.on_selection_changed(items)
