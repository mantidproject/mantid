from __future__ import print_function


class PeriodicTablePresenter(object):
    def __init__(self, view, model):
        self.view = view
        self.model = model
        self.set_buttons()

    @property
    def widget(self):
        return self.view

    def set_buttons(self):
        for el in self.view.ptable.elements:
            if el.symbol in self.model.peak_data:
                self.view.ptable.enableElementButton(el.symbol)
            else:
                self.view.ptable.disableElementButton(el.symbol)

    def is_selected(self, element):
        return self.widget.ptable.isElementSelected(element)
