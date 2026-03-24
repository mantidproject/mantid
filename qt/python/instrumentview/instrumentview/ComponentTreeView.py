# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from qtpy.QtWidgets import QTreeView

from mantidqt.utils.qt.qappthreadcall import run_on_qapp_thread


@run_on_qapp_thread()
class ComponentTreeView(QTreeView):
    def subscribe_presenter(self, presenter) -> None:
        self._presenter = presenter
        self.setModel(self._presenter.model_for_qt_tree)
        self.selectionModel().selectionChanged.connect(self.on_selection_changed)

    def on_selection_changed(self, _selected, _deselected):
        items = [self.model().itemFromIndex(index) for index in self.selectionModel().selectedIndexes()]
        self._presenter.on_selection_changed(items)
