# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#

from qtpy.QtWidgets import QFileDialog, QWidget
from qtpy.QtCore import *
from matplotlib import pyplot as plt

from mantidqt.utils.qt import load_ui
from mantidqt.widgets.instrumentview.api import *
from mantid.simpleapi import *
from mantidqt.widgets.sliceviewer.presenter import SliceViewer
from mantidqt.plotting.functions import pcolormesh
from workbench.config import get_window_config


class PreviewView(QObject):

    """
    Type of the preview when the instrument viewer widget is used to show the
    data.
    """
    IVIEW = "instrument_viewer"

    """
    Type of the preview when the slice viewer widget is used to show the data.
    """
    SVIEW = "slice_viewer"

    """
    Type of the preview when the 2d plot is used to show the data.
    """
    PLOT2D = "plot_2d"

    """
    Type of the preview when the 1d plot is used to show the data.
    """
    PLOT1D = "plot_1d"

    """
    Type of the preview when the spectrum plot is used to show the data.
    """
    PLOTSPECTRUM = "plot_spectrum"

    """
    Type the preview.
    """
    _type = None

    """
    Presenter.
    """
    _presenter = None

    """
    Reference to the visualisation widget.
    """
    _widget = None

    """
    Signal to request closing the preview
    """
    sig_request_close = Signal()

    def __init__(self):
        super().__init__()

    def set_type(self, preview_type):
        """
        Set the preview type.

        Args:
            preview_type (str): preview type
        """
        self._type = preview_type

    def set_presenter(self, presenter):
        """
        Set the presenter.

        Args:
            presenter (PreviewPresenter): presenter
        """
        self._presenter = presenter

    def show_workspace(self, workspace_name):
        """
        Show the workspace on the adapted widget.
        @param workspace_name (str): name of the workspace
        """
        if self._type == self.IVIEW:
            self._widget = get_instrumentview(workspace_name, get_window_config()[1])
            self._widget.show_view()
            self._widget.container.closing.connect(self.on_close)
            self.sig_request_close.connect(self._widget.container.emit_close)
        if self._type == self.SVIEW:
            self._widget = SliceViewer(ws=mtd[workspace_name])
            self._widget.view.show()
            self._widget.view.close_signal.connect(self.on_close)
            self.sig_request_close.connect(self._widget.view.emit_close)
        if self._type == self.PLOT2D:
            self._widget = pcolormesh([workspace_name])
            self._widget.canvas.mpl_connect("close_event", self.on_close)
            self.sig_request_close.connect(lambda: plt.close(self._widget))

        if self._type == self.PLOT1D:
            self._widget = plotBin(workspace_name, 0, error_bars=True)
            self._widget.canvas.mpl_connect("close_event", self.on_close)
            self.sig_request_close.connect(lambda: plt.close(self._widget))

        if self._type == self.PLOTSPECTRUM:
            self._widget = plotSpectrum(workspace_name, 0, error_bars=True)
            self._widget.canvas.mpl_connect("close_event", self.on_close)
            self.sig_request_close.connect(lambda: plt.close(self._widget))

    def change_workspace(self, workspace_name):
        """
        Change the workspace displayed by the current widget.
        @param workspace_name (str): name of the new workspace
        """
        if self._type == self.IVIEW:
            self._widget.replace_workspace(workspace_name)
        if self._type == self.SVIEW:
            return
        if self._type == self.PLOT2D:
            pcolormesh([workspace_name], self._widget)
        if self._type == self.PLOT1D:
            plotBin(workspace_name, 0, error_bars=True, window=self._widget,
                    clearWindow=True)
        if self._type == self.PLOTSPECTRUM:
            plotSpectrum(workspace_name, 0, error_bars=True, window=self._widget, clearWindow=True)

    def on_close(self, event=None):
        """
        Triggered when the widget is closed.
        """
        self._presenter.close_preview()


