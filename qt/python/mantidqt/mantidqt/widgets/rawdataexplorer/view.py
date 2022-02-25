# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from matplotlib import pyplot as plt
from qtpy.QtWidgets import QFileDialog, QWidget, QMessageBox
from qtpy.QtCore import *

from mantid.simpleapi import *
from mantid.kernel import logger
from mantidqt.utils.qt import load_ui
from mantidqt.widgets.instrumentview.api import *
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
        try:
            self.get_widget(workspace_name)
        except ValueError as e:
            message = "Could not open view:\n{0}".format(e)
            logger.error(message)
            message_box = QMessageBox()
            message_box.setText(message)
            message_box.exec()
            self.on_close()
            return

        if self._type == self.IVIEW:
            self._widget.show_view()
            self._widget.container.closing.connect(self.on_close)
            self.sig_request_close.connect(self._widget.container.emit_close)
        if self._type == self.SVIEW:
            self._widget.view.show()
            self._widget.view.close_signal.connect(self.on_close)
            self.sig_request_close.connect(self._widget.view.emit_close)
        if self._type == self.PLOT2D:
            self._widget.canvas.mpl_connect("close_event", self.on_close)
            self.sig_request_close.connect(lambda: plt.close(self._widget))

        if self._type == self.PLOT1D:
            self._widget.canvas.mpl_connect("close_event", self.on_close)
            self.sig_request_close.connect(lambda: plt.close(self._widget))

        if self._type == self.PLOTSPECTRUM:
            self._widget.canvas.mpl_connect("close_event", self.on_close)
            self.sig_request_close.connect(lambda: plt.close(self._widget))

        self._presenter.get_main_view().fileTree.set_ignore_next_focus_out(True)

    def get_widget(self, workspace_name):
        """
        Create the appropriate widget to show.
        @param workspace_name: name of the workspace
        """
        if self._type == self.IVIEW:
            parent, flags = get_window_config()
            self._widget = get_instrumentview(workspace_name, True, parent, flags, use_thread=False)
        elif self._type == self.SVIEW:
            self._widget = SliceViewer(ws=mtd[workspace_name])
        elif self._type == self.PLOT1D:
            self._widget = plotBin(workspace_name, 0, error_bars=True)
        elif self._type == self.PLOT2D:
            self._widget = pcolormesh([workspace_name])
        elif self._widget == self.PLOTSPECTRUM:
            self._widget = plotSpectrum(workspace_name, 0, error_bars=True)

    def change_workspace(self, workspace_name):
        """
        Change the workspace displayed by the current widget.
        @param workspace_name (str): name of the new workspace
        """
        if self._type == self.IVIEW:
            self._widget.replace_workspace(workspace_name)
        elif self._type == self.SVIEW:
            return
        elif self._type == self.PLOT2D:
            pcolormesh([workspace_name], self._widget)
            self._presenter.get_main_view().fileTree.set_ignore_next_focus_out(True)
        elif self._type == self.PLOT1D:
            plotBin(workspace_name, 0, error_bars=True, window=self._widget,
                    clearWindow=True)
            self._presenter.get_main_view().fileTree.set_ignore_next_focus_out(True)
        elif self._type == self.PLOTSPECTRUM:
            plotSpectrum(workspace_name, 0, error_bars=True, window=self._widget, clearWindow=True)
            self._presenter.get_main_view().fileTree.set_ignore_next_focus_out(True)

    def on_close(self, name=None):
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

    file_tree_path_changed = Signal(str)

    def __init__(self, presenter, parent=None):
        super().__init__(parent)

        self.ui = load_ui(__file__, 'rawdataexplorer.ui', baseinstance=self)

        self._presenter = presenter

        self._current_selection = set()

        self._previews = list()

        self.setAttribute(Qt.WA_DeleteOnClose, True)
        self.setup_connections()

        self._last_clicked = None  # full path of the last model item clicked. It can be a directory or a file.

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
        if preview in self._previews:
            self._previews.remove(preview)

    def clear_selection(self):
        """
        Clear all items currently selected
        """
        self._current_selection = set()
        self.fileTree.clear_selection()

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

    def on_item_selected(self, last_selected_index):
        """
        Triggered when an item is selected in the tree widget.
        This method checks the selected items and sends a signal if they changed.
        @param last_selected_index: the index of the last selected item
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

            if index == last_selected_index:
                self._last_clicked = file_model.filePath(last_selected_index)
            selection.add(file_path)

        if selection != self._current_selection:
            self._current_selection = selection
            self._presenter.on_selection_changed()

    def setup_connections(self):
        """
        Set up connections between signals and slots in the view.
        """
        self.browse.clicked.connect(self.show_directory_manager)
        self.fileTree.sig_new_current.connect(self.on_item_selected, Qt.QueuedConnection)

    def is_accumulating(self):
        return self._presenter.is_accumulating()

    def show_directory_manager(self):
        """
        Open a new directory manager window so the user can select a directory.
        """
        base_directory = self.fileTree.model().rootPath()

        # we have to use the DontUseNativeDialog flag because without it, the ShowDirsOnly flag is ignored on Linux
        dialog = QFileDialog()
        file_tree_path = dialog.getExistingDirectory(parent=self,
                                                     caption="Select a directory",
                                                     directory=base_directory,
                                                     options=QFileDialog.DontUseNativeDialog | QFileDialog.ShowDirsOnly)

        self.file_tree_path_changed.emit(file_tree_path)
