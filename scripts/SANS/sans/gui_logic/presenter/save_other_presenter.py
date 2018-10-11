# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from mantid import ConfigService
from mantid import AnalysisDataService
from sans.algorithm_detail.batch_execution import save_workspace_to_file
import os
from mantid.api import WorkspaceGroup


class SaveOtherPresenter():
    def __init__(self, parent_presenter=None):
        self._parent_presenter = parent_presenter
        self._view = None
        self.current_directory = ConfigService['defaultsave.directory']
        self.filename = ''
        self.workspace_list = AnalysisDataService.getObjectNames()

    def set_view(self, view=None):
        if view:
            self._view = view
            self._view.subscribe(self)
            self._view.current_directory = self.current_directory
            self._view.populate_workspace_list(self.workspace_list)

    def on_file_name_changed(self, file_name):
        self.filename = file_name

    def on_browse_clicked(self):
        self.current_directory = self._view.launch_file_browser(self.current_directory)
        self._view.current_directory = self.current_directory

    def on_save_clicked(self):
        file_formats = self._view.get_save_options()
        if not file_formats:
            return
        selected_workspaces = self.get_workspaces()
        selected_filenames = self.get_filenames(selected_workspaces, self.filename)
        for name_to_save, filename in zip(selected_workspaces, selected_filenames):
            save_workspace_to_file(name_to_save, file_formats, filename)

    def on_item_selection_changed(self):
        self.selected_workspaces = self._view.get_selected_workspaces()
        if len(self.selected_workspaces) > 1:
            self._view.disable_filename()
        else:
            self._view.enable_filename()

    def on_directory_changed(self, directory):
        self.current_directory = directory

    def on_cancel_clicked(self):
        self._view.done(0)

    def show(self):
        self._view.show()

    def get_filenames(self, selected_workspaces, filename):
        if filename and len(selected_workspaces) == 1:
            return [os.path.join(self.current_directory, filename)]
        else:
            return [os.path.join(self.current_directory, x) for x in selected_workspaces]

    def get_workspaces(self):
        simple_list = self._view.get_selected_workspaces()
        for workspace_name in simple_list:
            workspace = AnalysisDataService.retrieve(workspace_name)
            if issubclass(type(workspace),WorkspaceGroup):
                simple_list.remove(workspace_name)
                simple_list += list(workspace.getNames())
        return list(set(simple_list))