class RawDataExplorerView(QWidget):
    """
    The view for the raw data explorer widget.
    """

    """
    Presenter.
    """
    _presenter = None

    """
    List of selected files in the tree widget.
    """
    _current_selection = None

    """
    List of preview views.
    """
    _previews = None

    """
    Full path of the last model item clicked. It can be a directory or a file.
    """
    _last_clicked = None

    file_tree_path_changed = Signal(str)

    def __init__(self, presenter, parent=None):
        super().__init__(parent)

        self.ui = load_ui(__file__, 'rawdataexplorer.ui', baseinstance=self)

        self._presenter = presenter

        self._current_selection = set()

        self._previews = list()

        self.setAttribute(Qt.WA_DeleteOnClose, True)
        self.setup_connections()

        self.fileTree.sig_new_current.connect(self.on_file_clicked, Qt.QueuedConnection)

        self.is_busy = False

    def closeEvent(self, event):
        self.deleteLater()
        super(RawDataExplorerView, self).closeEvent(event)

    def add_preview(self):
        """
        Add a new preview.

        Returns:
            PreviewView: new preview
        """
        preview = PreviewView()
        self._previews.append(preview)
        return preview

    def del_preview(self, preview):
        """
        Remove a preview.

        Args:
            preview (PreviewView): preview to be removed
        """
        self._previews.remove(preview)

    def clear_selection(self):
        """
        Clear all items currently selected
        """
        selection_model = self.fileTree.selectionModel()
        for index in selection_model.selectedRows():
            selection_model.select(index, QItemSelectionModel.Deselect | QItemSelectionModel.Rows)
        self._current_selection = set()

    def get_last_clicked(self):
        """
        Get last file clicked

        @return the full path to the last file clicked by the user
        """
        return self._last_clicked

    def get_selection(self):
        """
        Get the selected files. The set contains the full path of the files and
        the files match the tree widget name filter.
        @return (set(str)): selected filenames
        """
        return self._current_selection

    def on_file_clicked(self, last_clicked_index):
        """
        Triggered when a file is clicked in the tree widget. This method check
        the selected items and sends a signal if it changed.
        """
        selection_model = self.fileTree.selectionModel()
        file_model = self.fileTree.model()

        selected_indexes = selection_model.selectedRows()
        selection = set()

        for index in selected_indexes:

            file_path = file_model.filePath(index)
            if file_model.isDir(index):
                # we don't select directories
                selection_model.select(index, QItemSelectionModel.Deselect | QItemSelectionModel.Rows)
                continue

            if index == last_clicked_index:
                self._last_clicked = file_model.filePath(last_clicked_index)
            selection.add(file_path)

        if selection != self._current_selection:
            self._current_selection = selection
            self._presenter.on_selection_changed()

    def setup_connections(self):
        """
        Set up connections between signals and slots in the view.
        """
        self.browse.clicked.connect(self.show_directory_manager)

    def show_directory_manager(self):
        """
        Open a new directory manager window so the user can select a directory.
        """
        # we have to use the DontUseNativeDialog flag because without it, the ShowDirsOnly flag is ignored on Linux
        file_tree_path = QFileDialog().getExistingDirectory(parent=self,
                                                            caption="Select a directory",
                                                            directory="/home",
                                                            options=QFileDialog.DontUseNativeDialog | QFileDialog.ShowDirsOnly)
        self.file_tree_path_changed.emit(file_tree_path)

    # Views openers and managers
    @staticmethod
    def open_new_iview(ws_to_show):
        """
        Open a new instrument view with the selected preview characteristics.
        Correspond to the "New" target.
        @param ws_to_show: the name of the workspace to display
        @return the instrument view object, to be kept by the presenter
        """
        iview = get_instrumentview(ws_to_show)
        iview.show_view()
        return iview

    @staticmethod
    def replace_old_iview(ws_to_show, window):
        """
        Replace the workspace shown in the last created instrument view, if there is one, or create a new one.
        Correspond to the "Same" target.
        @param ws_to_show: the name of the workspace to display
        @param window: TODO
        """
        window.replace_workspace(ws_to_show)

    @staticmethod
    def plot_1D(ws_to_show, window=None, clear_window=True):
        """
        Plot the data contained in the ws_to_show workspace as a 1D bin plot.
        If no window is provided, one is opened and returned.
        @param ws_to_show: the name of the workspace whose first bin should be shown
        @param window: the window object on which to display the plot. If None, one will be created
        @param clear_window: whether or not to clear the window of any previous display before the new data is shown
        @return the plot object holding the display
        """
        return plotBin(ws_to_show, 0, error_bars=True, window=window, clearWindow=clear_window)

    @staticmethod
    def plot_2D(ws_to_show, window=None):
        """
        Plot the data contained in all workspaces in ws_to_show as colorfills.
        @param ws_to_show: the workspaces to show, as a list of workspaces
        @param window: the window object on which to display the plot. If None, one will be created
        @return the plot object holding the display
        """
        return pcolormesh(ws_to_show, window)

    @staticmethod
    def slice_viewer(ws_to_show, view=None):
        """
        Open the slice viewer and
        """
        if not view:
            presenter = SliceViewer(ws=mtd[ws_to_show])
            presenter.view.show()
            return presenter.view
