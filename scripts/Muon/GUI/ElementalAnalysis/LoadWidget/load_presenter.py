from __future__ import absolute_import


class LoadWidget(object):
    def __init__(self, view, model):
        self.view = view
        self.model = model

    def register_button_clicked(self, slot):
        self.view.on_button_clicked(slot)

    def unregister_button_clicked(self, slot):
        self.view.unreg_on_button_clicked(slot)

    def register_spinbox_val_changed(self, slot):
        self.view.on_spinbox_val_changed(slot)

    def unregister_spinbox_val_changed(self, slot):
        self.view.unreg_on_spinbox_val_changed(slot)
