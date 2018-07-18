from __future__ import print_function

from Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table_model import PeriodicTableModel
from Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table_view import PeriodicTableView


class PeriodicTable(object):
    def __init__(self):
        self.view = PeriodicTableView()
        self.model = PeriodicTableModel()
        self.set_buttons()

    @property
    def widget(self):
        return self.view

    @property
    def peak_data(self):
        return self.model.peak_data

    @property
    def selection(self):
        return self.view.ptable.getSelection()

    def set_buttons(self):
        for el in self.view.ptable.elements:
            if el.symbol in self.model.peak_data:
                self.view.ptable.enableElementButton(el.symbol)
            else:
                self.view.ptable.disableElementButton(el.symbol)

    def element_data(self, element):
        try:
            return self.model.peak_data[element]
        except KeyError:
            return None

    def select_element(self, element, deselect=False):
        self.view.ptable.setElementSelected(element, not deselect)

    def is_selected(self, element):
        return self.view.ptable.isElementSelected(element)

    def add_elements(self, *elements):
        self.view.ptable.setSelection(elements)

    def register_table_changed(self, slot):
        self.view.sig_table_changed.connect(slot)

    def unregister_table_changed(self, slot):
        self.view.sig_table_changed.disconnect(slot)

    def register_table_lclicked(self, slot):
        self.view.sig_table_lclicked.connect(slot)

    def unregister_table_lclicked(self, slot):
        self.view.sig_table_lclicked.disconnect(slot)

    def register_table_rclicked(self, slot):
        self.view.sig_table_rclicked.connect(slot)

    def unregister_table_rclicked(self, slot):
        self.view.sig_table_rclicked.disconnect(slot)
