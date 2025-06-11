# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
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
)
from qtpy.QtGui import QPalette, QIntValidator
from qtpy.QtCore import Qt
from pyvistaqt import BackgroundPlotter
import matplotlib.pyplot as plt
from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg as FigureCanvas
from instrumentview.FullInstrumentViewPresenter import FullInstrumentViewPresenter
from instrumentview.DetectorInfo import DetectorInfo
from typing import Callable
import numpy as np


class FullInstrumentViewWindow(QMainWindow):
    """View for the Instrument View window. Contains the 3D view, the projection view, boxes showing information about the selected
    detector, and a line plot of selected detector(s)"""

    _detector_spectrum_fig = None

    def __init__(self, workspace, parent=None, off_screen=False):
        """The instrument in the given workspace will be displayed. The off_screen option is for testing or rendering an image
        e.g. in a script."""
        super(FullInstrumentViewWindow, self).__init__(parent)
        self.setWindowTitle("Instrument View")

        central_widget = QWidget(self)
        self.setCentralWidget(central_widget)
        parent_horizontal_layout = QHBoxLayout(central_widget)
        pyvista_vertical_layout = QVBoxLayout()
        options_vertical_layout = QVBoxLayout()
        parent_horizontal_layout.addLayout(options_vertical_layout, 1)
        parent_horizontal_layout.addLayout(pyvista_vertical_layout, 3)

        self.main_plotter = BackgroundPlotter(show=False, menu_bar=False, toolbar=False, off_screen=off_screen)
        pyvista_vertical_layout.addWidget(self.main_plotter.app_window)
        self.projection_plotter = BackgroundPlotter(show=False, menu_bar=False, toolbar=False, off_screen=off_screen)
        pyvista_vertical_layout.addWidget(self.projection_plotter.app_window)

        detector_group_box = QGroupBox("Detector Info")
        detector_vbox = QVBoxLayout()
        self._detector_name_edit = self._add_detector_info_boxes(detector_vbox, "Name")
        self._detector_id_edit = self._add_detector_info_boxes(detector_vbox, "Detector ID")
        self._detector_workspace_index_edit = self._add_detector_info_boxes(detector_vbox, "Workspace Index")
        self._detector_component_path_edit = self._add_detector_info_boxes(detector_vbox, "Component Path")
        self._detector_xyz_edit = self._add_detector_info_boxes(detector_vbox, "XYZ Position")
        self._detector_spherical_position_edit = self._add_detector_info_boxes(detector_vbox, "Spherical Position")
        self._detector_pixel_counts_edit = self._add_detector_info_boxes(detector_vbox, "Pixel Counts")
        detector_group_box.setLayout(detector_vbox)

        time_of_flight_group_box = QGroupBox("Time of Flight")
        self._tof_min_edit, self._tof_max_edit = self._add_min_max_group_box(
            time_of_flight_group_box, self._on_tof_limits_updated, self._on_tof_limits_updated
        )
        contour_range_group_box = QGroupBox("Contour Range")
        self._contour_range_min_edit, self._contour_range_max_edit = self._add_min_max_group_box(
            contour_range_group_box, self._on_contour_limits_updated, self._on_contour_limits_updated
        )

        multi_select_group_box = QGroupBox("Multi-Select")
        multi_select_h_layout = QHBoxLayout()
        self._multi_Select_Check = QCheckBox()
        self._multi_Select_Check.setText("Select multiple detectors")
        self._multi_Select_Check.stateChanged.connect(self._on_multi_select_check_box_clicked)
        multi_select_h_layout.addWidget(self._multi_Select_Check)
        multi_select_group_box.setLayout(multi_select_h_layout)

        options_vertical_layout.addWidget(detector_group_box)
        options_vertical_layout.addWidget(time_of_flight_group_box)
        options_vertical_layout.addWidget(contour_range_group_box)
        options_vertical_layout.addWidget(multi_select_group_box)

        self._presenter = FullInstrumentViewPresenter(self, workspace)

        projection_group_box = QGroupBox("Projection")
        projection_vbox = QVBoxLayout()
        self._setup_projection_options(projection_vbox)
        projection_group_box.setLayout(projection_vbox)
        options_vertical_layout.addWidget(projection_group_box)

        options_vertical_layout.addWidget(QSplitter(Qt.Horizontal))
        self._detector_spectrum_fig, self._detector_spectrum_axes = plt.subplots(subplot_kw={"projection": "mantid"})
        self._detector_figure_canvas = FigureCanvas(self._detector_spectrum_fig)
        options_vertical_layout.addWidget(self._detector_figure_canvas)

        options_vertical_layout.addStretch()
        central_widget.setLayout(parent_horizontal_layout)
        self.resize(1300, 1000)
        self.main_plotter.reset_camera()
        self.projection_plotter.reset_camera()

    def _add_min_max_group_box(self, parent_box: QGroupBox, min_callback, max_callback) -> tuple[QLineEdit, QLineEdit]:
        """Creates a minimum and a maximum box (with labels) inside the given group box. The callbacks will be attached to textEdited
        signal of the boxes"""
        root_vbox = QVBoxLayout()
        min_hbox = QHBoxLayout()
        min_hbox.addWidget(QLabel("Min"))
        min_edit = QLineEdit()
        min_edit.textEdited.connect(min_callback)
        max_int_32 = np.iinfo(np.int32).max
        min_edit.setValidator(QIntValidator(0, max_int_32, self))
        min_hbox.addWidget(min_edit)
        max_hbox = QHBoxLayout()
        max_hbox.addWidget(QLabel("Max"))
        max_edit = QLineEdit()
        max_edit.textEdited.connect(max_callback)
        max_edit.setValidator(QIntValidator(0, max_int_32, self))
        max_hbox.addWidget(max_edit)
        root_vbox.addLayout(min_hbox)
        root_vbox.addLayout(max_hbox)
        parent_box.setLayout(root_vbox)
        return (min_edit, max_edit)

    def _add_detector_info_boxes(self, parent_box: QVBoxLayout, label: str) -> QHBoxLayout:
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

    def _setup_projection_options(self, parent: QVBoxLayout):
        """Add widgets for the projection options"""
        projection_combo_box = QComboBox(self)
        projection_combo_box.addItems(self._presenter.projection_combo_options())
        projection_combo_box.currentIndexChanged.connect(self._on_projection_combo_box_changed)
        parent.addWidget(projection_combo_box)

    def _on_projection_combo_box_changed(self, value):
        """If the projection type is changed, then tell the presenter to do something"""
        if type(value) is int:
            self._presenter.projection_option_selected(value)

    def _on_multi_select_check_box_clicked(self, state):
        """Tell the presenter if either single or multi select is enabled"""
        self._presenter.set_multi_select_enabled(state == 2)

    def set_contour_range_limits(self, contour_limits: list) -> None:
        """Update the contour range edit boxes with formatted text"""
        self._contour_range_min_edit.setText(f"{contour_limits[0]:.0f}")
        self._contour_range_max_edit.setText(f"{contour_limits[1]:.0f}")

    def set_tof_range_limits(self, tof_limits: list) -> None:
        """Update the TOF edit boxes with formatted text"""
        self._tof_min_edit.setText(f"{tof_limits[0]:.0f}")
        self._tof_max_edit.setText(f"{tof_limits[1]:.0f}")

    def _on_tof_limits_updated(self, text):
        """When TOF limits are changed, read the new limits and tell the presenter to update the colours accordingly"""
        is_valid, min, max = self._parse_min_max_text(text, self._tof_min_edit, self._tof_max_edit)
        if is_valid:
            self._presenter.set_tof_limits(min, max)

    def _on_contour_limits_updated(self, text):
        """When contour limits are changed, read the new limits and tell the presenter to update the colours accordingly"""
        is_valid, min, max = self._parse_min_max_text(text, self._contour_range_min_edit, self._contour_range_max_edit)
        if is_valid:
            self._presenter.set_contour_limits(min, max)

    def _parse_min_max_text(self, text, min_edit, max_edit) -> tuple[bool, int, int]:
        """Try to parse the text in the edit boxes as numbers. Return the results and whether the attempt was successful."""
        if text is None:
            return (False, 0, 0)
        try:
            min = int(min_edit.text())
            max = int(max_edit.text())
        except ValueError:
            return (False, 0, 0)
        if max <= min:
            return (False, min, max)
        return (True, min, max)

    def update_scalar_range(self, clim, label: str) -> None:
        """Set the range of the colours displayed, i.e. the legend"""
        self.main_plotter.update_scalar_bar_range(clim, label)
        self.projection_plotter.update_scalar_bar_range(clim, label)

    def closeEvent(self, QCloseEvent):
        """When closing, make sure to close the plotters and figure correctly to prevent errors"""
        super().closeEvent(QCloseEvent)
        self.main_plotter.close()
        self.projection_plotter.close()
        if self._detector_spectrum_fig is not None:
            plt.close(self._detector_spectrum_fig.get_label())

    def add_simple_shape(self, mesh, colour=None, pickable=False):
        """Draw the given mesh in the main plotter window"""
        self.main_plotter.add_mesh(mesh, color=colour, pickable=pickable)

    def add_mesh(self, mesh, pickable=False, scalars=None, clim=None) -> None:
        """Draw the given mesh in the main plotter window"""
        self.main_plotter.add_mesh(mesh, pickable=pickable, scalars=scalars, clim=clim, render_points_as_spheres=True, point_size=7)

    def add_picked_mesh(self, point_cloud, scalars, pickable=True) -> None:
        self.main_plotter.add_mesh(
            point_cloud,
            scalars=scalars,
            opacity=[0.0, 0.5],
            show_scalar_bar=False,
            pickable=pickable,
            color="red",
            point_size=20,
            render_points_as_spheres=True,
        )

    def add_rgba_mesh(self, mesh, scalars):
        """Draw the given mesh in the main plotter window, and set the colours manually with RGBA numbers"""
        self.main_plotter.add_mesh(mesh, scalars=scalars, rgba=True, render_points_as_spheres=True, point_size=10)

    def enable_point_picking(self, callback=None) -> None:
        """Switch on point picking, i.e. picking a single point with right-click"""
        self.main_plotter.disable_picking()
        if not self.main_plotter.off_screen:
            self.main_plotter.enable_point_picking(show_message=False, callback=callback, use_picker=callback is not None)

        self.projection_plotter.disable_picking()
        if not self.projection_plotter.off_screen:
            self.projection_plotter.enable_point_picking(show_message=False, callback=callback, use_picker=callback is not None)

    def enable_rectangle_picking(self, callback=None) -> None:
        """Switch on rectangle picking, i.e. draw a rectangle to select all detectors within the rectangle"""
        self.main_plotter.disable_picking()
        if not self.main_plotter.off_screen:
            self.main_plotter.enable_rectangle_picking(callback=callback, use_picker=callback is not None, font_size=12)

    def add_projection_mesh(self, mesh, scalars=None, clim=None, pickable=False) -> None:
        """Draw the given mesh in the projection plotter. This is a 2D plot so we set options accordingly on the plotter"""
        self.projection_plotter.clear()
        self.projection_plotter.add_mesh(mesh, scalars=scalars, clim=clim, render_points_as_spheres=True, point_size=7, pickable=pickable)
        self.projection_plotter.view_xy()
        if not self.projection_plotter.off_screen:
            self.projection_plotter.enable_image_style()

    def add_pickable_projection_mesh(self, mesh, scalars=None, pickable=True) -> None:
        """Draw the given mesh in the projection plotter. This is a 2D plot so we set options accordingly on the plotter"""
        self.projection_plotter.add_mesh(
            mesh,
            scalars=scalars,
            opacity=[0.0, 0.5],
            show_scalar_bar=False,
            pickable=pickable,
            color="red",
            point_size=20,
            render_points_as_spheres=True,
        )
        self.projection_plotter.view_xy()
        if not self.projection_plotter.off_screen:
            self.projection_plotter.enable_image_style()

    def show_axes(self) -> None:
        """Show axes on the main plotter"""
        if not self.main_plotter.off_screen:
            self.main_plotter.show_axes()

    def set_camera_focal_point(self, focal_point) -> None:
        """Set the camera focal point on the main plotter"""
        self.main_plotter.camera.focal_point = focal_point

    def show_plot_for_detectors(self, workspace, workspace_indices: list) -> None:
        """Plot all the given spectra, where they are defined by their workspace indices, not the spectra numbers"""
        self._detector_spectrum_axes.clear()

        for d in workspace_indices:
            self._detector_spectrum_axes.plot(workspace, label=workspace.name() + "Workspace Index " + str(d), wkspIndex=d)

        self._detector_spectrum_axes.legend(fontsize=8.0).set_draggable(True)
        self._detector_figure_canvas.draw()

    def update_selected_detector_info(self, detector_infos: list[DetectorInfo]) -> None:
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
