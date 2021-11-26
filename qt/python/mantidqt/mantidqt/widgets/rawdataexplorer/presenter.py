# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#

import os.path
from qtpy.QtWidgets import QAbstractItemView
from qtpy.QtCore import *

from mantid.simpleapi import mtd
from mantid.api import PreviewManager, PreviewType

from .model import RawDataExplorerModel
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

    def __init__(self, main_view, view, main_model, model):
        self._main_view = main_view
        self._view = view
        view.set_presenter(self)
        self._main_model = main_model
        self._model = model

        preview_type = self._model.get_preview_type()
        if preview_type == PreviewType.IVIEW:
            self._view.set_type(PreviewView.IVIEW)
        if preview_type == PreviewType.SVIEW:
            self._view.set_type(PreviewView.SVIEW)
        if preview_type == PreviewType.PLOT2D:
            self._view.set_type(PreviewView.PLOT2D)
        if preview_type == PreviewType.PLOT1D:
            self._view.set_type(PreviewView.PLOT1D)
        if preview_type == PreviewType.PLOTSPECTRUM:
            self._view.set_type(PreviewView.PLOTSPECTRUM)

        workspace_name = self._model.get_workspace_name()
        self._view.show_workspace(workspace_name)

        self._model.sig_workspace_changed.connect(self.on_workspace_changed)

    def close_preview(self):
        """
        Triggered when the view is closed.
        """
        self._main_view.del_preview(self._view)
        self._main_model.del_preview(self._model)

    def on_workspace_changed(self):
        """
        Triggered when the workspace in the model is modified.
        """
        ws_name = self._model.get_workspace_name()
        self._view.change_workspace(ws_name)


