# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench

from qtpy.QtWidgets import QFileDialog

from mantidqt.io import open_a_file_dialog
from mantidqt.project.saveprojectdialog.view import ProjectSaveDialogView


class ProjectSaveDialogPresenter:

    def __init__(self, project, view=None):
        self.view = view if view else ProjectSaveDialogView()
        self.view.show()

        self.project = project

        self.view.browse_push_button.clicked.connect(lambda: self.browse_button_clicked())
        self.view.accepted.connect(lambda: self.save_as())

    def browse_button_clicked(self):
        filename = self.project._save_file_dialog()
        self.view.set_location(filename)

    def save_as(self):
        # check path is valid
        self.project.do_save(path=self.view.get_location())
