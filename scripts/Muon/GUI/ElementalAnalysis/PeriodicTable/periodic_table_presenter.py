from __future__ import print_function


class PeriodicTablePresenter(object):
    def __init__(self, view, model):
        self.view = view
        self.model = model
        self.set_buttons()

    @property
    def peak_data(self):
        return self.model.peak_data

    @property
    def selection(self):
        return self.view.ptable.getSelection()

    def set_buttons(self):
        for el in self.view.ptable.elements:
            self.view.ptable.setElementSelected(el.symbol, False)
            if el.symbol in self.model.peak_data:
                self.view.ptable.enableElementButton(el.symbol)
            else:
                self.view.ptable.disableElementButton(el.symbol)

    def element_data(self, element):
        return self.model.peak_data[element]

    def select_element(self, element):
        self.view.ptable.setElementSelected(element, True)

    def deselect_element(self, element):
        self.view.ptable.setElementSelected(element, False)

    def is_selected(self, element):
        return self.view.ptable.isElementSelected(element)

    def add_elements(self, *elements):
        self.view.ptable.setSelection(elements)

    def register_table_changed(self, slot):
        self.view.on_table_changed(slot)

    def unregister_table_changed(self, slot):
        self.view.unreg_on_table_changed(slot)

    def register_table_lclicked(self, slot):
        self.view.on_table_lclicked(slot)

    def unregister_table_lclicked(self, slot):
        self.view.unreg_on_table_lclicked(slot)

    def register_table_rclicked(self, slot):
        self.view.on_table_rclicked(slot)

    def unregister_table_rclicked(self, slot):
        self.view.unreg_on_table_rclicked(slot)

    def set_peak_datafile(self, filename):
        self.model.peak_data_file = filename
        self.set_buttons()