class RawDataExplorerPresenter(QObject):
    """
    Presenter for the RawDataExplorer widget
    """

    def __init__(self, parent=None, view=None, model=None):
        """
        @param parent: The parent of the object, if there is one
        @param view: Optional - a view to use instead of letting the class create one (intended for testing)
        @param model: Optional - a model to use instead of letting the class create one (intended for testing)
        """
        # Create model and view, or accept mocked versions
        super().__init__(parent)
        self.view = RawDataExplorerView(self, parent) if view is None else view
        self.model = RawDataExplorerModel(self) if model is None else model

        self.displays = DisplayManager()

        # TODO remember previous one ? set to some default directory ? ManageUserDirectory ?
        self.working_dir = "~/mantid/build/ExternalData/Testing/Data/UnitTest/ILL"

        self.set_working_directory(self.working_dir)
        self.preview_manager = PreviewManager.Instance()

        self.setup_connections()

        self.model.sig_new_preview.connect(self.on_new_preview)

    def setup_connections(self):
        """
        Set up the connections between signals and slots in the widget.
        """
        self.view.file_tree_path_changed.connect(self.on_file_dialog_choice)
        self.view.repositoryPath.editingFinished.connect(self.on_qlineedit)

        self.view.accumulate.stateChanged.connect(self.on_accumulate_checked)

    def set_working_directory(self, new_working_directory):
        """
        Change the current working directory and update the tree view
        @param new_working_directory : the path to the new working directory, which exists
        """
        self.working_dir = new_working_directory
        self.view.repositoryPath.setText(self.working_dir)
        self.view.fileTree.model().setRootPath(self.working_dir)
        self.view.fileTree.setRootIndex(self.view.fileTree.model().index(self.working_dir))

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
        if os.path.isdir(new_dir):
            self.set_working_directory(self.view.repositoryPath.text())

    def on_accumulate_checked(self):
        """
        Slot triggered by checking the accumulate checkbox.
        Manage the file selection mode.
        """
        if self.is_accumulate_checked():
            self.view.fileTree.setSelectionMode(QAbstractItemView.MultiSelection)
        else:
            self.view.fileTree.setSelectionMode(QAbstractItemView.SingleSelection)
            self.view.fileTree.selectionModel().clearSelection()

    def on_selection_changed(self):
        """
        Triggered when the selection changed in the file system widget.
        """
        last_clicked = self.view.get_last_clicked()

        if not os.path.isfile(last_clicked):
            # if the user clicked on a directory, do nothing preview-wise
            return

        self.view.fileTree.setCursor(Qt.BusyCursor)

        self.model.modify_preview(last_clicked)
        self.view.fileTree.unsetCursor()

    def on_new_preview(self, previewModel):
        """
        Triggered when a new preview model has been added to the model. This is
        creating a dedicated MVP for this preview.
        """
        view = self.view.add_preview()
        PreviewPresenter(self.view, view, self.model, previewModel)

    def show_ws(self, ws_to_show):
        """
        Decide which view to call the workspace in.
        @param ws_to_show: the name of the workspace to show
        """
        target_type = self.view.get_current_target()
        preview_type = PreviewType.IVIEW

        if target_type == "New" or not self.displays.get_last_plot(preview_type):
            if preview_type == PreviewType.IVIEW:
                iview = self.view.open_new_iview(ws_to_show)
                self.displays.add_new_display(preview_type, iview, ws_to_show)
            elif preview_type == PreviewType.PLOT1D:
                plot = self.view.plot_1D(ws_to_show, None, False)
                plot.canvas.mpl_connect("close_event", self.displays.on_close_1D)
                self.displays.add_new_display(preview_type, plot, ws_to_show)
            elif preview_type == PreviewType.PLOT2D:
                plot = self.view.plot_2D([ws_to_show])
                plot.canvas.mpl_connect("close_event", self.displays.on_close_2D)
                self.displays.add_new_display(preview_type, plot, ws_to_show)
            elif preview_type == PreviewType.SVIEW:
                view = self.view.slice_viewer(ws_to_show)
                view.close_signal.connect(self.displays.on_close_sview)
                self.displays.add_new_display(preview_type, view, ws_to_show)

        elif target_type == "Same":
            last_window = self.displays.get_last_plot(preview_type)
            if preview_type == PreviewType.IVIEW:
                self.view.replace_old_iview(ws_to_show, last_window)
                self.displays.replace_ws_in_last(preview_type, ws_to_show)
            elif preview_type == PreviewType.PLOT1D:
                self.view.plot_1D(ws_to_show, last_window, True)
                self.displays.replace_ws_in_last(preview_type, ws_to_show)
            elif preview_type == PreviewType.PLOT2D:
                self.view.plot_2D([ws_to_show], last_window)
                self.displays.replace_ws_in_last(preview_type, ws_to_show)
            elif preview_type == PreviewType.SVIEW:
                last_ws = self.displays.get_last_workspaces(preview_type)[-1]
                last_window.presenter.rename_workspace(last_ws, ws_to_show)
                last_window.presenter.replace_workspace(last_ws, mtd[ws_to_show])
                self.displays.replace_ws_in_last(preview_type, ws_to_show)

        # overplotting is an option only for Plot1D
        elif target_type == "Over":
            last_window = self.displays.get_last_plot(preview_type)
            if preview_type == PreviewType.PLOT1D:
                self.view.plot_1D(ws_to_show, last_window, False)
                self.displays.add_ws_to_last(preview_type, ws_to_show)

        # Tiling is an option only for Plot1D and Plot2D
        elif target_type == "Tile":
            last_window = self.displays.get_last_plot(preview_type)
            if preview_type == PreviewType.PLOT1D:
                # TODO add tiling for plot1D
                pass
            if preview_type == PreviewType.PLOT2D:
                self.displays.add_ws_to_last(preview_type, ws_to_show)
                workspaces_to_show = self.displays.get_last_workspaces(preview_type)
                self.view.plot_2D(workspaces_to_show, last_window)

    def is_accumulate_checked(self):
        """
        @return whether or not the accumulate checkbox is checked
        """
        return self.view.accumulate.isChecked()


