# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
from qtpy.QtWidgets import QFileDialog
from qtpy.QtCore import Signal

from .model import RawDataExplorerModel
from .view import RawDataExplorerView
from qtpy.QtCore import *

from os import path


class RawDataExplorerPresenter(QObject):
    """
    TODO
    """
    def __init__(self, parent=None, view=None, model=None):
        """
        TODO
        :param parent: The parent of the object, if there is one
        :param view: Optional - a view to use instead of letting the
                     class create one (intended for testing)
        :param model: Optional - a model to use instead of letting
                      the class create one (intended for testing)
        """
        # Create model and view, or accept mocked versions
        super().__init__(parent)
        self.view = RawDataExplorerView(parent) if view is None else view
        self.model = RawDataExplorerModel(self) if model is None else model

        self.working_dir = "/home"  # TODO set a better one

        self.view.set_file_model(self.model.file_model)

        self.view.file_tree_path_changed.connect(self.on_file_dialog_choice)
        self.view.repositoryPath.editingFinished.connect(self.on_qlineedit)

        self.view.instrumentSelector.currentIndexChanged.connect(self.model.on_instrument_changed)

        self.view.fileTree.clicked.connect(self.model.on_file_clicked)
        self.view.fileTree.doubleClicked.connect(self.model.on_file_clicked)

        self.model.show_ws.connect(self.view.show_ws)
        # self.view.instrumentChanged.connect(self.instrumentChanged)

    def set_working_directory(self, new_working_directory):
        """
        Change the current working directory and update the tree view
        @param new_working_directory : the path to the new working directory, which exists
        """
        self.working_dir = new_working_directory
        self.view.repositoryPath.setText(self.working_dir)
        self.model.file_model.setRootPath(self.working_dir)
        self.view.fileTree.setRootIndex(self.model.file_model.index(self.working_dir))

    def on_file_dialog_choice(self, new_directory):
        """
        Slot triggered when the user selects a new working directory from a dialog window
        @param new_directory : the directory selected by the user. None if they cancelled.
        """
        if new_directory:
            self.set_working_directory(new_directory)

    def on_qlineedit(self):
        """
        Slot triggered when the user finishes writing a new directory path in the editable field
        Fetch the new value and check if it is a valid directory. If not, does nothing.
        """
        new_dir = self.view.repositoryPath.text()
        if path.isdir(new_dir):
            self.set_working_directory(self.view.repositoryPath.text())
