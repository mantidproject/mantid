# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from qtpy.QtWidgets import QMainWindow, QHBoxLayout, QPushButton, QSplitter, QLineEdit, QFileDialog, QGroupBox
from qtpy.QtCore import *

import mantid
from mantidqt.interfacemanager import InterfaceManager
from mantidqt.widgets.sliceviewer.views.toolbar import ToolItemText

from .rectangle_plot import MultipleRectangleSelectionLinePlot
from .rectangle_controller import RectanglesManager


class ScanExplorerView(QMainWindow):

    """Index of the slice viewer widget in the splitter. Used to replace it when needed."""
    SLICE_VIEWER_SPLITTER_INDEX = 0

    """Allowed file extensions"""
    FILE_EXTENSION_FILTER = "*.nxs"

    """Signal sent when files are selected in the file dialog."""
    sig_files_selected = Signal(str)

    def __init__(self, parent=None, presenter=None):
        super().__init__(parent)
        self.presenter = presenter

        self._data_view = None  # data_view object, lifted directly from the slice viewer.
        # Needs data to be created, so it is added at runtime

        self._rectangles_manager = None  # the object managing the multiple rectangles on the plot

        # splitter managing the entire window
        self.splitter = QSplitter(orientation=Qt.Vertical, parent=self)
        self.setCentralWidget(self.splitter)

        self.lower_splitter = None  # splitter for the lower part of the widget.
        # Holds the data view and the rectangle table, as they are created. At startup, not shown.

        # at start up, only the upper part: selecting data
        # line edit to write path to file to open
        self.file_line_edit = QLineEdit()

        # button to open a browser to select file to open
        self.browse_button = QPushButton(text="Browse")
        self.browse_button.clicked.connect(self.show_directory_manager)

        # button to open the algorithm dialog for finer control
        self.advanced_button = QPushButton(text="Advanced", toolTip="Open SANSILLParameterScan dialog")
        self.advanced_button.clicked.connect(self.open_alg_dialog)

        # setting the layout
        upper_layout = QHBoxLayout(self)
        upper_layout.addWidget(self.file_line_edit)
        upper_layout.addWidget(self.browse_button)
        upper_layout.addWidget(self.advanced_button)

        bar_widget = QGroupBox()
        bar_widget.setLayout(upper_layout)

        self.splitter.addWidget(bar_widget)

        # register startup
        mantid.UsageService.registerFeatureUsage(mantid.kernel.FeatureType.Interface, "ScanExplorer", False)

    def start_multiple_rect_mode(self):
        """
        Change to multiple rectangle mode.
        """
        self._rectangles_manager = RectanglesManager(self)

        if self.lower_splitter.count() == 1:
            self.lower_splitter.addWidget(self._rectangles_manager)
        else:
            self.lower_splitter.replaceWidget(1, self._rectangles_manager)

        tool = MultipleRectangleSelectionLinePlot
        self._data_view.mpl_toolbar.set_action_checked(ToolItemText.REGIONSELECTION, state=True, trigger=True)
        self._data_view.switch_line_plots_tool(tool, self.presenter)

        # TODO stop calling private attributes
        self._data_view.mpl_toolbar.set_action_checked("TEST", state=True, trigger=False)
        self._data_view.mpl_toolbar.homeClicked.connect(self._data_view._line_plots.clear)

    def refresh_view(self):
        """
        Updates the view to enable/disable certain options depending on the model.
        """

        # we don't want to use model.get_ws for the image info widget as this needs
        # extra arguments depending on workspace type.
        workspace = self.presenter.ws()
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
        if self.splitter.count() == 1:
            self.lower_splitter = QSplitter()
            self.lower_splitter.addWidget(self._data_view)

            self.splitter.addWidget(self.lower_splitter)
        else:
            self.lower_splitter.replaceWidget(self.SLICE_VIEWER_SPLITTER_INDEX, self._data_view)

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
        Open a new directory manager window so the user can select files.
        """
        base_directory = "/users/tillet/data/d16_omega/new_proto"

        dialog = QFileDialog()
        dialog.setFileMode(QFileDialog.ExistingFiles)
        file_path, _ = dialog.getOpenFileName(parent=self,
                                              caption="Open file",
                                              directory=base_directory,
                                              filter=self.FILE_EXTENSION_FILTER)
        if file_path:
            self.sig_files_selected.emit(file_path)

    def open_alg_dialog(self):
        """
        Open the dialog for SANSILLParameterScan and setup connections.
        """
        manager = InterfaceManager()
        preset = dict()
        enabled = dict()
        if self.file_line_edit.text():
            preset["SampleRuns"] = self.file_line_edit.text()
            enabled = ["SampleRuns"]

        dialog = manager.createDialogFromName("SANSILLParameterScan", 1, self, False, preset, "", enabled)

        dialog.show()

        dialog.accepted.connect(self.presenter.on_dialog_accepted)

    @property
    def data_view(self):
        return self._data_view

    @data_view.setter
    def data_view(self, new_data_view):
        self._data_view = new_data_view

    @property
    def rectangles_manager(self):
        return self._rectangles_manager