class DisplayManager:

    """
    Object managing the displays currently active.
    The displays are kept in different lists depending of their type so the last one open of each type is always readily
    available.
    """

    def __init__(self):
        super().__init__()
        self.instrument_views = []
        self.plot1D = []
        self.plot2D = []
        self.slice_viewer = []

    def get_open_displays_by_preview(self, preview_type):
        """
        @param preview_type: a preview type
        @return the list of displays currently opened for the given preview
        """
        if preview_type == PreviewType.IVIEW:
            return self.instrument_views
        if preview_type == PreviewType.PLOT1D:
            return self.plot1D
        if preview_type == PreviewType.PLOT2D:
            return self.plot2D
        if preview_type == PreviewType.SVIEW:
            return self.slice_viewer

    def get_last_display(self, preview_type):
        """
        Get the most recent display opened for this given preview
        @param preview_type : the preview type the display should be of
        @return the most recent display if there is one, else None
        """
        previews = self.get_open_displays_by_preview(preview_type)
        if previews:
            return previews[-1]
        else:
            return None

    def get_last_plot(self, preview_type):
        """
        Get the plot object currently showing the last display for the given preview.
        @param preview_type : the preview type the plot should be displaying
        @return the plot if there is a display else None
        """
        display = self.get_last_display(preview_type)
        return display.plot if display else None

    def get_last_workspaces(self, preview_type):
        """
        Get the group of workspaces currently shown in the last display.
        @return the list of workspaces if there is a display else None
        """
        display = self.get_last_display(preview_type)
        return display.ws if display else None

    def add_new_display(self, preview_type, view, ws_to_add):
        """
        Add a new display of the preview_type type in the manager.
        @param preview_type: the preview type of the target display
        @param view: the view of this display (i.e. the actual window object)
        @param ws_to_add: the workspaces currently linked to this view
        """
        self.get_open_displays_by_preview(preview_type).append(Display(view, ws_to_add))

    def add_ws_to_last(self, preview_type, ws_to_add):
        """
        Add a workspace to the list of workspaces being shown in the last opened display of preview_type type
        @param preview_type: the preview type of the display the ws is being added to
        @param ws_to_add: the workspace to add
        """
        self.get_last_display(preview_type).add_ws(ws_to_add)

    def replace_ws_in_last(self, preview_type, ws_to_replace_with):
        """
        Replace all the workspaces in the last plot by the new one
        @param preview_type: the preview type of the display the ws should be shown on.
        @param ws_to_replace_with: the workspace with which to replace the old ones.
        """
        self.get_last_display(preview_type).replace_ws_by(ws_to_replace_with)

    def on_close_1D(self, event):
        """
        Slot triggered when a 1D plot is closed.
        @param event: the matplotlib event corresponding to the close
        """
        for index, display in enumerate(self.plot1D):
            if display.plot.canvas == event.canvas:
                self.plot1D.pop(index)
                # TODO clean all the involved ws, depending on the final caching policy
                return

    def on_close_2D(self, event):
        """
        Slot triggered when a 2D plot is closed
        @param event: the matplotlib event corresponding to the close
        """
        for index, display in enumerate(self.plot2D):
            if display.plot.canvas == event.canvas:
                self.plot2D.pop(index)
                # TODO clean all the involved ws, depending on the final caching policy
                return

    def on_close_sview(self):
        # TODO find a way to catch the slice viewer close signal, and find a way to get the name/object that was closed
        pass

    def on_close_iview(self):
        # TODO carry the close signal for the instrument viewer from C++ and find the name of the one that was closed
        pass


class Display:
    """
    A Display is a convenience object holding a plot of any type - Plot1D, Plot2D, Iview, Slice Viewer - and the list
    of the names of the workspaces currently being shown on this plot.
    """

    def __init__(self, plot, workspace):
        self.plot = plot
        self.ws = [workspace]

    def get_last_workspace(self):
        """
        @return the last workspace to have been added to the display, or None if there are none.
        """
        return self.ws[-1] if self.ws else None

    def pop_last_ws(self):
        """
        Pop the last workspace associated to this display
        @return the last workspace to have been added to the display, or None if there are none.
        """
        last_ws = self.ws.pop() if self.ws else None
        return last_ws

    def add_ws(self, ws_to_add):
        """
        Add a workspace to the list of workspaces
        @param ws_to_add: the workspace to add,as a string
        """
        self.ws.append(ws_to_add)

    def replace_ws_by(self, ws_to_replace_with):
        """
        Replace all of the currently linked workspaces by a new one
        @param ws_to_replace_with: the workspace with which to replace the old ones.
        """
        self.ws.clear()
        self.ws.append(ws_to_replace_with)
