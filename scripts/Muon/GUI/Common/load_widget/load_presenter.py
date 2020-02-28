# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import absolute_import, print_function

from Muon.GUI.Common import thread_model
import mantid.simpleapi as mantid


class LoadPresenter(object):
    def __init__(self, view, load_model, co_model):
        self.view = view
        self.load_model = load_model
        self.co_model = co_model
        self.load_thread = None
        self.co_thread = None
        self._current_run = None

        self.view.on_load_clicked(self.load_run)
        self.view.on_load_clicked(self.co_model.wipe_co_runs)
        self.view.on_co_add_clicked(self.co_add_run)
        self.view.on_spinbox_changed(self.update_models)

    def equalise_last_loaded_run(self, runs):
        if self.co_model.loaded_runs == {} and self.load_model.loaded_runs == {}:
            return
        try:
            self._current_run = str(list(runs)[-1])
        except IndexError:
            self.view.warning("The run was not found.")
            return

    def set_coadd_loaded_run(self):
        self.equalise_last_loaded_run(self.co_model.loaded_runs.keys())

    def set_loaded_run(self):
        self.equalise_last_loaded_run(self.load_model.loaded_runs.keys())

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
        self.set_loaded_run()
        self.enable_buttons()
        self.load_thread.deleteLater()
        self.load_thread = None

    def end_co_thread(self):
        self.set_coadd_loaded_run()
        self.enable_buttons()
        self.co_thread.deleteLater()
        self.co_thread = None

    def new_thread(self, model):
        return thread_model.ThreadModel(model)

    def last_loaded_run(self):
        try:
            return self._current_run
        except IndexError:
            return None

    def on_loading_finished(self, slot):
        self.view.on_loading_finished(slot)

    def unreg_on_loading_finished(self, slot):
        self.view.unreg_on_loading_finished(slot)

    def get_run_num_loaded_detectors(self, run):
        num_detectors = 0
        try:
            group = mantid.mtd[run]
            num_detectors = len(group)
        except:
            num_detectors = 0
        return num_detectors
