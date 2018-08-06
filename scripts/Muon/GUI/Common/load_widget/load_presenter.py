from __future__ import absolute_import, print_function

from Muon.GUI.Common import thread_model


class LoadPresenter(object):
    def __init__(self, view, model):
        self.view = view
        self.model = model
        self.thread = None

        self.view.on_load_clicked(self.load_run)
        self.view.on_co_add_clicked(self.co_add_run)
        self.view.on_spinbox_changed(self.model.set_run)

    def setup_thread(self):
        self.thread = self.new_thread()
        self.thread.threadWrapperSetUp(
            self.disable_buttons, self.end_thread)
        self.thread.loadData({})
        self.thread.start()

    def load_run(self):
        self.model.co_runs = []
        self.model.co_load_ws = None
        self.setup_thread()

    def co_add_run(self):
        if self.model.run not in self.model.co_runs:
            self.model.co_runs.append(self.model.run)
            self.setup_thread()

    def disable_buttons(self):
        self.view.spinbox.setEnabled(False)
        self.view.load_button.setEnabled(False)
        self.view.co_button.setEnabled(False)

    def enable_buttons(self):
        self.view.spinbox.setEnabled(True)
        self.view.load_button.setEnabled(True)
        self.view.co_button.setEnabled(True)

    def cancel(self):
        if self.thread is not None:
            self.thread.cancel()

    def end_thread(self):
        self.enable_buttons()
        self.thread.threadWrapperTearDown(
            self.disable_buttons, self.end_thread)
        self.thread.deleteLater()
        self.thread = None

    def new_thread(self):
        return thread_model.ThreadModel(self.model)
