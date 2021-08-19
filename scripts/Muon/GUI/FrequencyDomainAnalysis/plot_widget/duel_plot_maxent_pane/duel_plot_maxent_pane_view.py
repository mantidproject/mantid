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

class DuelPlotMaxentPaneView(BasePaneView):

    def __init__(self, parent=None):
        super().__init__(parent=parent)
        self._selectors_layout = QtWidgets.QHBoxLayout()

        self._data_selector = CyclicDataSelectorView(self)
        self._data_selector.set_data_combo_box_label("Display:")
        self._data_selector.set_data_combo_box_label_width(300)
        self._selectors_layout.addWidget(self._data_selector)

        self._selectors_layout.setSpacing(100)
        self.verticalLayout.insertItem(1, self._selectors_layout)

    def set_slot_for_selection_changed(self, slot):
        self._data_selector.set_slot_for_dataset_changed(slot)

    def update_selection(self, selections)->None:
        self._data_selector.update_dataset_name_combo_box(selections)

    @property
    def get_selection_for_plot(self):
        return self._data_selector.current_dataset_name
