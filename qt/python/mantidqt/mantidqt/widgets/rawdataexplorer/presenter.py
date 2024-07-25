# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import os.path
from qtpy.QtWidgets import QAbstractItemView
from qtpy.QtCore import *
from qtpy.QtGui import QGuiApplication

from mantid.simpleapi import config
from mantid.api import PreviewType
from mantid.kernel import logger

from .model import RawDataExplorerModel, PreviewModel
from .view import RawDataExplorerView, PreviewView


class PreviewPresenter:

    """
    Raw data explorer main view.
    """

    _main_view = None

    """
    Associated view.
    """
    _view = None

    """
    Raw data explorer main model.
    """
    _main_model = None

    """
    Preview model.
    """
    _model = None

    """
    Conversion dictionary between PreviewType and PreviewView
    """
    _conversion_dict = {
        PreviewType.IVIEW: PreviewView.IVIEW,
        PreviewType.SVIEW: PreviewView.SVIEW,
        PreviewType.PLOT1D: PreviewView.PLOT1D,
        PreviewType.PLOT2D: PreviewView.PLOT2D,
        PreviewType.PLOTSPECTRUM: PreviewView.PLOTSPECTRUM,
    }

    def __init__(self, main_view: RawDataExplorerView, view: PreviewView, main_model: RawDataExplorerModel, model: PreviewModel):
        self._main_view = main_view
        self._view = view
        view.set_presenter(self)
        self._main_model = main_model
        self._model = model

        preview_type = self._model.get_preview_type()
        if preview_type in self._conversion_dict:
            self._view.set_type(self._conversion_dict[preview_type])

        workspace_name = self._model.get_workspace_name()
        self._view.show_workspace(workspace_name)
        self._model.sig_workspace_changed.connect(self.on_workspace_changed)
        self._model.sig_request_close.connect(lambda: self._view.sig_request_close.emit())

    def close_preview(self, requested_by_user: bool = True):
        """
        Slot triggered when the view is closed.
        @param requested_by_user: whether closing the preview was requested by the user or by the system
        """
        self._main_view.del_preview(self._view)
        self._main_model.del_preview(self._model)

        if requested_by_user:
            self._main_view.clear_selection()
        else:
            self._main_view.select_last_clicked()

    def on_workspace_changed(self):
        """
        Slot triggered when the workspace in the model is modified.
        """
        ws_name = self._model.get_workspace_name()
        self._view.change_workspace(ws_name)

    def get_main_view(self):
        """
        Getter for the main view
        """
        return self._main_view


class RawDataExplorerPresenter(QObject):
    """
    Presenter for the RawDataExplorer widget
    """

    def __init__(self, parent=None, view: RawDataExplorerView = None, model: RawDataExplorerModel = None):
        """
        @param parent: The parent of the object, if there is one
        @param view: Optional - a view to use instead of letting the class create one (intended for testing)
        @param model: Optional - a model to use instead of letting the class create one (intended for testing)
        """
        # Create model and view, or accept mocked versions
        super().__init__(parent)
        self.view = RawDataExplorerView(self, parent) if view is None else view
        self.model = RawDataExplorerModel(self) if model is None else model

        self._is_accumulating = False

        self._set_initial_directory()

        self.setup_connections()

    def cancel_memory_update(self):
        """
        Cancel any further memory updates, so the memory manager can stop.
        """
        self.model.memory_manager.cancel_memory_update()

    def setup_connections(self):
        """
        Set up the connections between signals and slots in the widget.
        """
        self.view.file_tree_path_changed.connect(self.on_file_dialog_choice)
        self.view.repositoryPath.editingFinished.connect(self.on_qlineedit)
        self.view.fileTree.sig_accumulate_changed.connect(self.on_accumulate_changed)
        self.model.sig_new_preview.connect(self.on_new_preview)

    def _set_initial_directory(self):
        """
        Set the directory in which the tree starts.
        """

        data_search_dirs = config.getDataSearchDirs()

        for directory in data_search_dirs:
            if os.path.isdir(directory):
                # we take the first valid directory in the user defined list
                path = directory
                break
        else:
            # if none were found, we use the root as default
            path = os.path.abspath(os.sep)

        self.set_working_directory(path)

    def set_working_directory(self, new_working_directory: str):
        """
        Change the current working directory and update the tree view
        @param new_working_directory : the path to the new working directory, which exists
        """
        # discover the directory's content first, so that it is already cached when Qt creates the new root
        # on linux, it noticeably improves reactivity when opening a big directory
        os.listdir(new_working_directory)

        # set the new root
        self.view.repositoryPath.setText(new_working_directory)
        self.view.fileTree.model().setRootPath(new_working_directory)
        self.view.fileTree.setRootIndex(self.view.fileTree.model().index(new_working_directory))

    def on_file_dialog_choice(self, new_directory: str):
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
        if os.path.isdir(new_dir):
            self.set_working_directory(new_dir)
        else:
            logger.warning("Not a valid path for a directory to set as root for the explorer.")

    def on_accumulate_changed(self, is_now_accumulating: bool):
        """
        Slot triggered by the user entering or exiting the accumulation mode by pressing Ctrl.
        Manage the file selection mode.
        @param is_now_accumulating: whether the user is entering or leaving the accumulating mode
        """
        self._is_accumulating = is_now_accumulating

        if not is_now_accumulating:
            # if we are leaving accumulating mode, the selection must be cleared
            self.view.fileTree.selectionModel().clearSelection()

        self.set_selection_mode()

    def on_selection_changed(self):
        """
        Triggered when the selection changed in the file system widget.
        """
        last_clicked = self.view.get_last_clicked()

        if not os.path.isfile(last_clicked):
            # if the user clicked on a directory, do nothing preview-wise
            return

        QGuiApplication.setOverrideCursor(Qt.WaitCursor)
        self.set_selection_mode(False)
        self.model.modify_preview(last_clicked)
        QGuiApplication.restoreOverrideCursor()
        self.set_selection_mode(True)

    def on_new_preview(self, preview_model: PreviewModel):
        """
        Triggered when a new preview model has been added to the model. This is
        creating a dedicated MVP for this preview.
        @param preview_model: the preview model of the new preview
        """
        view = self.view.add_preview()
        PreviewPresenter(self.view, view, self.model, preview_model)

    def is_accumulating(self) -> bool:
        """
        @return whether or not the user is currently accumulating workspaces.
        """
        return self._is_accumulating

    def set_selection_mode(self, can_select: bool = True):
        """
        Set the current selection mode for the widget.
        @param can_select: True if selection is allowed.
        """
        if not can_select:
            self.view.fileTree.setSelectionMode(QAbstractItemView.NoSelection)
        elif self.is_accumulating():
            self.view.fileTree.setSelectionMode(QAbstractItemView.MultiSelection)
        else:
            self.view.fileTree.setSelectionMode(QAbstractItemView.SingleSelection)
