# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from functools import partial
from pyvista import PolyData
from qtpy.QtWidgets import (
    QMainWindow,
    QVBoxLayout,
    QHBoxLayout,
    QWidget,
    QLabel,
    QLineEdit,
    QGroupBox,
    QSizePolicy,
    QComboBox,
    QSplitter,
    QCheckBox,
    QTextEdit,
    QPushButton,
    QListWidget,
    QListWidgetItem,
    QAbstractItemView,
    QScrollArea,
    QTabWidget,
    QFrame,
)
from qtpy.QtGui import QDoubleValidator, QMovie, QDragEnterEvent, QDropEvent, QDragMoveEvent, QColor, QPalette
from qtpy.QtCore import Qt, QEvent, QSize
from superqt import QDoubleRangeSlider
from pyvistaqt import BackgroundPlotter
from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.widgets import Cursor
from pyvista.plotting.picking import RectangleSelection
from pyvista.plotting.opts import PickerType
from vtkmodules.vtkCommonDataModel import vtkBox, vtkCylinder, vtkImplicitFunction
from vtkmodules.vtkInteractionWidgets import (
    vtkImplicitCylinderWidget,
    vtkImplicitCylinderRepresentation,
    vtkBoxWidget2,
    vtkBoxRepresentation,
)
from vtkmodules.vtkCommonCore import vtkCommand
import numpy as np
import pyvista as pv
import matplotlib.pyplot as plt
from mantid.dataobjects import Workspace2D
from mantid import UsageService, ConfigService
from mantid.kernel import FeatureType
from mantidqt.plotting.mantid_navigation_toolbar import MantidNavigationToolbar
from mantidqt.utils.qt.qappthreadcall import run_on_qapp_thread

from instrumentview.Detectors import DetectorInfo
from instrumentview.InteractorStyles import CustomInteractorStyleZoomAndSelect, CustomInteractorStyleRubberBand3D
from instrumentview.Projections.ProjectionType import ProjectionType
from instrumentview.Globals import CurrentTab
from instrumentview.ComponentTreeView import ComponentTreeView

import os
from contextlib import suppress
from typing import Callable


class CylinderWidgetNoRotation(vtkImplicitCylinderWidget):
    def __init__(self):
        super().__init__()
        self.AddObserver(vtkCommand.StartInteractionEvent, lambda *_: self._on_interaction())

    def _on_interaction(self):
        # Replace rotation state (integer 4) with translation state (integer 3)
        if self.GetCylinderRepresentation().GetInteractionState() == 4:
            self.GetCylinderRepresentation().SetInteractionState(3)
            return


class RectangleWidgetNoRotation(vtkBoxWidget2):
    def __init__(self):
        super().__init__()
        self.AddObserver(vtkCommand.StartInteractionEvent, lambda *_: self._on_interaction())

    def _on_interaction(self):
        # Replace rotation state (integer 8) with translation state (integer 7)
        if self.GetRepresentation().GetInteractionState() == 8:
            self.GetRepresentation().SetInteractionState(7)
            return


class WorkspaceListWidget(QListWidget):
    def __init__(self, parent):
        super().__init__(parent)
        self.setDragDropMode(QAbstractItemView.DropOnly)

    def dragEnterEvent(self, event: QDragEnterEvent):
        if event.mimeData().hasText():
            event.acceptProposedAction()

    def dragMoveEvent(self, event: QDragMoveEvent):
        if event.mimeData().hasText():
            event.acceptProposedAction()

    def dropEvent(self, event: QDropEvent):
        drop_text = event.mimeData().text()
        for i in range(self.count()):
            item = self.item(i)
            if item.text() == drop_text:
                item.setCheckState(Qt.Checked)
        event.acceptProposedAction()


class NoWheelComboBox(QComboBox):
    """QComboBox that ignores mouse wheel events unless it has focus."""

    def wheelEvent(self, event):
        event.ignore()


