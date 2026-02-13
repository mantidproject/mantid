# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench

from mantidqt.widgets.saveprojectdialog.view import ProjectSaveDialogView


class ProjectSaveDialogPresenter:
    """
    The presenter for the dialog which allows users to select the location for the
    project they are saving and choose between saving all workspaces or altered
    workspaces only.
    """

    def __init__(self, project, conf=None, view=None):
        """
        Create a presenter for the dialog for saving a project.
        :param project: The project being saved.
        :param conf: An optional UserConfig to which the user's selection will be saved.
        :param view: A view to display the dialog. If None uses ProjectSaveDialogView.
        """
        self.view = view or ProjectSaveDialogView()

        self.project = project
        self.conf = conf

        self.view.browse_push_button.clicked.connect(self.browse_button_clicked)
        self.view.location_line_edit.textChanged.connect(self.location_selected)
        self.view.accepted.connect(self.save_as)
        self.view.rejected.connect(self.cancel)

        self.view.set_save_altered_workspaces_only(project.save_altered_workspaces_only)
        self.view.set_location(project.last_project_location)
        self.view.set_ok_enabled(self.view.get_location() != "")
        self.view.exec()

    def location_selected(self):
        """
        Enables the OK button after a save location has been chosen.
        """
        self.view.set_ok_enabled(True)

    def browse_button_clicked(self):
        """
        Opens a dialog for choosing a save location and sets the location line edit text to this path.
        """
        filename = self.project._save_file_dialog()

        if filename:
            self.view.set_location(filename)

    def save_as(self):
        """
        Sets whether the user selected "save all workspaces" or "save altered workspaces only" in the UserConfig so it
        is remembered for future saves, and saves the project.
        """
        self.project.save_altered_workspaces_only = self.view.get_save_altered_workspaces_only()

        if self.conf:
            self.conf.set("project/save_altered_workspaces_only", self.view.get_save_altered_workspaces_only())

        self.project.save_as(path=self.view.get_location())

    def cancel(self):
        """
        Informs the project being saved that "Cancel" was clicked so the saving is aborted.
        """
        self.project.saving_cancelled = True
