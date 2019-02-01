# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#

def find_number_of_workspaces_in_directory(path):
    files = []
def replace_space_with_t(string):

def sort_by_last_modified(paths):


class ProjectRecoveryModel(object):
    def __init__(self, project_recovery, presenter):
        self.presenter = presenter
        self.project_recovery = project_recovery

        self.recovery_running = False
        self.failed_run = False

        self.rows = [[]]

    def get_row(self, c):
        if isinstance(c, str):

        if isinstance(c, int):

    def start_mantid_normally(self):

    def recover_selected_checkpoint(self, selected):

    def open_selected_in_editor(self, selected):

    def get_failed_run(self):

    def has_recovery_started(self):
        return self.recovery_running

    def decide_last_checkpoint(self):

    def fill_rows(self):

    def get_number_of_checkpoints(self):

    def _fill_first_row(self):

    def _fill_row(self, path, checkpoint_name):

    def _update_checkpoint_tried(self, checkpoint_name):

    def _check_recover_was_a_success(self, project_file):

    def _create_thread_and_manage(self, checkpoint):