@run_on_qapp_thread()
class FullInstrumentViewWindow(QMainWindow):
    """View for the Instrument View window. Contains the 3D view, the projection view, boxes showing information about the selected
    detector, and a line plot of selected detector(s)"""

    _detector_spectrum_fig = None
    _ASPECT_RATIO_SETTING_STRING = "InstrumentView.MaintainAspectRato"

    def __init__(self, parent=None, off_screen=False):
        """The instrument in the given workspace will be displayed. The off_screen option is for testing or rendering an image
        e.g. in a script."""

        pv.global_theme.background = "black"
        pv.global_theme.font.color = "white"

        super(FullInstrumentViewWindow, self).__init__(parent)
        self.setWindowTitle("Instrument View")
        self._off_screen = off_screen

        central_widget = QWidget(self)
        self.setCentralWidget(central_widget)
        tab_widget = QTabWidget()
        parent_horizontal_layout = QHBoxLayout(central_widget)

        pyvista_vertical_widget = QWidget()
        left_column_widget = QWidget()
        tab_widget.addTab(left_column_widget, "Home")
        pyvista_vertical_layout = QVBoxLayout(pyvista_vertical_widget)
        left_column_layout = QVBoxLayout(left_column_widget)

        left_column_scroll = QScrollArea()
        left_column_scroll.setWidgetResizable(True)
        left_column_scroll.setWidget(tab_widget)
        left_column_scroll.setFrameShape(QFrame.NoFrame)

        hsplitter = QSplitter(Qt.Horizontal)
        hsplitter.addWidget(left_column_scroll)
        hsplitter.addWidget(pyvista_vertical_widget)
        hsplitter.setSizes([150, 200])
        hsplitter.splitterMoved.connect(self._on_splitter_moved)
        parent_horizontal_layout.addWidget(hsplitter)

        self.main_plotter = BackgroundPlotter(show=False, menu_bar=False, toolbar=False, off_screen=off_screen)
        pyvista_vertical_layout.addWidget(self.main_plotter.app_window)

        self._detector_spectrum_fig, self._detector_spectrum_axes = plt.subplots(subplot_kw={"projection": "mantid"})
        self._detector_figure_canvas = FigureCanvas(self._detector_spectrum_fig)
        self._detector_figure_canvas.setMinimumSize(QSize(0, 0))
        self._plot_toolbar = MantidNavigationToolbar(self._detector_figure_canvas, self)
        plot_widget = QWidget()
        plot_layout = QVBoxLayout(plot_widget)
        plot_layout.addWidget(self._detector_figure_canvas)
        plot_layout.addWidget(self._plot_toolbar)
        self._lineplot_peak_cursor = None

        vsplitter = QSplitter(Qt.Vertical)
        vsplitter.addWidget(self.main_plotter.app_window)
        vsplitter.addWidget(plot_widget)
        vsplitter.setStretchFactor(0, 5)
        vsplitter.setStretchFactor(1, 4)
        vsplitter.splitterMoved.connect(self._on_splitter_moved)

        pyvista_vertical_layout.addWidget(vsplitter)

        detector_group_box = QGroupBox("Detector Info")
        detector_info_layout = QVBoxLayout(detector_group_box)
        self._detector_name_edit = self._add_detector_info_boxes(detector_info_layout, "Name")
        self._detector_id_edit = self._add_detector_info_boxes(detector_info_layout, "Detector ID")
        self._detector_workspace_index_edit = self._add_detector_info_boxes(detector_info_layout, "Workspace Index")
        self._detector_component_path_edit = self._add_detector_info_boxes(detector_info_layout, "Component Path")
        self._detector_xyz_edit = self._add_detector_info_boxes(detector_info_layout, "XYZ Position")
        self._detector_spherical_position_edit = self._add_detector_info_boxes(detector_info_layout, "Spherical Position")
        self._detector_pixel_counts_edit = self._add_detector_info_boxes(detector_info_layout, "Pixel Counts")
        self._detector_relative_angle_edit = self._add_detector_info_boxes(detector_info_layout, "Relative Angle (degrees)")
        self.set_relative_detector_angle(None)

        self._integration_limit_group_box = QGroupBox("Time of Flight")
        (
            self._integration_limit_min_edit,
            self._integration_limit_max_edit,
            self._integration_limit_slider,
            self._integration_limit_reset,
        ) = self._add_min_max_group_box(self._integration_limit_group_box)
        self._contour_range_group_box = QGroupBox("Contour Range")
        self._contour_range_min_edit, self._contour_range_max_edit, self._contour_range_slider, self._contour_range_reset = (
            self._add_min_max_group_box(self._contour_range_group_box)
        )

        projection_group_box = QGroupBox("Projection")
        projection_layout = QHBoxLayout(projection_group_box)
        self._projection_combo_box = NoWheelComboBox(self)
        self._reset_projection = QPushButton("Reset Projection")
        self._reset_projection.setToolTip("Resets the projection to default.")
        self._reset_projection.clicked.connect(self.reset_camera)
        self._clear_point_picked_detectors = QPushButton("Clear Mouse Picking")
        self._aspect_ratio_check_box = QCheckBox()
        self._aspect_ratio_check_box.setText("Maintain Aspect Ratio")
        self._aspect_ratio_check_box.setToolTip(
            "If checked, the detectors will be drawn in 2D projections in their original aspect ratio, "
            "otherwise they will fill the available area."
        )
        aspect_ratio_option = ConfigService.Instance()[self._ASPECT_RATIO_SETTING_STRING]
        self._aspect_ratio_check_box.setChecked(aspect_ratio_option.casefold() == "yes")
        self._show_monitors_check_box = QCheckBox()
        self._show_monitors_check_box.setText("Show Monitors?")
        projection_layout.addWidget(self._projection_combo_box)
        projection_layout.addWidget(self._reset_projection)
        projection_layout.addWidget(self._clear_point_picked_detectors)
        projection_layout.addWidget(self._aspect_ratio_check_box)
        projection_layout.addWidget(self._show_monitors_check_box)

        peak_ws_group_box = QGroupBox("Peaks Workspaces")
        peak_v_layout = QVBoxLayout(peak_ws_group_box)
        peak_buttons_h_layout = QHBoxLayout()
        self._add_peak_button = QPushButton("Add Peak")
        self._delete_peak_button = QPushButton("Delete Single Peak")
        self._delete_all_selected_peaks_button = QPushButton("Delete All Selected Peaks")
        self._peak_ws_list = WorkspaceListWidget(self)
        self._peak_ws_list.setSizeAdjustPolicy(QListWidget.AdjustToContents)
        peak_buttons_h_layout.addWidget(self._add_peak_button)
        peak_buttons_h_layout.addWidget(self._delete_peak_button)
        peak_buttons_h_layout.addWidget(self._delete_all_selected_peaks_button)
        peak_v_layout.addLayout(peak_buttons_h_layout)
        peak_v_layout.addWidget(self._peak_ws_list)

        grouping_masking_group_box = QGroupBox("Grouping and Masking")
        grouping_masking_group_layout = QVBoxLayout(grouping_masking_group_box)
        shapes_widget = QWidget()
        shapes_layout = QHBoxLayout(shapes_widget)
        self._add_circle = QPushButton("Add Circle")
        self._add_circle.setCheckable(True)
        self._add_rectangle = QPushButton("Add Rectangle")
        self._add_rectangle.setCheckable(True)
        shapes_layout.addWidget(self._add_circle)
        shapes_layout.addWidget(self._add_rectangle)
        self._shape_buttons = [self._add_circle, self._add_rectangle]

        selection_tab = QWidget()
        (
            self._add_selection,
            self._clear_selections,
            self._selection_list,
            self._save_roi_to_ws,
            self._save_grouping_to_ws,
            self._save_grouping_to_xml,
        ) = self._add_tab(selection_tab, "ROI")
        self._save_roi_to_ws.setText("Export ROI to ADS")
        self._save_grouping_to_ws.setText("Export Grouping to ADS")
        self._save_grouping_to_xml.setText("Save Grouping to XML")

        mask_tab = QWidget()
        (self._add_mask, self._clear_masks, self._mask_list, self._save_mask_to_ws, self._save_mask_to_file, self._overwrite_mask) = (
            self._add_tab(mask_tab, "Mask")
        )
        self._save_mask_to_ws.setText("Save Mask to ADS")
        self._save_mask_to_file.setText("Save Mask to XML")
        self._overwrite_mask.setText("Apply Mask Permanently")

        self._picking_masking_tab = QTabWidget()
        self._picking_masking_tab.addTab(selection_tab, CurrentTab.Grouping.value)
        self._picking_masking_tab.addTab(mask_tab, CurrentTab.Masking.value)
        grouping_masking_group_layout.addWidget(shapes_widget)
        grouping_masking_group_layout.addWidget(self._picking_masking_tab)

        self.status_group_box = QGroupBox("Status")
        status_layout = QHBoxLayout(self.status_group_box)
        status_label = QLabel("Loading ...")
        status_graphics = QLabel()
        status_graphics.setFixedSize(50, 50)
        status_graphics.setAlignment(Qt.AlignCenter)
        status_graphics.setScaledContents(True)
        spinner = QMovie(f"{os.path.dirname(__file__)}/loading.gif")
        status_graphics.setMovie(spinner)
        status_layout.addWidget(status_label)
        status_layout.addWidget(status_graphics)
        spinner.start()
        status_layout.addStretch()

        options_vertical_widget = QWidget()
        options_vertical_layout = QVBoxLayout(options_vertical_widget)
        options_vertical_layout.addWidget(detector_group_box)
        options_vertical_layout.addWidget(self._integration_limit_group_box)
        options_vertical_layout.addWidget(self._contour_range_group_box)
        options_vertical_layout.addWidget(projection_group_box)
        units_group_box = QGroupBox("Units")
        units_vbox = QVBoxLayout()
        self._setup_units_options(units_vbox)
        units_group_box.setLayout(units_vbox)
        options_vertical_layout.addWidget(units_group_box)
        options_vertical_layout.addWidget(peak_ws_group_box)
        options_vertical_layout.addWidget(grouping_masking_group_box)
        options_vertical_layout.addWidget(QSplitter(Qt.Horizontal))

        options_vertical_layout.addWidget(self.status_group_box)
        left_column_layout.addWidget(options_vertical_widget)
        left_column_layout.addStretch()

        component_tree_tab = QWidget()
        component_layout = QVBoxLayout(component_tree_tab)
        self.component_tree = ComponentTreeView()
        self.component_tree.setSelectionMode(QAbstractItemView.ExtendedSelection)
        component_layout.addWidget(self.component_tree)
        tab_widget.addTab(component_tree_tab, "Component Tree")

        self.interactor_style = CustomInteractorStyleZoomAndSelect()
        self._overlay_meshes = []
        self._lineplot_overlays = []

        screen_geometry = self.screen().geometry()
        self.resize(int(screen_geometry.width() * 0.8), int(screen_geometry.height() * 0.8))
        window_geometry = self.frameGeometry()
        center_point = screen_geometry.center()
        window_geometry.moveCenter(center_point)
        self.move(window_geometry.topLeft())

        self._current_widget = None
        self._projection_camera_map = {}
        self._parallel_scales = {}

        UsageService.registerFeatureUsage(FeatureType.Interface, "InstrumentView2025", False)

    def check_sum_spectra_checkbox(self) -> None:
        self._sum_spectra_checkbox.setChecked(True)
        self._presenter.on_sum_spectra_checkbox_clicked()

    def is_maintain_aspect_ratio_checkbox_checked(self) -> bool:
        return self._aspect_ratio_check_box.isChecked()

    def store_maintain_aspect_ratio_option(self) -> None:
        option = "Yes" if self.is_maintain_aspect_ratio_checkbox_checked() else "No"
        ConfigService.Instance()[self._ASPECT_RATIO_SETTING_STRING] = option

    def enable_or_disable_aspect_ratio_box(self) -> None:
        self._aspect_ratio_check_box.setDisabled(self.current_selected_projection() == ProjectionType.THREE_D)

    def is_show_monitors_checkbox_checked(self) -> bool:
        return self._show_monitors_check_box.isChecked()

    def _on_splitter_moved(self, pos, index) -> None:
        self._detector_spectrum_fig.tight_layout()

    def hide_status_box(self) -> None:
        self.status_group_box.hide()

    def reset_camera(self) -> None:
        if self._off_screen:
            return

        if self.current_selected_projection() in self._projection_camera_map.keys():
            self.main_plotter.camera_position = self._projection_camera_map[self.current_selected_projection()]
            self.main_plotter.camera.parallel_scale = self._parallel_scales[self.current_selected_projection()]
        else:
            # Apply default position, in case cache not available
            self.main_plotter.reset_camera()
        return

    def cache_camera_position(self) -> None:
        self.main_plotter.reset_camera()
        self._projection_camera_map[self.current_selected_projection()] = self.main_plotter.camera_position
        self._parallel_scales[self.current_selected_projection()] = self.main_plotter.camera.parallel_scale

    def _add_min_max_group_box(self, parent_box: QGroupBox) -> tuple[QLineEdit, QLineEdit, QDoubleRangeSlider, QPushButton]:
        """Creates a minimum and a maximum box (with labels) inside the given group box. The callbacks will be attached to textEdited
        signal of the boxes"""
        min_hbox = QHBoxLayout()
        min_hbox.addWidget(QLabel("Min"))
        min_edit = QLineEdit()
        max_float_64 = np.finfo(np.float64).max
        min_edit.setValidator(QDoubleValidator(0, max_float_64, 4, self))
        min_hbox.addWidget(min_edit)
        max_hbox = QHBoxLayout()
        max_hbox.addWidget(QLabel("Max"))
        max_edit = QLineEdit()
        max_edit.setValidator(QDoubleValidator(0, max_float_64, 4, self))
        max_hbox.addWidget(max_edit)
        reset_hbox = QHBoxLayout()
        reset_button = QPushButton("Reset")
        reset_hbox.addWidget(reset_button)

        slider = QDoubleRangeSlider(Qt.Orientation.Horizontal, parent=parent_box)
        slider.setRange(0, 1)
        slider_hbox = QHBoxLayout()
        slider_hbox.addWidget(slider)

        root_hbox = QHBoxLayout()
        root_hbox.addLayout(min_hbox)
        root_hbox.addLayout(max_hbox)
        root_hbox.addLayout(reset_hbox)

        root_vbox = QVBoxLayout()
        root_vbox.addLayout(slider_hbox)
        root_vbox.addLayout(root_hbox)
        parent_box.setLayout(root_vbox)

        return (min_edit, max_edit, slider, reset_button)

    def set_contour_min_max_boxes(self, limits: tuple[float, float]) -> None:
        self._set_min_max_edit_boxes(self._contour_range_min_edit, self._contour_range_max_edit, limits)

    def set_integration_min_max_boxes(self, limits: tuple[float, float]) -> None:
        self._set_min_max_edit_boxes(self._integration_limit_min_edit, self._integration_limit_max_edit, limits)

    def _set_min_max_edit_boxes(self, min_edit: QLineEdit, max_edit: QLineEdit, limits: tuple[float, float]) -> None:
        min, max = limits

        def format_float(value):
            return f"{value:.4f}".rstrip("0").rstrip(".")

        min_edit.setText(format_float(min))
        max_edit.setText(format_float(max))

    def _add_connections_to_edits_and_slider(self, min_edit: QLineEdit, max_edit: QLineEdit, slider, presenter_callback: Callable):
        def set_slider(callled_from_min):
            def wrapped():
                try:
                    min_val, max_val = float(min_edit.text()), float(max_edit.text())
                except ValueError:
                    return
                if callled_from_min:
                    min_val = min(min_val, max_val)
                else:
                    max_val = max(min_val, max_val)
                slider.setValue((min_val, max_val))
                presenter_callback()
                return

            return wrapped

        # Connections to sync sliders and edits
        slider.valueChanged.connect(lambda lims: self._set_min_max_edit_boxes(min_edit, max_edit, lims))
        min_edit.editingFinished.connect(set_slider(callled_from_min=True))
        max_edit.editingFinished.connect(set_slider(callled_from_min=False))

    def _add_detector_info_boxes(self, parent_box: QVBoxLayout, label: str) -> QTextEdit:
        """Adds a text box to the given parent that is designed to show read-only information about the selected detector"""
        hbox = QHBoxLayout()
        hbox.addWidget(QLabel(label))
        line_edit = QTextEdit()
        line_edit.document().setTextWidth(line_edit.viewport().width())
        line_edit.setFixedHeight(27)
        line_edit.setReadOnly(True)
        line_edit.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Fixed)
        hbox.addWidget(line_edit)
        parent_box.addLayout(hbox)
        palette = line_edit.palette()
        palette.setColor(QPalette.Base, palette.color(QPalette.Window))
        line_edit.setPalette(palette)
        return line_edit

    def _add_tab(self, parent_tab: QWidget, label: str):
        tab_layout = QVBoxLayout(parent_tab)
        add_item_btn = QPushButton(f"Add {label}")
        clear_items_btn = QPushButton("Clear All")
        pre_list_layout = QHBoxLayout()
        pre_list_layout.addWidget(add_item_btn)
        pre_list_layout.addWidget(clear_items_btn)
        item_list = WorkspaceListWidget(self)
        item_list.setSizeAdjustPolicy(QListWidget.AdjustToContents)
        item_list.setSelectionMode(QAbstractItemView.NoSelection)
        post_list_layout = QHBoxLayout()
        save_to_ws_btn = QPushButton()
        save_to_file_btn = QPushButton()
        overwrite_btn = QPushButton()
        post_list_layout.addWidget(save_to_ws_btn)
        post_list_layout.addWidget(save_to_file_btn)
        post_list_layout.addWidget(overwrite_btn)
        tab_layout.addLayout(pre_list_layout)
        tab_layout.addWidget(item_list)
        tab_layout.addLayout(post_list_layout)
        return (add_item_btn, clear_items_btn, item_list, save_to_ws_btn, save_to_file_btn, overwrite_btn)

    def subscribe_presenter(self, presenter) -> None:
        self._presenter = presenter
        for unit in self._presenter.available_unit_options():
            self._units_combo_box.addItem(unit)
        self._integration_limit_group_box.setTitle(self._presenter.workspace_display_unit)
        self.main_plotter.set_color_cycler(self._presenter._COLOURS)
        self.refresh_peaks_ws_list()
        self.refresh_workspaces_in_list(CurrentTab.Masking)
        self.refresh_workspaces_in_list(CurrentTab.Grouping)

    def setup_connections_to_presenter(self) -> None:
        self._projection_combo_box.currentIndexChanged.connect(self._presenter.update_plotter)
        self._clear_point_picked_detectors.clicked.connect(self._presenter.on_clear_point_picked_detectors_clicked)
        self._contour_range_slider.sliderReleased.connect(self._presenter.on_contour_limits_updated)
        self._contour_range_reset.clicked.connect(self._presenter.on_contour_range_reset_clicked)
        self._integration_limit_slider.sliderReleased.connect(self._presenter.on_integration_limits_updated)
        self._integration_limit_reset.clicked.connect(self._presenter.on_integration_limits_reset_clicked)
        self._units_combo_box.currentIndexChanged.connect(self._presenter.on_unit_option_selected)
        self._export_workspace_button.clicked.connect(self._presenter.on_export_workspace_clicked)
        self._sum_spectra_checkbox.clicked.connect(self._presenter.on_sum_spectra_checkbox_clicked)
        self._peak_ws_list.itemChanged.connect(self._presenter.on_peaks_workspace_selected)
        self._mask_list.itemChanged.connect(partial(self._presenter.on_list_item_selected, CurrentTab.Masking))
        self._selection_list.itemChanged.connect(partial(self._presenter.on_list_item_selected, CurrentTab.Grouping))
        self._save_mask_to_ws.clicked.connect(self._presenter.on_save_to_workspace_clicked)
        self._save_mask_to_file.clicked.connect(self._presenter.on_save_mask_to_xml_clicked)
        self._overwrite_mask.clicked.connect(self._presenter.on_apply_permanently_clicked)
        self._save_roi_to_ws.clicked.connect(self._presenter.on_save_to_workspace_clicked)
        self._save_grouping_to_ws.clicked.connect(self._presenter.on_save_grouping_to_ads_clicked)
        self._save_grouping_to_xml.clicked.connect(self._presenter.on_save_grouping_to_xml_clicked)
        self._clear_masks.clicked.connect(self._presenter.on_clear_list_clicked)
        self._clear_selections.clicked.connect(self._presenter.on_clear_list_clicked)
        self._aspect_ratio_check_box.clicked.connect(self._presenter.on_aspect_ratio_check_box_clicked)
        self._add_peak_button.clicked.connect(self._presenter.on_add_peak_clicked)
        self._delete_peak_button.clicked.connect(self._presenter.on_delete_peak_clicked)
        self._delete_all_selected_peaks_button.clicked.connect(self._presenter.on_delete_all_selected_peaks_clicked)
        self._show_monitors_check_box.clicked.connect(self._presenter.on_show_monitors_check_box_clicked)

        self._add_connections_to_edits_and_slider(
            self._contour_range_min_edit,
            self._contour_range_max_edit,
            self._contour_range_slider,
            self._presenter.on_contour_limits_updated,
        )
        self._add_connections_to_edits_and_slider(
            self._integration_limit_min_edit,
            self._integration_limit_max_edit,
            self._integration_limit_slider,
            self._presenter.on_integration_limits_updated,
        )

        self._add_circle.toggled.connect(self.on_toggle_add_circle)
        self._add_rectangle.toggled.connect(self.on_toggle_add_rectangle)

        self._add_mask.clicked.connect(self._presenter.on_add_item_clicked)
        self._add_mask.setDisabled(True)
        self._add_selection.clicked.connect(self._presenter.on_add_item_clicked)
        self._add_selection.setDisabled(True)

    def on_toggle_add_circle(self, checked):
        self._on_toggle_add_shape(checked, self.add_cylinder_widget)

    def on_toggle_add_rectangle(self, checked):
        self._on_toggle_add_shape(checked, self.add_rectangular_widget)

    def _on_toggle_add_shape(self, checked, add_widget_function: Callable):
        if checked:
            add_widget_function()
        else:
            self.delete_current_widget()

        # Enable button for applying mask/group if widget is present, disable otherwise
        self._add_mask.setEnabled(checked)
        self._add_selection.setEnabled(checked)

        # Disable buttons for adding more widgets if widget is already present, enable otherwise
        for btn in self._shape_buttons:
            if btn != self.sender():
                btn.setDisabled(checked)

    def enable_or_disable_mask_widgets(self):
        for btn in self._shape_buttons:
            if btn.isChecked():
                btn.toggle()
            btn.setDisabled(self.current_selected_projection() == ProjectionType.THREE_D)

    def delete_current_widget(self):
        # Should delete widgets explicitly, otherwise not garbage collected
        if not self._current_widget:
            return
        self._current_widget.EnabledOff()
        self._current_widget.SetInteractor(None)
        self._current_widget.RemoveAllObservers()
        del self._current_widget
        self._current_widget = None

    def _setup_units_options(self, parent: QVBoxLayout):
        """Add widgets for the units options"""
        self._units_combo_box = NoWheelComboBox(self)
        self._units_combo_box.setToolTip("Select the units for the spectra line plot")
        parent.addWidget(self._units_combo_box)
        hBox = QHBoxLayout()
        self._export_workspace_button = QPushButton("Export Spectra to ADS", self)
        hBox.addWidget(self._export_workspace_button)
        self._sum_spectra_checkbox = QCheckBox(self)
        self._sum_spectra_checkbox.setChecked(True)
        self._sum_spectra_checkbox.setText("Sum Selected Spectra")
        self._sum_spectra_checkbox.setToolTip(
            "Selected spectra will be converted to d-Spacing, summed, then converted back to the desired unit."
        )
        hBox.addWidget(self._sum_spectra_checkbox)
        parent.addLayout(hBox)

    def refresh_peaks_ws_list(self) -> None:
        # Maintain any peaks workspaces that are checked
        current_checked_peak_workspaces: list[str] = []
        for list_i in range(self._peak_ws_list.count()):
            list_item = self._peak_ws_list.item(list_i)
            if list_item.checkState() > 0:
                current_checked_peak_workspaces.append(list_item.text())

        self._peak_ws_list.blockSignals(True)
        self._peak_ws_list.clear()
        peaks_workspaces = self._presenter.peaks_workspaces_in_ads()
        for peaks_ws_index in range(len(peaks_workspaces)):
            peaks_ws = peaks_workspaces[peaks_ws_index]
            list_item = QListWidgetItem(peaks_ws, self._peak_ws_list)
            if peaks_ws in current_checked_peak_workspaces:
                list_item.setCheckState(Qt.Checked)
            else:
                list_item.setCheckState(Qt.Unchecked)
        self._peak_ws_list.blockSignals(False)
        self._peak_ws_list.adjustSize()

    def refresh_workspaces_in_list(self, kind: CurrentTab) -> None:
        list_to_refresh = self._mask_list if kind is CurrentTab.Masking else self._selection_list

        keys_from_workspaces_in_ads = self._presenter.get_list_keys_from_workspaces_in_ads(kind)
        keys_in_current_list = [list_to_refresh.item(i).text() for i in range(list_to_refresh.count())]
        keys_cached_in_model = self._presenter.cached_keys(kind)

        # Keys from ads but not yet in list
        for key in keys_from_workspaces_in_ads:
            if key not in keys_in_current_list:
                item = QListWidgetItem(key, list_to_refresh)
                item.setCheckState(Qt.Unchecked)

        # Remove keys that are not cached in model and not in ads
        for i in range(list_to_refresh.count() - 1, -1, -1):
            item = list_to_refresh.item(i)
            if item.text() not in keys_cached_in_model and item.text() not in keys_from_workspaces_in_ads:
                removed = list_to_refresh.takeItem(i)
                del removed

    def refresh_peaks_ws_list_colours(self) -> None:
        picked_index = 0
        for list_i in range(self._peak_ws_list.count()):
            list_item = self._peak_ws_list.item(list_i)
            if list_item.checkState() > 0:
                list_item.setForeground(QColor(self._presenter._COLOURS[picked_index % len(self._presenter._COLOURS)]))
                picked_index += 1
            else:
                list_item.setForeground(self._peak_ws_list.palette().color(QPalette.Text))

    def select_peaks_workspace(self, peaks_ws: str) -> None:
        for list_i in range(self._peak_ws_list.count()):
            list_item = self._peak_ws_list.item(list_i)
            if list_item.text() == peaks_ws:
                list_item.setCheckState(Qt.Checked)
                return

    def set_add_peak_button_enabled(self, is_enabled: bool) -> None:
        self._add_peak_button.setEnabled(is_enabled)

    def set_delete_peak_button_enabled(self, is_enabled: bool) -> None:
        self._delete_peak_button.setEnabled(is_enabled)

    def set_delete_all_selected_peaks_button_enabled(self, is_enabled: bool) -> None:
        self._delete_all_selected_peaks_button.setEnabled(is_enabled)

    def set_unit_combo_box_index(self, index: int) -> None:
        self._units_combo_box.setCurrentIndex(index)

    def current_selected_unit(self) -> str:
        """Get the currently selected unit from the combo box"""
        return self._units_combo_box.currentText()

    def sum_spectra_selected(self) -> bool:
        return self._sum_spectra_checkbox.isChecked()

    def get_integration_limits(self) -> tuple[float, float]:
        return self._integration_limit_slider.value()

    def set_integration_range_limits(self, integration_limits: tuple[float, float]) -> None:
        """Update the integration limit edit boxes with formatted text"""
        lower, upper = integration_limits
        if upper <= lower:
            self._integration_limit_group_box.hide()
            return
        self._integration_limit_slider.setRange(*integration_limits)
        self._integration_limit_slider.setValue(integration_limits)
        return

    def get_contour_limits(self) -> tuple[float, float]:
        return self._contour_range_slider.value()

    def set_contour_range_limits(self, contour_limits: tuple[int, int]) -> None:
        """Update the contour range edit boxes with formatted text"""
        lower, upper = contour_limits
        if upper <= lower:
            self._contour_range_group_box.hide()
            return
        self._contour_range_slider.setRange(*contour_limits)
        self._contour_range_slider.setValue(contour_limits)
        return

    def set_plotter_scalar_bar_range(self, clim: tuple[int, int], label: str) -> None:
        """Set the range of the colours displayed, i.e. the legend"""
        self.main_plotter.update_scalar_bar_range(clim, label)

    def closeEvent(self, QCloseEvent: QEvent) -> None:
        """When closing, make sure to close the plotters and figure correctly to prevent errors"""
        super().closeEvent(QCloseEvent)
        with suppress(TypeError):
            self._contour_range_max_edit.disconnect()
            self._contour_range_min_edit.disconnect()
        self.main_plotter.close()
        if self._detector_spectrum_fig is not None:
            plt.close(self._detector_spectrum_fig.get_label())
        if hasattr(self, "_presenter") and self._presenter is not None:
            self._presenter.handle_close()

    def set_projection_combo_options(self, default_index: int, options: list[str]) -> None:
        self._projection_combo_box.addItems(options)
        self._projection_combo_box.setCurrentIndex(default_index)

    def current_selected_projection(self) -> str:
        return self._projection_combo_box.currentText()

    def add_simple_shape(self, mesh: PolyData, colour=None, pickable=False) -> None:
        """Draw the given mesh in the main plotter window"""
        self.main_plotter.add_mesh(mesh, color=colour, pickable=pickable)

    def clear_main_plotter(self) -> None:
        self.delete_current_widget()
        self.main_plotter.clear()

    def add_detector_mesh(self, mesh: PolyData, is_projection: bool, scalars=None) -> None:
        """Draw the given mesh in the main plotter window"""
        scalar_bar_args = dict(interactive=True, vertical=False, title_font_size=15, label_font_size=12) if scalars is not None else None
        self.main_plotter.add_mesh(
            mesh, pickable=False, scalars=scalars, render_points_as_spheres=True, point_size=15, scalar_bar_args=scalar_bar_args
        )

        if self.main_plotter.off_screen:
            return

        if not is_projection:
            self.main_plotter.enable_trackball_style()
            return

        self.main_plotter.view_xy()
        self.main_plotter.enable_parallel_projection()
        self.main_plotter.enable_zoom_style()

    def add_pickable_mesh(self, point_cloud: PolyData, scalars: np.ndarray | str) -> None:
        self.main_plotter.add_mesh(
            point_cloud,
            scalars=scalars,
            opacity=[0.0, 0.3],
            clim=[0, 1],
            show_scalar_bar=False,
            pickable=True,
            cmap="Oranges",
            point_size=30,
            render_points_as_spheres=True,
        )

    def add_masked_mesh(self, mesh: PolyData) -> None:
        if mesh.number_of_points == 0:
            return
        # RGB for dark grey is (64, 64, 64), normalised is (0.25, 0.25, 0.25)
        self.main_plotter.add_mesh(mesh, color=(0.25, 0.25, 0.25), pickable=False, render_points_as_spheres=True, point_size=15)

    def add_cylinder_widget(self) -> None:
        cylinder_repr = vtkImplicitCylinderRepresentation()
        cylinder_repr.SetOutlineTranslation(False)
        # Set bounding box line to invisible
        cylinder_repr.GetOutlineProperty().SetOpacity(0)
        cylinder_repr.SetMinRadius(0.001)

        width, height = self.main_plotter.renderer.GetSize()
        cx, cy, _cz = self.display_to_world_coords(width / 2, height / 2, 0)
        cylinder_repr.SetCenter([cx, cy, 0.5])

        x, y, _z = self.display_to_world_coords(width / 2 + 0.15 * width, height / 2, 0)
        cylinder_repr.SetRadius(np.sqrt((x - cx) ** 2 + (y - cy) ** 2))

        # Arbritary border factor for bounding box
        xmin, xmax, ymin, ymax, _zmin, _zmax = self.main_plotter.bounds
        border = (np.sqrt((xmax - xmin) ** 2 + (ymax - ymin) ** 2)) / 2
        cylinder_repr.SetWidgetBounds([xmin - border, xmax + border, ymin - border, ymax + border, 0, 1])

        # For 2D projections, camera view is always perpendicular to Z axis
        cylinder_repr.SetAxis([0, 0, 1])
        cylinder_widget = CylinderWidgetNoRotation()
        cylinder_widget.SetRepresentation(cylinder_repr)
        cylinder_widget.SetCurrentRenderer(self.main_plotter.renderer)
        cylinder_widget.SetInteractor(self.main_plotter.iren.interactor)
        cylinder_widget.On()
        self._current_widget = cylinder_widget
        # The command below is a hacky way of making the widget appear on top of detectors
        # No idea why it works
        self.main_plotter.camera_position = self.main_plotter.camera_position

    def add_rectangular_widget(self) -> None:
        rect_repr = vtkBoxRepresentation()

        width, height = self.main_plotter.renderer.GetSize()
        x0, y0, _z0 = self.display_to_world_coords(width / 3, height / 3, 0)
        x1, y1, _z1 = self.display_to_world_coords(2 * width / 3, 2 * height / 3, 0)
        rect_repr.SetPlaceFactor(1.0)
        rect_repr.PlaceWidget([x0, x1, y0, y1, -0.1, 1])
        rect_repr.SetUseBounds(True)

        rect_widget = RectangleWidgetNoRotation()
        rect_widget.SetRepresentation(rect_repr)
        rect_widget.SetCurrentRenderer(self.main_plotter.renderer)
        rect_widget.SetInteractor(self.main_plotter.iren.interactor)
        rect_widget.On()
        self._current_widget = rect_widget
        # The command below is a hacky way of making the widget appear on top of detectors
        # No idea why it works
        self.main_plotter.camera_position = self.main_plotter.camera_position
        return

    def display_to_world_coords(self, x, y, z):
        # Convert from display coordinates to world coordinates
        renderer = self.main_plotter.renderer
        renderer.SetDisplayPoint(x, y, z)
        renderer.DisplayToWorld()
        world_x, world_y, world_z, world_w = renderer.GetWorldPoint()
        return world_x / world_w, world_y / world_w, world_z / world_w

    def get_current_widget_implicit_function(self) -> vtkImplicitFunction | None:
        if isinstance(self._current_widget, CylinderWidgetNoRotation):
            cylinder = vtkCylinder()
            self._current_widget.GetCylinderRepresentation().GetCylinder(cylinder)
            return cylinder
        elif isinstance(self._current_widget, RectangleWidgetNoRotation):
            box = vtkBox()
            box.SetBounds(self._current_widget.GetRepresentation().GetBounds())
            return box
        else:
            return None

    def add_rgba_mesh(self, mesh: PolyData, scalars: np.ndarray | str):
        """Draw the given mesh in the main plotter window, and set the colours manually with RGBA numbers"""
        self.main_plotter.add_mesh(mesh, scalars=scalars, rgba=True, pickable=False, render_points_as_spheres=True, point_size=10)

    def enable_point_picking(self, is_projection: bool, callback: Callable) -> None:
        """Switch on point picking, i.e. picking a single point with right-click"""
        self.main_plotter.disable_picking()
        # NOTE: Need to remove interactor to avoid artifacts in 2D or 3D
        self.interactor_style.remove_interactor()
        picking_tolerance = 0.01
        if not self.main_plotter.off_screen:
            if is_projection:
                self.main_plotter.enable_zoom_style()
            else:
                self.main_plotter.enable_trackball_style()
            self.main_plotter.enable_surface_point_picking(
                show_message=False,
                use_picker=True,
                callback=callback,
                show_point=False,
                pickable_window=False,
                picker="point",
                tolerance=picking_tolerance,
            )

    def enable_rectangle_picking(self, is_projection: bool, callback: Callable) -> None:
        """Switch on rectangle picking, i.e. draw a rectangle to select all detectors within the rectangle"""
        self.main_plotter.disable_picking()

        if not self.main_plotter.off_screen:
            if is_projection:
                self.interactor_style.set_interactor(self.main_plotter.iren.interactor)
                self.main_plotter.iren.style = self.interactor_style
            else:
                self.main_plotter.iren.style = CustomInteractorStyleRubberBand3D()

            def _end_pick_helper(picker, *_):
                callback(RectangleSelection(frustum=picker.GetFrustum(), viewport=(-1, -1, -1, -1)))

            self.main_plotter.iren.picker = PickerType.RENDERED
            self.main_plotter.iren.add_pick_observer(_end_pick_helper)
            self.main_plotter._picker_in_use = True

    def show_axes(self) -> None:
        """Show axes on the main plotter"""
        if not self.main_plotter.off_screen:
            self.main_plotter.show_axes()

    def set_camera_focal_point(self, focal_point: np.ndarray) -> None:
        """Set the camera focal point on the main plotter"""
        self.main_plotter.camera.focal_point = focal_point

    def show_plot_for_detectors(self, workspace: Workspace2D) -> None:
        """Plot all the given spectra, where they are defined by their workspace indices, not the spectra numbers"""
        self._detector_spectrum_axes.clear()
        sum_spectra = self.sum_spectra_selected()
        if workspace is not None and workspace.getNumberHistograms() > 0:
            spectra = workspace.getSpectrumNumbers()
            for spec in spectra:
                self._detector_spectrum_axes.plot(workspace, specNum=spec, label=f"Spectrum {spec}" if not sum_spectra else None)
            if not sum_spectra:
                self._detector_spectrum_axes.legend(fontsize=8.0).set_draggable(True)
            for line in self._lineplot_overlays:
                self._detector_spectrum_axes.add_line(line)
            integration_limits = self.get_integration_limits()
            self._detector_spectrum_axes.set_xlim(integration_limits[0], integration_limits[1])

        self.redraw_lineplot()

    def set_selected_detector_info(self, detector_infos: list[DetectorInfo]) -> None:
        """For a list of detectors, with their info wrapped up in a class, update all of the info text boxes"""
        self._set_detector_edit_text(self._detector_name_edit, detector_infos, lambda d: d.name)
        self._set_detector_edit_text(self._detector_id_edit, detector_infos, lambda d: str(d.detector_id))
        self._set_detector_edit_text(self._detector_workspace_index_edit, detector_infos, lambda d: str(d.workspace_index))
        self._set_detector_edit_text(self._detector_component_path_edit, detector_infos, lambda d: d.component_path)
        self._set_detector_edit_text(
            self._detector_xyz_edit,
            detector_infos,
            lambda d: f"x: {d.xyz_position[0]:.3f}, y: {d.xyz_position[1]:.3f}, z: {d.xyz_position[2]:.3f}",
        )
        self._set_detector_edit_text(
            self._detector_spherical_position_edit,
            detector_infos,
            lambda d: f"r: {d.spherical_position[0]:.3f}, 2\u03b8: {d.spherical_position[1]:.1f}, \u03c6: {d.spherical_position[2]:.1f}",
        )
        self._set_detector_edit_text(self._detector_pixel_counts_edit, detector_infos, lambda d: str(d.pixel_counts))

    def set_relative_detector_angle(self, angle: float | None) -> None:
        angle_text = "Select exactly two detectors to show angle" if angle is None else f"{angle:.4f}"
        self._detector_relative_angle_edit.setPlainText(angle_text)

    def _set_detector_edit_text(
        self, edit_box: QTextEdit, detector_infos: list[DetectorInfo], property_lambda: Callable[[DetectorInfo], str]
    ) -> None:
        """Set the text in one of the detector info boxes"""
        edit_box.setPlainText(",".join(property_lambda(d) for d in detector_infos))

    def selected_peaks_workspaces(self) -> list[str]:
        return [
            self._peak_ws_list.item(row_index).text()
            for row_index in range(self._peak_ws_list.count())
            if self._peak_ws_list.item(row_index).checkState() > 0
        ]

    def clear_overlay_meshes(self) -> None:
        for mesh in self._overlay_meshes:
            self.main_plotter.remove_actor(mesh[0])
            self.main_plotter.remove_actor(mesh[1])
        self._overlay_meshes.clear()

    def clear_lineplot_overlays(self) -> None:
        for line in self._lineplot_overlays:
            line.remove()
        self._lineplot_overlays.clear()
        for text in self._detector_spectrum_axes.texts:
            text.remove()

    def plot_overlay_mesh(self, positions: np.ndarray, labels: list[str], colour: str) -> None:
        points_actor = self.main_plotter.add_points(positions, color=colour, point_size=15, render_points_as_spheres=True, opacity=0.2)
        labels_actor = self.main_plotter.add_point_labels(
            positions,
            labels,
            font_size=15,
            font_family="times",
            show_points=False,
            always_visible=True,
            fill_shape=False,
            shape_opacity=0,
            text_color=colour,
        )
        self._overlay_meshes.append((points_actor, labels_actor))

    def plot_lineplot_overlay(self, x_values: list[float], labels: list[str], colour: str) -> None:
        for x, label in zip(x_values, labels):
            self._lineplot_overlays.append(self._detector_spectrum_axes.axvline(x, color=colour, linestyle="--"))
            self._detector_spectrum_axes.text(
                x,
                0.99,
                label,
                transform=self._detector_spectrum_axes.get_xaxis_transform(),
                color=colour,
                ha="right",
                va="top",
                fontsize=8,
                rotation=90,
            )
        self.redraw_lineplot()

    def redraw_lineplot(self) -> None:
        self._detector_spectrum_fig.tight_layout()
        self._detector_figure_canvas.draw()

    def get_current_selected_tab(self) -> CurrentTab:
        index = self._picking_masking_tab.currentIndex()
        tab_name = self._picking_masking_tab.tabText(index)
        return CurrentTab(tab_name)

    def get_current_selected_list_widget(self) -> QListWidget:
        return self._mask_list if self.get_current_selected_tab() == CurrentTab.Masking else self._selection_list

    def selected_items_in_list(self, kind: CurrentTab) -> list[str]:
        list_to_read_from = self._mask_list if kind is CurrentTab.Masking else self._selection_list
        return [
            list_to_read_from.item(row_index).text()
            for row_index in range(list_to_read_from.count())
            if list_to_read_from.item(row_index).checkState() > 0
        ]

    def set_new_item_key(self, kind: CurrentTab, new_key: str) -> None:
        list_to_modify = self._mask_list if kind is CurrentTab.Masking else self._selection_list
        list_item = QListWidgetItem(new_key, list_to_modify)
        list_item.setCheckState(Qt.Checked)

    def clear_item_list(self, kind: CurrentTab):
        list_to_clear = self._mask_list if kind is CurrentTab.Masking else self._selection_list
        # Iterate backwards otherwise breaks indexing
        for i in range(list_to_clear.count() - 1, -1, -1):
            item = list_to_clear.item(i)
            if item.text() not in self._presenter.cached_keys(kind):
                # Skip items that are workspaces
                continue
            removed = list_to_clear.takeItem(i)
            del removed

    def has_any_peak_overlays(self) -> bool:
        return len(self._lineplot_overlays) > 0

    def _on_axes_click(self, event) -> None:
        self._plot_toolbar.setDisabled(False)
        if event.inaxes is not self._detector_spectrum_axes or event.xdata is None:
            return
        self._presenter.on_peak_selected(event.xdata)

    def add_peak_cursor_to_lineplot(self) -> None:
        self._lineplot_peak_cursor = Cursor(self._detector_spectrum_axes, color="tab:red", linewidth=1, horizOn=False)
        self._figure_canvas_click_id = self._detector_figure_canvas.mpl_connect("button_press_event", self._on_axes_click)
        self._plot_toolbar.setDisabled(True)

    def remove_peak_cursor_from_lineplot(self) -> None:
        if self._lineplot_peak_cursor is not None:
            self._detector_figure_canvas.mpl_disconnect(self._figure_canvas_click_id)
            self._figure_canvas_click_id = None
            self._lineplot_peak_cursor.linev.remove()
            self._lineplot_peak_cursor = None
            self._detector_figure_canvas.draw_idle()
