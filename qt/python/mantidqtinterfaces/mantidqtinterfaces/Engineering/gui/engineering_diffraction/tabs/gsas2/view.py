# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtWidgets, QtCore
from qtpy.QtGui import QRegExpValidator
from qtpy.QtCore import Qt
from qtpy.QtWidgets import QDockWidget, QMainWindow, QSizePolicy
from os import path

from mantidqt.utils.qt import load_ui
from matplotlib.figure import Figure
import matplotlib.lines as lines
from mantidqt.MPLwidgets import FigureCanvas

from mantidqt.utils.qt.line_edit_double_validator import LineEditDoubleValidator
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.gsas2.plot_toolbar import GSAS2PlotToolbar

Ui_calib, _ = load_ui(__file__, "gsas2_tab.ui")


class GSAS2View(QtWidgets.QWidget, Ui_calib):
    sig_enable_controls = QtCore.Signal(bool)
    sig_update_sample_field = QtCore.Signal()
    fit_range_changed = QtCore.Signal(list)

    def __init__(self, parent=None, instrument="ENGINX"):
        super(GSAS2View, self).__init__(parent)
        self.setupUi(self)

        self.instrument_group_file_finder.setLabelText("Instrument Group")
        self.instrument_group_file_finder.isForRunFiles(False)
        self.instrument_group_file_finder.setFileExtensions([".prm"])
        self.instrument_group_file_finder.allowMultipleFiles(True)

        self.phase_file_finder.setLabelText("Phase")
        self.phase_file_finder.isForRunFiles(False)
        self.phase_file_finder.setFileExtensions([".cif"])
        self.phase_file_finder.allowMultipleFiles(True)

        self.focused_data_file_finder.setLabelText("Focused Data")
        self.focused_data_file_finder.isForRunFiles(False)
        self.focused_data_file_finder.setFileExtensions([".gss", ".gsa"])
        self.focused_data_file_finder.allowMultipleFiles(True)

        self.mark_project_name_invalid_when_empty()
        self.project_name_line_edit.textChanged.connect(self.mark_project_name_invalid_when_empty)
        self.mark_checkboxes_invalid_when_empty()
        self.refine_microstrain_checkbox.stateChanged.connect(self.mark_checkboxes_invalid_when_empty)
        self.refine_sigma_one_checkbox.stateChanged.connect(self.mark_checkboxes_invalid_when_empty)
        self.refine_gamma_y_checkbox.stateChanged.connect(self.mark_checkboxes_invalid_when_empty)
        self.x_min_line_edit.editingFinished.connect(self.set_min_line_from_line_edit)
        self.x_max_line_edit.editingFinished.connect(self.set_max_line_from_line_edit)

        none_one_many_int_float_comma_separated = QRegExpValidator(
            QtCore.QRegExp(r"^(?:\d+(?:\.\d*)?|\.\d+)(?:,(?:\d+(?:\.\d*)?|\.\d+))*$"), self.override_unitcell_length
        )
        self.override_unitcell_length.setValidator(none_one_many_int_float_comma_separated)
        valid_file_name = QRegExpValidator(QtCore.QRegExp(r'^[^<>:;,"@£$%&\'^!?"*|\\\/]+$'), self.project_name_line_edit)
        self.project_name_line_edit.setValidator(valid_file_name)
        self.x_min_line_edit.setValidator(LineEditDoubleValidator(self.x_min_line_edit, 0))
        self.x_max_line_edit.setValidator(LineEditDoubleValidator(self.x_max_line_edit, 1))

        combobox_index = self.refinement_method_combobox.findText("Rietveld")
        self.refinement_method_combobox.model().item(combobox_index).setEnabled(False)
        self.refinement_method_combobox.model().item(combobox_index).setToolTip("Rietveld is not currently supported.")

        # Plotting
        self.figure = None
        self.min_line = None
        self.min_follower = None
        self.min_releaser = None
        self.max_line = None
        self.max_follower = None
        self.max_releaser = None
        self.toolbar = None
        self.plot_dock = None
        self.dock_window = None
        self.initial_chart_width = None
        self.initial_chart_height = None
        self.has_first_undock_occurred = 0
        self.fit_range = None
        self.initial_x_limits = None
        self.x_bounds = None
        self.y_bounds = None

        self.setup_figure()
        self.setup_toolbar()

    # ===============
    # Slot Connectors
    # ===============

    def set_refine_clicked(self, slot):
        self.refine_button.clicked.connect(slot)

    # =================
    # Component Setters
    # =================

    def set_default_gss_files(self, filepaths):
        self.set_default_files(filepaths, self.focused_data_file_finder)

    def set_default_prm_files(self, filepath):
        self.set_default_files([filepath], self.instrument_group_file_finder)

    def set_default_files(self, filepaths, file_finder):
        if not filepaths:
            return
        file_finder.setUserInput(",".join(filepaths))
        directories = set()
        for filepath in filepaths:
            directory, discard = path.split(filepath)
            directories.add(directory)
        if len(directories) == 1:
            file_finder.setLastDirectory(directory)

    def set_number_histograms(self, number_output_histograms):
        self.number_output_histograms_combobox.clear()
        histogram_indices_items = [str(k) for k in range(1, number_output_histograms + 1)]
        self.number_output_histograms_combobox.addItems(histogram_indices_items)
        self.number_output_histograms_combobox.setCurrentIndex(0)

    def mark_project_name_invalid_when_empty(self):
        new_value = self.project_name_line_edit.text().strip(" \t")
        if new_value == "":
            self.project_name_invalid.show()
            self.project_name_invalid.setToolTip("No Project Name specified.")
        else:
            self.project_name_invalid.hide()
            self.project_name_invalid.setToolTip("")

    def mark_checkboxes_invalid_when_empty(self):
        if (
            self.refine_microstrain_checkbox.isChecked()
            and self.refine_sigma_one_checkbox.isChecked()
            and self.refine_gamma_y_checkbox.isChecked()
        ):
            self.checkboxes_invalid.show()
            self.checkboxes_invalid.setToolTip("Refining the Microstrain with Sigma-1 and Gamma(Y) may not be advisable.")
        else:
            self.checkboxes_invalid.hide()
            self.checkboxes_invalid.setToolTip("")

    # =================
    # Component Getters
    # =================

    def get_refinement_parameters(self):
        return [
            self.refinement_method_combobox.currentText(),
            self.override_unitcell_length.text(),
            self.refine_microstrain_checkbox.isChecked(),
            self.refine_sigma_one_checkbox.isChecked(),
            self.refine_gamma_y_checkbox.isChecked(),
        ]

    def get_load_parameters(self):
        return [
            self.instrument_group_file_finder.getFilenames(),
            self.phase_file_finder.getFilenames(),
            self.focused_data_file_finder.getFilenames(),
        ]

    def get_project_name(self):
        return self.project_name_line_edit.text()

    def get_x_limits_from_line_edits(self):
        # if self.figure.axes[0].get_lines():  # check there is currently a valid plot
        return [[self.x_min_line_edit.text()], [self.x_max_line_edit.text()]]

    # =================
    # Force Actions
    # =================

    def find_files_load_parameters(self):
        self.instrument_group_file_finder.findFiles(True)
        self.phase_file_finder.findFiles(True)
        self.focused_data_file_finder.findFiles(True)

    # =================
    # Plot Window Setup
    # =================

    def setup_figure(self):
        self.figure = Figure()
        self.figure.canvas = FigureCanvas(self.figure)
        self.figure.canvas.mpl_connect("pick_event", self.on_press)

        # self.figure.canvas.mpl_connect('button_press_event', self.mouse_click)
        self.figure.add_subplot(111, projection="mantid")
        self.toolbar = GSAS2PlotToolbar(self.figure.canvas, self, False)
        self.toolbar.setMovable(False)

        self.dock_window = QMainWindow(self.group_plot)
        self.dock_window.setWindowFlags(Qt.Widget)
        self.dock_window.setDockOptions(QMainWindow.AnimatedDocks)
        self.dock_window.setCentralWidget(self.toolbar)
        self.plot_dock = QDockWidget()
        self.plot_dock.setWidget(self.figure.canvas)
        self.plot_dock.setFeatures(QDockWidget.DockWidgetFloatable | QDockWidget.DockWidgetMovable)
        self.plot_dock.setAllowedAreas(Qt.BottomDockWidgetArea)
        self.plot_dock.setWindowTitle("GSAS II Plot")
        self.plot_dock.topLevelChanged.connect(self.make_undocked_plot_larger)
        self.initial_chart_width, self.initial_chart_height = self.plot_dock.width(), self.plot_dock.height()
        self.plot_dock.setSizePolicy(QSizePolicy(QSizePolicy.MinimumExpanding, QSizePolicy.MinimumExpanding))
        self.dock_window.addDockWidget(Qt.BottomDockWidgetArea, self.plot_dock)
        self.vLayout_plot.addWidget(self.dock_window)

    def resizeEvent(self, QResizeEvent):
        self.update_axes_position()

    def make_undocked_plot_larger(self):
        # only make undocked plot larger the first time it is undocked as the undocked size gets remembered
        if self.plot_dock.isFloating() and self.has_first_undock_occurred == 0:
            factor = 1.0
            aspect_ratio = self.initial_chart_width / self.initial_chart_height
            new_height = self.initial_chart_height * factor
            docked_height = self.dock_window.height()
            if docked_height > new_height:
                new_height = docked_height
            new_width = new_height * aspect_ratio
            self.plot_dock.resize(int(new_width), int(new_height))
            self.has_first_undock_occurred = 1

        self.update_axes_position()

    def setup_toolbar(self):
        self.toolbar.sig_home_clicked.connect(self.display_all)
        self.toolbar.sig_home_clicked.connect(self.display_all)

    # ======================
    # Plot Component Setters
    # ======================

    def clear_figure(self):
        self.figure.clf()
        self.figure.add_subplot(111, projection="mantid")
        self.figure.canvas.draw()
        self.x_bounds = None
        self.y_bounds = None

    def update_figure(self, plot_window_title=None):
        window_title = "GSAS-II Plot"
        if plot_window_title:
            window_title = plot_window_title
        self.plot_dock.setWindowTitle(window_title)
        self.toolbar.update()
        self.update_legend(self.get_axes()[0])
        self.update_axes_position()
        self.figure.canvas.draw()

    def update_axes_position(self):
        """
        Set axes position so that labels are always visible - it deliberately ignores height of ylabel (and less
        importantly the length of xlabel). This is because the plot window typically has a very small height when docked
        in the UI.
        """
        ax = self.get_axes()[0]
        y0_lab = (
            ax.xaxis.get_tightbbox(renderer=self.figure.canvas.get_renderer()).transformed(self.figure.transFigure.inverted()).y0
        )  # vertical coord of bottom left corner of xlabel in fig ref. frame
        x0_lab = (
            ax.yaxis.get_tightbbox(renderer=self.figure.canvas.get_renderer()).transformed(self.figure.transFigure.inverted()).x0
        )  # horizontal coord of bottom left corner ylabel in fig ref. frame
        pos = ax.get_position()
        x0_ax = pos.x0 + 0.05 - x0_lab  # move so that ylabel left bottom corner at horizontal coord 0.05
        y0_ax = pos.y0 + 0.05 - y0_lab  # move so that xlabel left bottom corner at vertical coord 0.05
        ax.set_position([x0_ax, y0_ax, 0.95 - x0_ax, 0.95 - y0_ax])

    def update_legend(self, ax):
        if ax.get_lines():
            ax.make_legend()
            handles, labels = ax.get_legend_handles_labels()
            by_label = dict(zip(labels, handles))
            ax.legend(by_label.values(), by_label.keys())
            ax.get_legend().set_title("")
            ax.get_legend().set_draggable(True)
        else:
            if ax.get_legend():
                ax.get_legend().remove()

    def display_all(self):
        for ax in self.get_axes():
            if ax.lines:
                ax.relim()
            if self.x_bounds and self.y_bounds:
                ax.set_xlim(self.x_bounds)
                ax.set_ylim(self.y_bounds)
            else:
                ax.autoscale()
        self.update_figure()

    # ======================
    # Plot Component Getters
    # ======================

    def get_axes(self):
        return self.figure.axes

    def get_figure(self):
        return self.figure

    # ========================
    # Limits and Range Markers
    # ========================

    def setup_range_markers(self, x_minimum, x_maximum):
        self.x_bounds = self.figure.axes[0].get_xlim()
        self.y_bounds = self.figure.axes[0].get_ylim()
        self.min_line = lines.Line2D([x_minimum, x_minimum], [self.y_bounds[0], self.y_bounds[1]], picker=5, color="green", linestyle="--")
        self.figure.axes[0].add_line(self.min_line)
        self.max_line = lines.Line2D([x_maximum, x_maximum], [self.y_bounds[0], self.y_bounds[1]], picker=5, color="red", linestyle="--")
        self.figure.axes[0].add_line(self.max_line)
        self.figure.canvas.draw_idle()
        self.update_figure()

    def on_press(self, event):
        if event.artist == self.min_line:
            self.min_follower = self.figure.canvas.mpl_connect("motion_notify_event", self.min_follow_mouse)
            self.min_releaser = self.figure.canvas.mpl_connect("button_release_event", self.min_release)

        elif event.artist == self.max_line:
            self.max_follower = self.figure.canvas.mpl_connect("motion_notify_event", self.max_follow_mouse)
            self.max_releaser = self.figure.canvas.mpl_connect("button_release_event", self.max_release)

    def min_follow_mouse(self, event):
        if event.xdata and self.initial_x_limits[0] <= float(event.xdata) <= float(self.x_max_line_edit.text()):
            self.min_line.set_xdata([event.xdata, event.xdata])
            self.set_x_min_line_edit(event.xdata)
        else:
            self.min_line.set_xdata([self.initial_x_limits[0], self.initial_x_limits[0]])
            self.set_x_min_line_edit(self.initial_x_limits[0])
        self.figure.canvas.draw_idle()

    def max_follow_mouse(self, event):
        if event.xdata and float(self.x_min_line_edit.text()) <= float(event.xdata) <= self.initial_x_limits[1]:
            self.max_line.set_xdata([event.xdata, event.xdata])
            self.set_x_max_line_edit(event.xdata)
        else:
            self.max_line.set_xdata([self.initial_x_limits[1], self.initial_x_limits[1]])
            self.set_x_max_line_edit(self.initial_x_limits[1])
        self.figure.canvas.draw_idle()

    def min_release(self, event):
        self.figure.canvas.mpl_disconnect(self.min_releaser)
        self.figure.canvas.mpl_disconnect(self.min_follower)

    def max_release(self, event):
        self.figure.canvas.mpl_disconnect(self.max_releaser)
        self.figure.canvas.mpl_disconnect(self.max_follower)

    def set_initial_x_limits(self, x_minimum, x_maximum):
        self.initial_x_limits = [x_minimum, x_maximum]
        tolerance = 0.01
        self.x_min_line_edit.validator().setBottom(float(x_minimum) - tolerance)
        self.x_min_line_edit.validator().setTop(float(x_maximum) + tolerance)
        self.x_max_line_edit.validator().setBottom(float(x_minimum) - tolerance)
        self.x_max_line_edit.validator().setTop(float(x_maximum) + tolerance)
        self.x_min_line_edit.validator().last_valid_value = str("{:.2f}".format(x_minimum))
        self.x_max_line_edit.validator().last_valid_value = str("{:.2f}".format(x_maximum))

    def set_min_line_from_line_edit(self):
        new_value = self.x_min_line_edit.text()
        if self.min_line and new_value != "":
            new_value = float(new_value)
            if self.initial_x_limits[0] <= new_value <= float(self.x_max_line_edit.text()):
                self.min_line.set_xdata([new_value, new_value])
            else:
                self.min_line.set_xdata([self.initial_x_limits[0], self.initial_x_limits[0]])
                self.set_x_min_line_edit(self.initial_x_limits[0])
            self.figure.canvas.draw_idle()

    def set_max_line_from_line_edit(self):
        new_value = self.x_max_line_edit.text()
        if self.max_line and new_value != "":
            new_value = float(new_value)
            if float(self.x_min_line_edit.text()) <= new_value <= self.initial_x_limits[1]:
                self.max_line.set_xdata([new_value, new_value])
            else:
                self.max_line.set_xdata([self.initial_x_limits[1], self.initial_x_limits[1]])
                self.set_x_max_line_edit(self.initial_x_limits[1])
            self.figure.canvas.draw_idle()

    def set_x_limits(self, x_minimum, x_maximum):
        self.set_x_limits_line_edits(x_minimum, x_maximum)
        self.setup_range_markers(x_minimum, x_maximum)
        self.set_initial_x_limits(x_minimum, x_maximum)

    def set_x_limits_line_edits(self, x_min, x_max):
        self.set_x_min_line_edit(x_min)
        self.set_x_max_line_edit(x_max)

    def set_x_min_line_edit(self, x_min):
        if x_min and x_min != "":
            two_decimal_string = str("{:.2f}".format(x_min))
            self.x_min_line_edit.setText(two_decimal_string)
            self.x_min_line_edit.validator().last_valid_value = two_decimal_string
        else:
            two_decimal_string = str("{:.2f}".format(self.initial_x_limits[0]))
            self.x_min_line_edit.setText(two_decimal_string)
            self.x_min_line_edit.validator().last_valid_value = two_decimal_string

    def set_x_max_line_edit(self, x_max):
        if x_max and x_max != "":
            two_decimal_string = str("{:.2f}".format(x_max))
            self.x_max_line_edit.setText(two_decimal_string)
            self.x_max_line_edit.validator().last_valid_value = two_decimal_string
        else:
            two_decimal_string = str("{:.2f}".format(self.initial_x_limits[1]))
            self.x_max_line_edit.setText(two_decimal_string)
            self.x_max_line_edit.validator().last_valid_value = two_decimal_string
