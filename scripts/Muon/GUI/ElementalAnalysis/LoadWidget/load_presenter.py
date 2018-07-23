from __future__ import absolute_import

from Muon.GUI.Common import thread_model



class LoadPresenter(object):
    def __init__(self, view, model):
        self.view = view
        self.model = model

        self.thread = None

        self.view.on_file_browser_complete(self.load_file)

    def load_file(self, filename):
        self.thread = self.new_thread()
        self.thread.threadWrapperSetUp(self.disable_browse, self.end_thread)
        self.thread.loadData({"Filename": filename})
        self.thread.start()

    def register_button_clicked(self, slot):
        self.view.on_button_clicked(slot)

    def unregister_button_clicked(self, slot):
        self.view.unreg_on_button_clicked(slot)

    def register_spinbox_val_changed(self, slot):
        self.view.on_spinbox_val_changed(slot)

    def unregister_spinbox_val_changed(self, slot):
        self.view.unreg_on_spinbox_val_changed(slot)

    def register_spinbox_submit(self, slot):
        self.view.on_spinbox_submit(slot)

    def unregister_spinbox_submit(self, slot):
        self.view.unreg_on_spinbox_submit(slot)

    def disable_browse(self):
        self.view.load_button.setEnabled(False)

    def enable_browse(self):
        self.view.load_button.setEnabled(True)

    def cancel(self):
        if self.thread is not None:
            self.thread.cancel()

    def end_thread(self):
        self.enable_browse()
        self.thread.threadWrapperTearDown(self.disable_browse, self.end_thread)
        self.thread.deleteLater()
        self.thread = None

    def new_thread(self):
        return thread_model.ThreadModel(self.model)
    
