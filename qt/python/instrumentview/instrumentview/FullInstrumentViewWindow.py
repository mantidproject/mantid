# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy.QtWidgets import QMainWindow, QVBoxLayout, QHBoxLayout, QWidget, QPushButton, QLabel, QLineEdit, QGroupBox, QSizePolicy
from qtpy.QtGui import QPalette
from pyvistaqt import BackgroundPlotter
import matplotlib.pyplot as plt
from instrumentview.FullInstrumentViewPresenter import FullInstrumentViewPresenter
from instrumentview.DetectorInfo import DetectorInfo
from typing import Callable


class FullWindow(QMainWindow):
    _detector_spectrum_fig = None

    def __init__(self, workspace, parent=None):
        super(FullWindow, self).__init__(parent)
        self.setWindowTitle("Instrument View")

        central_widget = QWidget(self)
        self.setCentralWidget(central_widget)
        parent_horizontal_layout = QHBoxLayout(central_widget)
        pyvista_vertical_layout = QVBoxLayout()
        options_vertical_layout = QVBoxLayout()
        parent_horizontal_layout.addLayout(options_vertical_layout, 1)
        parent_horizontal_layout.addLayout(pyvista_vertical_layout, 3)

        self.main_plotter = BackgroundPlotter(show=False, menu_bar=False, toolbar=False)
        pyvista_vertical_layout.addWidget(self.main_plotter.app_window)
        self.projection_plotter = BackgroundPlotter(show=False, menu_bar=False, toolbar=False)
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

        button2 = QPushButton("Option 2")
        options_vertical_layout.addWidget(detector_group_box)
        options_vertical_layout.addWidget(QLabel("Test"))
        options_vertical_layout.addWidget(button2)
        options_vertical_layout.addStretch()

        central_widget.setLayout(parent_horizontal_layout)

        self._presenter = FullInstrumentViewPresenter(self, workspace)

        self.resize(1300, 1000)

        self.main_plotter.reset_camera()
        self.projection_plotter.reset_camera()

    def _add_detector_info_boxes(self, parent_box: QVBoxLayout, label: str) -> QHBoxLayout:
        hbox = QHBoxLayout()
        hbox.addWidget(QLabel(label))
        line_edit = QLineEdit()
        line_edit.setReadOnly(True)
        line_edit.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Fixed)
        hbox.addWidget(line_edit)
        parent_box.addLayout(hbox)
        palette = line_edit.palette()
        palette.setColor(QPalette.Base, palette.color(QPalette.Window))
        line_edit.setPalette(palette)
        return line_edit

    def closeEvent(self, QCloseEvent):
        super().closeEvent(QCloseEvent)
        self.main_plotter.close()
        self.projection_plotter.close()
        if self._detector_spectrum_fig is not None:
            plt.close(self._detector_spectrum_fig.get_label())

    def add_simple_shape(self, mesh, colour=None, pickable=False):
        self.main_plotter.add_mesh(mesh, color=colour, pickable=pickable)

    def add_mesh(self, mesh, pickable=False, scalars=None, clim=None) -> None:
        self.main_plotter.add_mesh(mesh, pickable=pickable, scalars=scalars, clim=clim, render_points_as_spheres=True, point_size=7)

    def add_rgba_mesh(self, mesh, scalars):
        self.main_plotter.add_mesh(mesh, scalars=scalars, rgba=True, render_points_as_spheres=True, point_size=10)

    def enable_point_picking(self, callback=None) -> None:
        self.main_plotter.enable_point_picking(show_message=False, callback=callback, use_picker=callback is not None)

    def add_projection_mesh(self, mesh, scalars=None, clim=None) -> None:
        self.projection_plotter.add_mesh(mesh, scalars=scalars, clim=clim, render_points_as_spheres=True, point_size=7)
        self.projection_plotter.view_xy()
        self.projection_plotter.enable_image_style()

    def show_axes(self) -> None:
        self.main_plotter.show_axes()

    def set_camera_focal_point(self, focal_point) -> None:
        self.main_plotter.camera.focal_point = focal_point

    def show_plot_for_detectors(self, workspace, workspace_indices: list) -> None:
        if self._detector_spectrum_fig is None:
            self._detector_spectrum_fig, self._detector_spectrum_axes = plt.subplots(subplot_kw={"projection": "mantid"})
            self._detector_spectrum_fig.canvas.mpl_connect("close_event", self.on_figure_close)

        self._detector_spectrum_axes.clear()

        for d in workspace_indices:
            self._detector_spectrum_axes.plot(workspace, label=workspace.name() + "Workspace Index " + str(d), wkspIndex=d)

        self._detector_spectrum_axes.legend(fontsize=8.0).set_draggable(True)
        self._detector_spectrum_fig.show()
        plt.draw()

    def on_figure_close(self, event) -> None:
        self._detector_spectrum_fig = None
        self._detector_spectrum_axes = None

    def update_selected_detector_info(self, detector_infos: list[DetectorInfo]) -> None:
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
        self, edit_box: QLineEdit, detector_infos: list[DetectorInfo], property_lambda: Callable[[DetectorInfo], str]
    ) -> None:
        edit_box.setText(",".join(property_lambda(d) for d in detector_infos))
        edit_box.adjustSize()
