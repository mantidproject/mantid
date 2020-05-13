# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench

from qtpy.QtWidgets import QDialogButtonBox

from mantidqt.project.saveprojectdialog.view import ProjectSaveDialogView


class ProjectSaveDialogPresenter:

    def __init__(self, project, view=None):
        self.view = view if view else ProjectSaveDialogView()

        self.project = project

        self.view.browse_push_button.clicked.connect(lambda: self.browse_button_clicked())
        self.view.location_line_edit.textChanged.connect(lambda: self.location_selected())
        self.view.accepted.connect(lambda: self.save_as())

        self.view.set_location(project.last_project_location)
        self.view.buttonBox.button(QDialogButtonBox.Ok).setEnabled(self.view.get_location() != "")
        self.view.open()

    def location_selected(self):
        self.view.buttonBox.button(QDialogButtonBox.Ok).setEnabled(True)

    def browse_button_clicked(self):
        filename = self.project._save_file_dialog()

        if filename:
            self.view.set_location(filename)

    def save_as(self):
        path = self.view.get_location()
        if path:
            self.project.do_save(path=path,
                                 altered_workspaces_only=self.view.get_save_altered_workspaces_only())
