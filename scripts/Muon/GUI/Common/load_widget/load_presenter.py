# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import absolute_import, print_function

from Muon.GUI.Common import thread_model


class LoadPresenter(object):
    def __init__(self, view, load_model, co_model):
        self.view = view
        self.load_model = load_model
        self.co_model = co_model
        self.load_thread = None
        self.co_thread = None

        self.view.on_load_clicked(self.equalise_loaded_runs)
        self.view.on_load_clicked(self.equalise_last_loaded_run)
        self.view.on_load_clicked(self.load_run)
        self.view.on_load_clicked(self.co_model.wipe_co_runs)
        self.view.on_co_add_clicked(self.equalise_loaded_runs)
        self.view.on_co_add_clicked(self.equalise_last_loaded_run)
        self.view.on_co_add_clicked(self.co_add_run)
        self.view.on_spinbox_changed(self.update_models)

    def equalise_loaded_runs(self):
        loaded_runs = max(
            self.co_model.loaded_runs,
            self.load_model.loaded_runs)
        self.co_model.loaded_runs = loaded_runs
        self.load_model.loaded_runs = loaded_runs

    def equalise_last_loaded_run(self):
        last_run = max(
            self.co_model.last_loaded_runs,
            self.load_model.last_loaded_runs)
        self.co_model.last_loaded_runs = last_run
        self.load_model.last_loaded_runs = last_run

    def update_models(self, run):
        self.load_model.set_run(run)
        self.co_model.set_run(run)

    def load_run(self):
        self.load_thread = self.new_thread(self.load_model)
        self.load_thread.threadWrapperSetUp(
            self.disable_buttons, self.end_load_thread)
        self.load_thread.start()

    def co_add_run(self):
        self.co_thread = self.new_thread(self.co_model)
        self.co_thread.threadWrapperSetUp(
            self.disable_buttons, self.end_co_thread)
        self.co_thread.start()

    def disable_buttons(self):
        self.view.disable_buttons()

    def enable_buttons(self):
        self.view.enable_buttons()

    def end_load_thread(self):
        self.enable_buttons()
        self.load_thread.deleteLater()
        self.load_thread = None

    def end_co_thread(self):
        self.enable_buttons()
        self.co_thread.deleteLater()
        self.co_thread = None

    def new_thread(self, model):
        return thread_model.ThreadModel(model)

    def last_loaded_run(self):
        try:
            return self.load_model.last_loaded_runs[-1]
        except IndexError:
            return None

    def on_loading_finished(self, slot):
        self.view.on_loading_finished(slot)

    def unreg_on_loading_finished(self, slot):
        self.view.unreg_on_loading_finished(slot)
