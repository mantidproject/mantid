# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
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
)
from qtpy.QtGui import QDoubleValidator, QMovie, QDragEnterEvent, QDropEvent, QDragMoveEvent, QColor, QPalette
from qtpy.QtCore import Qt, QEvent, QSize
from superqt import QDoubleRangeSlider
from pyvistaqt import BackgroundPlotter
import matplotlib.pyplot as plt
from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg as FigureCanvas
from instrumentview.Detectors import DetectorInfo
from instrumentview.InteractorStyles import CustomInteractorStyleZoomAndSelect, CustomInteractorStyleRubberBand3D
from typing import Callable
from mantid.dataobjects import Workspace2D
from mantid import UsageService
from mantid.kernel import FeatureType
from mantidqt.plotting.mantid_navigation_toolbar import MantidNavigationToolbar
import numpy as np
import pyvista as pv
from pyvista.plotting.picking import RectangleSelection
from pyvista.plotting.opts import PickerType
import os


class PeaksWorkspaceListWidget(QListWidget):
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


class FullInstrumentViewWindow(QMainWindow):
    """View for the Instrument View window. Contains the 3D view, the projection view, boxes showing information about the selected
    detector, and a line plot of selected detector(s)"""

    _detector_spectrum_fig = None

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
        parent_horizontal_layout = QHBoxLayout(central_widget)

        pyvista_vertical_widget = QWidget()
        left_column_widget = QWidget()
        pyvista_vertical_layout = QVBoxLayout(pyvista_vertical_widget)
        left_column_layout = QVBoxLayout(left_column_widget)

        left_column_scroll = QScrollArea()
        left_column_scroll.setWidgetResizable(True)
        left_column_scroll.setWidget(left_column_widget)

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
        plot_toolbar = MantidNavigationToolbar(self._detector_figure_canvas, self)
        plot_widget = QWidget()
        plot_layout = QVBoxLayout(plot_widget)
        plot_layout.addWidget(self._detector_figure_canvas)
        plot_layout.addWidget(plot_toolbar)

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
        self._integration_limit_min_edit, self._integration_limit_max_edit, self._integration_limit_slider = self._add_min_max_group_box(
            self._integration_limit_group_box
        )
        self._contour_range_group_box = QGroupBox("Contour Range")
        self._contour_range_min_edit, self._contour_range_max_edit, self._contour_range_slider = self._add_min_max_group_box(
            self._contour_range_group_box
        )

        multi_select_group_box = QGroupBox("Multi-Select")
        multi_select_layout = QHBoxLayout(multi_select_group_box)
        self._multi_select_check = QCheckBox()
        self._multi_select_check.setText("Rectangular Select")
        self._multi_select_check.setToolTip("Currently only working on 3D view.")
        self._clear_selection_button = QPushButton("Clear Selection")
        self._clear_selection_button.setToolTip("Clear the current selection of detectors")
        multi_select_layout.addWidget(self._multi_select_check)
        multi_select_layout.addWidget(self._clear_selection_button)

        projection_group_box = QGroupBox("Projection")
        projection_layout = QHBoxLayout(projection_group_box)
        self._projection_combo_box = QComboBox(self)
        self._reset_projection = QPushButton("Reset Projection")
        self._reset_projection.setToolTip("Resets the projection to default.")
        self._reset_projection.clicked.connect(self.reset_camera)
        projection_layout.addWidget(self._projection_combo_box)
        projection_layout.addWidget(self._reset_projection)

        peak_ws_group_box = QGroupBox("Peaks Workspaces")
        peak_v_layout = QVBoxLayout(peak_ws_group_box)
        self._peak_ws_list = PeaksWorkspaceListWidget(self)
        self._peak_ws_list.setSizeAdjustPolicy(QListWidget.AdjustToContents)
        peak_v_layout.addWidget(self._peak_ws_list)

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
        options_vertical_layout.addWidget(multi_select_group_box)
        options_vertical_layout.addWidget(projection_group_box)
        units_group_box = QGroupBox("Units")
        units_vbox = QVBoxLayout()
        self._setup_units_options(units_vbox)
        units_group_box.setLayout(units_vbox)
        options_vertical_layout.addWidget(units_group_box)
        options_vertical_layout.addWidget(peak_ws_group_box)
        options_vertical_layout.addWidget(QSplitter(Qt.Horizontal))

        options_vertical_layout.addWidget(self.status_group_box)
        left_column_layout.addWidget(options_vertical_widget)
        left_column_layout.addStretch()

        self.interactor_style = CustomInteractorStyleZoomAndSelect()
        self._overlay_meshes = []
        self._lineplot_overlays = []

        screen_geometry = self.screen().geometry()
        self.resize(int(screen_geometry.width() * 0.8), int(screen_geometry.height() * 0.8))
        window_geometry = self.frameGeometry()
        center_point = screen_geometry.center()
        window_geometry.moveCenter(center_point)
        self.move(window_geometry.topLeft())

        UsageService.registerFeatureUsage(FeatureType.Interface, "InstrumentView2025", False)

    def check_sum_spectra_checkbox(self) -> None:
        self._sum_spectra_checkbox.setChecked(True)
        self._presenter.on_sum_spectra_checkbox_clicked()

    def is_multi_picking_checkbox_checked(self) -> bool:
        return self._multi_select_check.isChecked()

    def _on_splitter_moved(self, pos, index) -> None:
        self._detector_spectrum_fig.tight_layout()

    def hide_status_box(self) -> None:
        self.status_group_box.hide()

    def reset_camera(self) -> None:
        if not self._off_screen:
            self.main_plotter.reset_camera()

    def _add_min_max_group_box(self, parent_box: QGroupBox) -> tuple[QLineEdit, QLineEdit, QDoubleRangeSlider]:
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

        slider = QDoubleRangeSlider(Qt.Orientation.Horizontal, parent=parent_box)
        slider.setRange(0, 1)
        slider_hbox = QHBoxLayout()
        slider_hbox.addWidget(slider)

        root_hbox = QHBoxLayout()
        root_hbox.addLayout(min_hbox)
        root_hbox.addLayout(max_hbox)

        root_vbox = QVBoxLayout()
        root_vbox.addLayout(slider_hbox)
        root_vbox.addLayout(root_hbox)
        parent_box.setLayout(root_vbox)

        return (min_edit, max_edit, slider)

    def _add_connections_to_edits_and_slider(self, min_edit: QLineEdit, max_edit: QLineEdit, slider, presenter_callback: Callable):
        def format_float(value):
            return f"{value:.4f}".rstrip("0").rstrip(".")

        def set_edits(limits):
            min, max = limits
            min_edit.setText(format_float(min))
            max_edit.setText(format_float(max))

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
        slider.valueChanged.connect(set_edits)
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

    def subscribe_presenter(self, presenter) -> None:
        self._presenter = presenter
        for unit in self._presenter.available_unit_options():
            self._units_combo_box.addItem(unit)
        self._integration_limit_group_box.setTitle(self._presenter.workspace_display_unit)
        self.main_plotter.set_color_cycler(self._presenter._COLOURS)
        self.refresh_peaks_ws_list()

    def setup_connections_to_presenter(self) -> None:
        self._projection_combo_box.currentIndexChanged.connect(self._presenter.on_projection_option_selected)
        self._multi_select_check.stateChanged.connect(self._presenter.on_multi_select_detectors_clicked)
        self._clear_selection_button.clicked.connect(self._presenter.on_clear_selected_detectors_clicked)
        self._contour_range_slider.sliderReleased.connect(self._presenter.on_contour_limits_updated)
        self._integration_limit_slider.sliderReleased.connect(self._presenter.on_integration_limits_updated)
        self._units_combo_box.currentIndexChanged.connect(self._presenter.on_unit_option_selected)
        self._export_workspace_button.clicked.connect(self._presenter.on_export_workspace_clicked)
        self._sum_spectra_checkbox.clicked.connect(self._presenter.on_sum_spectra_checkbox_clicked)
        self._peak_ws_list.itemChanged.connect(self._presenter.on_peaks_workspace_selected)

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

    def _setup_units_options(self, parent: QVBoxLayout):
        """Add widgets for the units options"""
        self._units_combo_box = QComboBox(self)
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

    def refresh_peaks_ws_list_colours(self) -> None:
        picked_index = 0
        for list_i in range(self._peak_ws_list.count()):
            list_item = self._peak_ws_list.item(list_i)
            if list_item.checkState() > 0:
                list_item.setForeground(QColor(self._presenter._COLOURS[picked_index % len(self._presenter._COLOURS)]))
                picked_index += 1
            else:
                list_item.setForeground(self._peak_ws_list.palette().color(QPalette.Text))

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
        self.main_plotter.close()
        if self._detector_spectrum_fig is not None:
            plt.close(self._detector_spectrum_fig.get_label())
        self._presenter.handle_close()

    def set_projection_combo_options(self, default_index: int, options: list[str]) -> None:
        self._projection_combo_box.addItems(options)
        self._projection_combo_box.setCurrentIndex(default_index)

    def current_selected_projection(self) -> str:
        return self._projection_combo_box.currentText()

    def add_simple_shape(self, mesh: PolyData, colour=None, pickable=False) -> None:
        """Draw the given mesh in the main plotter window"""
        self.main_plotter.add_mesh(mesh, color=colour, pickable=pickable)

    def add_main_mesh(self, mesh: PolyData, is_projection: bool, scalars=None) -> None:
        """Draw the given mesh in the main plotter window"""
        self.main_plotter.clear()
        scalar_bar_args = dict(interactive=True, vertical=False, title_font_size=15, label_font_size=12) if scalars is not None else None
        self.main_plotter.add_mesh(
            mesh, pickable=False, scalars=scalars, render_points_as_spheres=True, point_size=15, scalar_bar_args=scalar_bar_args
        )

        if not self.main_plotter.off_screen:
            self.main_plotter.enable_trackball_style()

        if is_projection:
            self.main_plotter.view_xy()
            if not self.main_plotter.off_screen:
                self.main_plotter.enable_zoom_style()

    def add_pickable_main_mesh(self, point_cloud: PolyData, scalars: np.ndarray | str) -> None:
        self.main_plotter.add_mesh(
            point_cloud,
            scalars=scalars,
            opacity=[0.0, 0.3],
            show_scalar_bar=False,
            pickable=True,
            cmap="Oranges",
            point_size=30,
            render_points_as_spheres=True,
        )

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
