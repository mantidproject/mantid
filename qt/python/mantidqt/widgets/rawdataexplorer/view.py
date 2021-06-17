# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#

from qtpy.QtWidgets import QFileDialog, QWidget, QHeaderView, QFileSystemModel
from qtpy.QtCore import *

from mantidqt.utils.qt import load_ui
from mantidqt.widgets.instrumentview.api import *
from mantid.simpleapi import *
from mantidqt.widgets.sliceviewer.presenter import SliceViewer
from mantidqt.plotting.functions import pcolormesh


class RawDataExplorerView(QWidget):
    """
    The view for the raw data explorer widget.
    """

    """
    List of filters for the file system tree widget.
    """
    _FILE_SYSTEM_FILTERS = ["*.nxs"]

    file_tree_path_changed = Signal(str)

    def __init__(self, presenter, parent=None):
        super().__init__(parent)

        self.ui = load_ui(__file__, 'rawdataexplorer.ui', baseinstance=self)

        self.presenter = presenter

        self.setAttribute(Qt.WA_DeleteOnClose, True)
        self.setup_connections()

        # tree widget setup
        file_model = QFileSystemModel()
        file_model.setNameFilters(self._FILE_SYSTEM_FILTERS)
        file_model.setNameFilterDisables(0)
        file_model.setRootPath("/")
        self.fileTree.setModel(file_model)
        self.fileTree.header().hideSection(2)
        self.fileTree.header().setSectionResizeMode(0, QHeaderView.Stretch)
        self.fileTree.header().setSectionResizeMode(1, QHeaderView.Fixed)
        self.fileTree.header().setSectionResizeMode(2, QHeaderView.Fixed)
        self.fileTree.header().setSectionResizeMode(3, QHeaderView.Fixed)

    def closeEvent(self, event):
        self.deleteLater()
        super(RawDataExplorerView, self).closeEvent(event)

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
                                                            options=QFileDialog.DontUseNativeDialog |
                                                                    QFileDialog.ShowDirsOnly)
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

    # Combobox getters
    def get_current_preview(self):
        """
        @return the name of the currently selected preview type, as a string.
        """
        return str(self.previewType.currentText())

    def get_current_target(self):
        """
        Get the current target's name.
        @return the currently selected target, as a string
        """
        return str(self.targetType.currentText())

    def get_current_acquisition(self):
        """
        Get the current acquisition's name.
        @return the currently selected acquisition, as a string
        """
        return str(self.acquisitionType.currentText())

    # Combobox option setters
    def populate_targets(self, targets):
        """
        Write the new possible targets in the target combobox.
        @param targets: the list of the possible targets, as strings.
        """
        self.targetType.clear()
        for target in targets:
            self.targetType.addItem(target)

    def set_target(self, target):
        """
        Set the currently selected target to target. Does nothing if it not a valid option.
        @param target: the target to select, as string.
        """
        index = self.targetType.findText(target)
        if index != -1:
            self.targetType.setCurrentIndex(index)

    def populate_previews(self, previews):
        """
        Write the new possible previews in the preview combobox.
        @param previews: the list of the possible previews, as strings.
        """
        self.previewType.blockSignals(True)
        self.previewType.clear()
        for preview in previews:
            self.previewType.addItem(preview)

        self.previewType.blockSignals(False)

    def populate_acquisitions(self, acquisitions):
        """
        Write the new possible acquisitions in the acquisition combobox.
        @param acquisitions: the list of the possible acquisitions, as strings.
        """
        self.acquisitionType.blockSignals(True)
        self.acquisitionType.clear()
        for acq in acquisitions:
            self.acquisitionType.addItem(acq)
        self.acquisitionType.blockSignals(False)
