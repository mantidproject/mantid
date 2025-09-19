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
)
from qtpy.QtGui import QPalette, QIntValidator, QMovie
from qtpy.QtCore import Qt, QEvent, QSize
from superqt import QDoubleRangeSlider
from pyvistaqt import BackgroundPlotter
import matplotlib.pyplot as plt
from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg as FigureCanvas
from instrumentview.Detectors import DetectorInfo
from typing import Callable
from mantid.dataobjects import Workspace2D
import numpy as np
import pyvista as pv
import os


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
        self.resize(1500, 1500)

        self._off_screen = off_screen

        central_widget = QWidget(self)
        self.setCentralWidget(central_widget)
        parent_horizontal_layout = QHBoxLayout(central_widget)

        pyvista_vertical_widget = QWidget()
        left_column_widget = QWidget()
        pyvista_vertical_layout = QVBoxLayout(pyvista_vertical_widget)
        left_column_layout = QVBoxLayout(left_column_widget)

        hsplitter = QSplitter(Qt.Horizontal)
        hsplitter.addWidget(left_column_widget)
        hsplitter.addWidget(pyvista_vertical_widget)
        hsplitter.setSizes([100, 200])
        parent_horizontal_layout.addWidget(hsplitter)

        self.main_plotter = BackgroundPlotter(show=False, menu_bar=False, toolbar=False, off_screen=off_screen)
        pyvista_vertical_layout.addWidget(self.main_plotter.app_window)

        self._detector_spectrum_fig, self._detector_spectrum_axes = plt.subplots(subplot_kw={"projection": "mantid"})
        self._detector_figure_canvas = FigureCanvas(self._detector_spectrum_fig)
        self._detector_figure_canvas.setMinimumSize(QSize(0, 0))
        plot_widget = QWidget()
        plot_layout = QVBoxLayout(plot_widget)
        plot_layout.addWidget(self._detector_figure_canvas)

        vsplitter = QSplitter(Qt.Vertical)
        vsplitter.addWidget(self.main_plotter.app_window)
        vsplitter.addWidget(plot_widget)
        vsplitter.setStretchFactor(0, 0)
        vsplitter.setStretchFactor(1, 1)

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

        self._time_of_flight_group_box = QGroupBox("Time of Flight")
        self._tof_min_edit, self._tof_max_edit, self._tof_slider = self._add_min_max_group_box(self._time_of_flight_group_box)
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
        options_vertical_layout.addWidget(self._time_of_flight_group_box)
        options_vertical_layout.addWidget(self._contour_range_group_box)
        options_vertical_layout.addWidget(multi_select_group_box)
        options_vertical_layout.addWidget(projection_group_box)
        options_vertical_layout.addWidget(self.status_group_box)
        left_column_layout.addWidget(options_vertical_widget)
        left_column_layout.addStretch()

    def disable_rectangle_picking_checkbox(self) -> None:
        self._multi_select_check.setChecked(False)
        self._multi_select_check.setEnabled(False)

    def enable_rectangle_picking_checkbox(self) -> None:
        self._multi_select_check.setChecked(False)
        self._multi_select_check.setEnabled(True)

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
        max_int_32 = np.iinfo(np.int32).max
        min_edit.setValidator(QIntValidator(0, max_int_32, self))
        min_hbox.addWidget(min_edit)
        max_hbox = QHBoxLayout()
        max_hbox.addWidget(QLabel("Max"))
        max_edit = QLineEdit()
        max_edit.setValidator(QIntValidator(0, max_int_32, self))
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
        def set_edits(limits):
            min, max = limits
            min_edit.setText(f"{min:.0f}")
            max_edit.setText(f"{max:.0f}")

        def set_slider(callled_from_min):
            def wrapped():
                try:
                    min, max = int(float(min_edit.text())), int(float(max_edit.text()))
                except ValueError:
                    return
                if callled_from_min:
                    min = max if min > max else min
                else:
                    max = min if max < min else max
                slider.setValue((min, max))
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

    def setup_connections_to_presenter(self) -> None:
        self._projection_combo_box.currentIndexChanged.connect(self._presenter.on_projection_option_selected)
        self._multi_select_check.stateChanged.connect(self._presenter.on_multi_select_detectors_clicked)
        self._multi_select_check.clicked.connect(self._cleanup_rectangle_picking)
        self._clear_selection_button.clicked.connect(self._presenter.on_clear_selected_detectors_clicked)
        self._contour_range_slider.sliderReleased.connect(self._presenter.on_contour_limits_updated)
        self._tof_slider.sliderReleased.connect(self._presenter.on_tof_limits_updated)

        self._add_connections_to_edits_and_slider(
            self._contour_range_min_edit,
            self._contour_range_max_edit,
            self._contour_range_slider,
            self._presenter.on_contour_limits_updated,
        )
        self._add_connections_to_edits_and_slider(
            self._tof_min_edit, self._tof_max_edit, self._tof_slider, self._presenter.on_tof_limits_updated
        )

    def _cleanup_rectangle_picking(self, is_clicked: bool) -> None:
        # TODO: Fix this workaround for reseting rectangle picking
        # Issue happens because if user is in selecting more (pressed 'r') and unticks box
        # means that some interactor events like mouse presses will be stuck in selecting mode
        if is_clicked:
            return
        self.main_plotter.disable_picking()
        self.main_plotter.enable_rectangle_picking(callback=lambda *_: None, show_message=False)
        self.main_plotter.disable_picking()

    def get_tof_limits(self) -> tuple[float, float]:
        return self._tof_slider.value()

    def set_tof_range_limits(self, tof_limits: tuple[float, float]) -> None:
        """Update the TOF edit boxes with formatted text"""
        lower, upper = tof_limits
        if upper <= lower:
            self._time_of_flight_group_box.hide()
            return
        self._tof_slider.setRange(*tof_limits)
        self._tof_slider.setValue(tof_limits)
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

    def add_simple_shape(self, mesh: PolyData, colour=None, pickable=False) -> None:
        """Draw the given mesh in the main plotter window"""
        self.main_plotter.add_mesh(mesh, color=colour, pickable=pickable)

    def add_main_mesh(self, mesh: PolyData, is_projection: bool, scalars=None) -> None:
        """Draw the given mesh in the main plotter window"""
        self.main_plotter.clear()
        scalar_bar_args = dict(interactive=True, vertical=True, title_font_size=15, label_font_size=12) if scalars is not None else None
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
            opacity=[0.0, 0.5],
            show_scalar_bar=False,
            pickable=True,
            cmap="Oranges",
            point_size=30,
            render_points_as_spheres=True,
        )

    def add_rgba_mesh(self, mesh: PolyData, scalars: np.ndarray | str):
        """Draw the given mesh in the main plotter window, and set the colours manually with RGBA numbers"""
        self.main_plotter.add_mesh(mesh, scalars=scalars, rgba=True, pickable=False, render_points_as_spheres=True, point_size=10)

    def enable_point_picking(self, callback: Callable) -> None:
        """Switch on point picking, i.e. picking a single point with right-click"""
        self.main_plotter.disable_picking()
        picking_tolerance = 0.01
        if not self.main_plotter.off_screen:
            self.main_plotter.enable_surface_point_picking(
                show_message=False,
                use_picker=True,
                callback=callback,
                show_point=False,
                pickable_window=False,
                picker="point",
                tolerance=picking_tolerance,
            )

    def enable_rectangle_picking(self, callback: Callable) -> None:
        """Switch on rectangle picking, i.e. draw a rectangle to select all detectors within the rectangle"""
        self.main_plotter.disable_picking()
        if not self.main_plotter.off_screen:
            self.main_plotter.enable_rectangle_picking(callback=callback, use_picker=callback is not None, font_size=12)

    def show_axes(self) -> None:
        """Show axes on the main plotter"""
        if not self.main_plotter.off_screen:
            self.main_plotter.show_axes()

    def set_camera_focal_point(self, focal_point: np.ndarray) -> None:
        """Set the camera focal point on the main plotter"""
        self.main_plotter.camera.focal_point = focal_point

    def set_plot_for_detectors(self, workspace: Workspace2D, workspace_indices: list | np.ndarray) -> None:
        """Plot all the given spectra, where they are defined by their workspace indices, not the spectra numbers"""
        self._detector_spectrum_axes.clear()
        for d in workspace_indices:
            self._detector_spectrum_axes.plot(workspace, label=workspace.name() + "Workspace Index " + str(d), wkspIndex=int(d))

        if len(workspace_indices) > 0:
            self._detector_spectrum_axes.legend(fontsize=8.0).set_draggable(True)

        self._detector_figure_canvas.draw()

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
            lambda d: f"r: {d.spherical_position[0]:.3f}, t: {d.spherical_position[1]:.1f}, p: {d.spherical_position[2]:.1f}",
        )
        self._set_detector_edit_text(self._detector_pixel_counts_edit, detector_infos, lambda d: str(d.pixel_counts))

    def _set_detector_edit_text(
        self, edit_box: QTextEdit, detector_infos: list[DetectorInfo], property_lambda: Callable[[DetectorInfo], str]
    ) -> None:
        """Set the text in one of the detector info boxes"""
        edit_box.setPlainText(",".join(property_lambda(d) for d in detector_infos))
