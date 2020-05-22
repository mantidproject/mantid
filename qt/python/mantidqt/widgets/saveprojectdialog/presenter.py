# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench

from mantidqt.widgets.saveprojectdialog.view import ProjectSaveDialogView
from workbench.config import CONF


class ProjectSaveDialogPresenter:

    def __init__(self, project, view=None):
        self.view = view if view else ProjectSaveDialogView()

        self.project = project

        self.view.browse_push_button.clicked.connect(self.browse_button_clicked)
        self.view.location_line_edit.textChanged.connect(self.location_selected)
        self.view.accepted.connect(self.save_as)
        self.view.rejected.connect(self.cancel)

        self.view.set_save_altered_workspaces_only(project.save_altered_workspaces_only)
        self.view.set_location(project.last_project_location)
        self.view.set_ok_enabled(self.view.get_location() != "")
        self.view.exec()

    def location_selected(self):
        self.view.set_ok_enabled(True)

    def browse_button_clicked(self):
        filename = self.project._save_file_dialog()

        if filename:
            self.view.set_location(filename)

    def save_as(self):
        self.project.save_altered_workspaces_only = self.view.get_save_altered_workspaces_only()
        CONF.set('project/save_altered_workspaces_only', self.view.get_save_altered_workspaces_only())
        self.project.save_as(path=self.view.get_location())

    def cancel(self):
        self.project.saving_cancelled = True
