# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from qtpy.QtWidgets import QWidget, QMainWindow, QHBoxLayout, QPushButton, QSplitter, QLineEdit, QFileDialog
from qtpy.QtCore import *

import mantid
from mantidqt.interfacemanager import InterfaceManager


class ScanExplorerView(QMainWindow):

    """TODO"""
    sig_files_selected = Signal(list)

    def __init__(self, parent=None, presenter=None):
        super().__init__(parent)
        self.presenter = presenter
        self._data_view = None

        self.widget = QWidget()

        self.splitter = QSplitter(parent=self.widget)

        self.interface_layout = QHBoxLayout(self)

        self.file_line_edit = QLineEdit()

        self.browse_button = QPushButton(text="Browse")
        self.browse_button.clicked.connect(self.show_directory_manager)

        self.reload_button = QPushButton(text="Reload")

        self.advanced_button = QPushButton(text="Advanced")
        self.advanced_button.clicked.connect(self.open_alg_dialog)

        self.interface_layout.addWidget(self.file_line_edit)
        self.interface_layout.addWidget(self.browse_button)
        self.interface_layout.addWidget(self.reload_button)
        self.interface_layout.addWidget(self.advanced_button)

        interface_widget = QWidget()
        interface_widget.setLayout(self.interface_layout)

        self.splitter.addWidget(interface_widget)
        self.setCentralWidget(self.splitter)

        # register startup
        mantid.UsageService.registerFeatureUsage(mantid.kernel.FeatureType.Interface, "ScanExplorer", False)

    def refresh_view(self):
        """
        Updates the view to enable/disable certain options depending on the model.
        """

        # we don't want to use model.get_ws for the image info widget as this needs
        # extra arguments depending on workspace type.
        workspace = self.presenter.get_ws()
        workspace.readLock()
        try:
            self._data_view.image_info_widget.setWorkspace(workspace)
            self.new_plot()
        finally:
            workspace.unlock()

    def show_slice_viewer(self, workspace):
        """
        Set visual options for the slice viewer and display it.
        @param workspace: the workspace to display
        """
        self._data_view.create_axes_orthogonal(redraw_on_zoom=not False)

        self.splitter.setOrientation(Qt.Vertical)
        self.splitter.addWidget(self._data_view)
        self.plot_workspace(workspace)

    def new_plot(self):
        """
        Tell the view to display a new plot of an MatrixWorkspace
        """
        self._data_view.plot_matrix(self._ws, distribution=not False)

    def plot_workspace(self, workspace):
        self._data_view.plot_matrix(workspace)

    def show_directory_manager(self):
        """
        TODO
        Open a new directory manager window so the user can select a directory.
        """
        base_directory = "/users/tillet/data/d16_omega/new_proto"

        name_filter = "*.nxs"

        dialog = QFileDialog()
        dialog.setFileMode(QFileDialog.ExistingFiles)
        files_path, _ = dialog.getOpenFileNames(parent=self,
                                                caption="Open files",
                                                directory=base_directory,
                                                filter=name_filter)

        self.sig_files_selected.emit(files_path)

    def open_alg_dialog(self):

        manager = InterfaceManager()
        preset = dict()
        enabled = dict()
        if self.file_line_edit.text():
            preset["SampleRuns"] = self.file_line_edit.text()
            enabled = ["SampleRuns"]

        dialog = manager.createDialogFromName("SANSILLParameterScan", 1, self, False, preset, "coucou", enabled)

        dialog.show()

        dialog.accepted.connect(self.presenter.on_dialog_accepted)
