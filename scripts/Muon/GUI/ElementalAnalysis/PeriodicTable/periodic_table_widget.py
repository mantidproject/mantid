from __future__ import absolute_import

from Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table_view import PeriodicTableView
from Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table_presenter import PeriodicTablePresenter

from PyQt4 import QtGui


class PeriodicTable(QtGui.QWidget):
    def __init__(self, parent=None):
        super(PeriodicTable, self).__init__(parent)
        view = PeriodicTableView(parent)
        self._presenter = PeriodicTablePresenter(view)

    @property
    def presenter(self):
        return self._presenter

    @property
    def widget(self):
        return self._presenter.widget

    @property
    def selection(self):
        return self.widget.ptable.getSelection()

    def is_selected(self, element):
        return self.widget.ptable.isElementSelected(element)

    def select_element(self, element, deselect=False):
        self.widget.ptable.setElementSelected(element, not deselect)

    def add_elements(self, *elements):
        self.widget.ptable.setSelection(elements)

    def register_table_changed(self, slot):
        self._presenter.widget.sig_table_changed.connect(slot)

    def unregister_table_changed(self, slot):
        self._presenter.widget.sig_table_changed.disconnect(slot)

    def register_table_lclicked(self, slot):
        self._presenter.widget.sig_table_lclicked.connect(slot)

    def unregister_table_lclicked(self, slot):
        self._presenter.widget.sig_table_lclicked.disconnect(slot)

    def register_table_rclicked(self, slot):
        self._presenter.widget.sig_table_rclicked.connect(slot)

    def unregister_table_rclicked(self, slot):
        self._presenter.widget.sig_table_rclicked.disconnect(slot)
