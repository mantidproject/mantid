# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtWidgets

from Muon.GUI.Common.plot_widget.base_pane.base_pane_view import BasePaneView
from Muon.GUI.Common.data_selectors.cyclic_data_selector_view import CyclicDataSelectorView


# might need to have a baseqidget instance and add to layout
# then have access methods into the base widget

class RawPaneView(BasePaneView):

    def __init__(self, parent=None):
        super().__init__(parent=parent)
        self._selectors_layout = QtWidgets.QHBoxLayout()

        self._runs_selector = CyclicDataSelectorView(self)
        self._runs_selector.set_data_combo_box_label("Runs:")
        self._runs_selector.set_data_combo_box_label_width(50)
        self._selectors_layout.addWidget(self._runs_selector)

        self._selectors_layout.setSpacing(100)

        self._detectors_selector = CyclicDataSelectorView(self)
        self._detectors_selector.set_data_combo_box_label("Detectors:")
        self._detectors_selector.set_data_combo_box_label_width(50)
        self._selectors_layout.addWidget(self._detectors_selector)

        self.verticalLayout.insertItem(1, self._selectors_layout)

    def set_slot_for_detectors_changed(self, slot):
        self._detectors_selector.set_slot_for_dataset_changed(slot)

    def update_detectors(self, detectors: list)->None:
        self._detectors_selector.update_dataset_name_combo_box(detectors)

    @property
    def get_detectors(self):
        return self._detectors_selector.current_dataset_name
        self._runs_selector.set_data_combo_box_label_width(50)
        self.verticalLayout.insertWidget(1, self._runs_selector)

    def set_slot_for_runs_changed(self, slot):
        self._runs_selector.set_slot_for_dataset_changed(slot)

    def update_runs(self, detectors: list)->None:
        self._runs_selector.update_dataset_name_combo_box(detectors)

    @property
    def get_run(self):
        return self._runs_selector.current_dataset_name
