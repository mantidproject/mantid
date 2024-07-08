# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from qtpy.QtWidgets import (
    QMainWindow,
    QHBoxLayout,
    QVBoxLayout,
    QPushButton,
    QSplitter,
    QLineEdit,
    QFileDialog,
    QGroupBox,
    QToolButton,
    QStatusBar,
    QLabel,
)
from qtpy.QtCore import Signal, Qt
from qtpy.QtGui import QCloseEvent

from mantid.api import MatrixWorkspace
from mantid.kernel import FeatureType
from mantid import UsageService
from mantidqt.interfacemanager import InterfaceManager
from mantidqt.widgets.sliceviewer.views.toolbar import ToolItemText
from mantidqt.widgets.sliceviewer.views.dataview import SliceViewerDataView
from mantidqt.widgets.sliceviewer.presenters.lineplots import PixelLinePlot

from .rectangle_plot import MultipleRectangleSelectionLinePlot
from .rectangle_controller import RectanglesManager


class SimpleScanViewerView(QMainWindow):
    """Index of the slice viewer widget in the splitter. Used to replace it when needed."""

    SLICE_VIEWER_SPLITTER_INDEX = 0

    """Allowed file extensions"""
    FILE_EXTENSION_FILTER = "*.nxs"

    """Signal sent when a file is selected to be loaded in the file dialog."""
    sig_file_selected = Signal(str)

    """Signal sent when a file is selected as a background in the dialog."""
    sig_background_selected = Signal(str)

    def __init__(self, parent=None, presenter=None):
        super().__init__(parent)
        self.presenter = presenter

        self._data_view = None  # data_view object, lifted directly from the slice viewer.
        # Needs data to be created, so it is added at runtime

        self._rectangles_manager = None  # the object managing the multiple rectangles on the plot

        # splitter managing the entire window
        self.splitter = QSplitter(orientation=Qt.Vertical, parent=self)
        self.setCentralWidget(self.splitter)

        self.lower_splitter = QSplitter()  # splitter for the lower part of the widget.
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

        # part not visible at start up : data handling
        # button to clear the plot and reset it
        self.clear_button = QPushButton(text="Clear")
        self.clear_button.setVisible(False)

        # button to go in navigation mode
        self.move_button = QPushButton(text="Move")
        self.move_button.setVisible(False)
        self.move_button.setCheckable(True)

        # button to go in zoom mode
        self.zoom_button = QPushButton(text="Zoom")
        self.zoom_button.setVisible(False)
        self.zoom_button.setCheckable(True)

        # button to go in rectangle drawing mode
        self.multiple_button = QPushButton(text="ROI")
        self.multiple_button.setVisible(False)
        self.multiple_button.setCheckable(True)

        # button to set/replace the background workspace
        self.background_button = QPushButton(text="Set background")
        self.background_button.setVisible(True)
        self.background_button.setCheckable(False)
        self.background_button.clicked.connect(self.on_set_background_clicked)

        # status bar with the help
        self.status_bar = QStatusBar(parent=self.splitter)
        self.status_bar.setStyleSheet("QStatusBar::item {border: None;}")  # Hide spacers between button and label
        self.status_bar_label = QLabel()
        self.help_button = QToolButton()
        self.help_button.setText("?")
        self.status_bar.addWidget(self.help_button)
        self.status_bar.addWidget(self.status_bar_label)

        # setting the layout
        upper_layout = QVBoxLayout(self.splitter)

        layout = QHBoxLayout()
        layout.addWidget(self.file_line_edit)
        layout.addWidget(self.browse_button)
        layout.addWidget(self.advanced_button)
        layout.addWidget(self.background_button)

        upper_layout.addLayout(layout)

        layout_2 = QHBoxLayout()

        layout_2.addWidget(self.clear_button)
        layout_2.addWidget(self.move_button)
        layout_2.addWidget(self.zoom_button)
        layout_2.addWidget(self.multiple_button)

        upper_layout.addLayout(layout_2)

        bar_widget = QGroupBox()
        bar_widget.setLayout(upper_layout)

        self.splitter.addWidget(bar_widget)
        self.splitter.addWidget(self.status_bar)

        # register startup
        UsageService.registerFeatureUsage(FeatureType.Interface, "SimpleScanViewer", False)

    def manage_buttons(self):
        """
        Handle data view manipulation buttons. Remove the slice viewer default control bar and show the custom one.
        """
        self.data_view.mpl_toolbar.setVisible(False)
        self.data_view.image_info_widget.setVisible(False)
        self.data_view.track_cursor.setVisible(False)
        self.data_view.dimensions.setVisible(False)

        self._data_view._region_selection_on = True  # TODO stop setting private parameter without asking nicely

        self.clear_button.clicked.connect(self.on_clear_clicked)
        self.move_button.clicked.connect(self.on_move_clicked)
        self.zoom_button.clicked.connect(self.on_zoom_clicked)
        self.multiple_button.clicked.connect(self.on_multiple_clicked)

        self.clear_button.setVisible(True)
        self.move_button.setVisible(True)
        self.zoom_button.setVisible(True)
        self.multiple_button.setVisible(True)

        self.multiple_button.setChecked(True)
        self.on_multiple_clicked(True)

    def on_clear_clicked(self, _):
        """
        Slot called when the clear button is clicked
        """
        self._data_view.mpl_toolbar.set_action_checked(ToolItemText.HOME, True, True)
        self._rectangles_manager.clear()

    def on_zoom_clicked(self, state: bool):
        """
        Slot called when the zoom button is clicked. Reset the rectangle manager and set mode to zoom.
        @param state: the new state of the button
        """
        if state:
            self.move_button.setChecked(False)
            self.multiple_button.setChecked(False)
            self._data_view.mpl_toolbar.set_action_checked(ToolItemText.ZOOM, True, True)
            self._data_view.mpl_toolbar.set_action_checked(ToolItemText.HOME, False, False)
            self._data_view.mpl_toolbar.set_action_checked(ToolItemText.PAN, False, False)
            self._data_view.mpl_toolbar.set_action_checked(ToolItemText.REGIONSELECTION, False, False)
        else:
            self.zoom_button.setChecked(True)

    def on_move_clicked(self, state: bool):
        """
        Slot called when the move button is clicked. Reset the rectangle manager and set mode to move.
        @param state: the new state of the button
        """
        if state:
            self.multiple_button.setChecked(False)
            self.zoom_button.setChecked(False)

            self._data_view.mpl_toolbar.set_action_checked(ToolItemText.PAN, True, True)
            self._data_view.mpl_toolbar.set_action_checked(ToolItemText.HOME, False, True)
            self._data_view.mpl_toolbar.set_action_checked(ToolItemText.ZOOM, False, True)
            self._data_view.mpl_toolbar.set_action_checked(ToolItemText.REGIONSELECTION, False, True)
        else:
            self.move_button.setChecked(True)

    def on_multiple_clicked(self, state: bool):
        """
        Slot called when the rectangles button is clicked. Change to multiple rectangle mode.
        @param state: the new state of the button
        """
        if state:
            self.move_button.setChecked(False)
            self.zoom_button.setChecked(False)

            self._data_view.mpl_toolbar.set_action_checked(ToolItemText.HOME, state=False, trigger=True)
            self._data_view.mpl_toolbar.set_action_checked(ToolItemText.ZOOM, state=False, trigger=True)
            self._data_view.mpl_toolbar.set_action_checked(ToolItemText.PAN, state=False, trigger=True)
        else:
            self.multiple_button.setChecked(True)

    def initialize_rectangle_manager(self):
        """
        Initialize or reset the rectangle manager setting.
        """
        self.data_view.add_line_plots(PixelLinePlot, self.data_view.presenter)

        self._rectangles_manager = RectanglesManager(self)
        self._rectangles_manager.additional_peaks_info = self.presenter.additional_peaks_info

        tool = MultipleRectangleSelectionLinePlot
        self._data_view.switch_line_plots_tool(tool, self.presenter)

        if self.lower_splitter.count() > 1:
            self.lower_splitter.replaceWidget(1, self._rectangles_manager)
        else:
            self.lower_splitter.addWidget(self._rectangles_manager)

    def on_set_background_clicked(self):
        """
        Slot triggered by clicking on set/replace background workspace. Open a window to select the workspace.
        """
        file_path = self._open_file_dialog()
        if file_path:
            self.sig_background_selected.emit(file_path)

    def show_slice_viewer(self, workspace: MatrixWorkspace):
        """
        Set visual options for the slice viewer and display it.
        @param workspace: the workspace to display
        """
        self.status_bar.setVisible(False)
        if self.lower_splitter.count() == 0:
            self.lower_splitter.addWidget(self._data_view)

            self.splitter.insertWidget(1, self.lower_splitter)
        else:
            self.lower_splitter.replaceWidget(self.SLICE_VIEWER_SPLITTER_INDEX, self._data_view)

        # forbid collapsing anything with the slice viewer in it
        self.lower_splitter.setCollapsible(0, False)
        self.splitter.setCollapsible(1, False)

        self._data_view.plot_matrix(workspace)

    def show_directory_manager(self):
        """
        Open a new directory manager window so the user can select files.
        """
        file_path = self._open_file_dialog()
        if file_path:
            self.sig_file_selected.emit(file_path)

    def _open_file_dialog(self) -> str:
        """
        Open a file dialog.
        @return the file selected by the user.
        """
        base_directory = self.presenter.get_base_directory()

        dialog = QFileDialog()
        dialog.setFileMode(QFileDialog.ExistingFiles)
        file_path, _ = dialog.getOpenFileName(parent=self, caption="Open file", directory=base_directory, filter=self.FILE_EXTENSION_FILTER)

        return file_path

    def open_alg_dialog(self):
        """
        Open the dialog for SANSILLParameterScan and setup connections.
        """
        manager = InterfaceManager()
        preset = dict()
        enabled = list()
        if self.file_line_edit.text():
            preset["SampleRuns"] = self.file_line_edit.text()
            enabled = ["SampleRuns"]

        dialog = manager.createDialogFromName("SANSILLParameterScan", 1, self, False, preset, "", enabled)

        dialog.show()

        dialog.accepted.connect(self.presenter.on_dialog_accepted)

    def closeEvent(self, event: QCloseEvent):
        """
        Triggered when the window is closed. Cleans help-related details.
        """
        # shutdown help window process
        self.presenter.assistant_process.close()
        self.presenter.assistant_process.waitForFinished()

        if self.data_view:
            # disconnect the help button so a call to help in the slice viewer won't invoke the incorrect one
            self.data_view.help_button.clicked.disconnect()

        super().closeEvent(event)

    @property
    def data_view(self) -> SliceViewerDataView:
        return self._data_view

    @data_view.setter
    def data_view(self, new_data_view: SliceViewerDataView):
        self._data_view = new_data_view

    @property
    def rectangles_manager(self) -> RectanglesManager:
        return self._rectangles_manager

    @rectangles_manager.setter
    def rectangles_manager(self, manager: RectanglesManager):
        self._rectangles_manager = manager
