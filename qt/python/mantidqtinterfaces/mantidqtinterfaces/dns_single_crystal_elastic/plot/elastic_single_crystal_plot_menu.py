# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

"""
DNS single crystal elastic plot tab menu of DNS reduction GUI.
"""

from mantidqt import icons
from qtpy.QtCore import Signal
from qtpy.QtGui import QPixmap, QIcon
from qtpy.QtWidgets import QActionGroup, QMenu
from mantidqt.widgets.plotconfigdialog.imagestabwidget.view import create_colormap_img
from mantid.plots.utility import get_colormap_names


def set_mdi_icons(mapping):
    mapping["down"].setIcon(icons.get_icon("mdi.arrow-down"))
    mapping["up"].setIcon(icons.get_icon("mdi.arrow-up"))
    mapping["grid"].setIcon(icons.get_icon("mdi.grid"))
    mapping["linestyle"].setIcon(icons.get_icon("mdi.ray-vertex"))
    mapping["crystal_axes"].setIcon(icons.get_icon("mdi.axis-arrow"))
    mapping["projections"].setIcon(icons.get_icon("mdi.chart-bell-curve"))
    mapping["invert_cb"].setIcon(icons.get_icon("mdi.invert-colors"))
    mapping["save_data"].setIcon(icons.get_icon("mdi.database-export"))


def set_up_colormap_selector(mapping):
    colormap_names = get_colormap_names()
    for cmap_name in colormap_names:
        qt_img = create_colormap_img(cmap_name)
        pixmap = QPixmap.fromImage(qt_img)
        mapping["colormap"].addItem(QIcon(pixmap), cmap_name)
        mapping["colormap"].setCurrentIndex(colormap_names.index("jet"))


class DNSElasticSCPlotOptionsMenu(QMenu):
    def __init__(self, parent):
        super().__init__("Plot Options")
        # adding action
        action_omega_offset = self.addAction("Change \u03c9 Offset")
        action_dx_dy = self.addAction("Change d-spacings")

        # connections
        action_omega_offset.triggered.connect(parent.change_omega_offset)
        action_dx_dy.triggered.connect(parent.change_dxdy)


class DNSElasticSCPlotViewMenu(QMenu):
    def __init__(self):
        super().__init__("Plot View")
        # adding actions
        self._menu_plot_type = PlotTypeMenu(self)
        self.addMenu(self._menu_plot_type)
        self._menu_axes = AxesMenu(self)
        self.addMenu(self._menu_axes)
        self._menu_interpolation = InterpolationMenu(self)
        self.addMenu(self._menu_interpolation)
        self._action_gouraud = self.addAction("Gouraud Shading")
        self._action_gouraud.setCheckable(True)
        self._menu_zoom = ZoomMenu(self)
        self.addMenu(self._menu_zoom)
        self.menus = [self._menu_plot_type, self._menu_axes, self._menu_interpolation, self._menu_zoom]
        # connecting signals
        self._action_gouraud.triggered.connect(self._gouraud_changed)

    sig_replot = Signal(str)
    sig_switch_state_changed = Signal()
    sig_axes_changed = Signal()

    def _get_shading(self):
        if self._action_gouraud.isChecked():
            return "gouraud"
        return "flat"

    def get_plot_view_settings(self):
        axis_dict = self._menu_axes.get_axis_settings()
        plot_view_dict = axis_dict
        plot_view_dict["shading"] = self._get_shading()
        plot_view_dict["plot_type"] = self._menu_plot_type.get_plot_type()
        plot_view_dict["interpolate"] = self._menu_interpolation.get_interpolation_setting_index()
        plot_view_dict["zoom"] = self._menu_zoom.get_value()
        return plot_view_dict

    def _gouraud_changed(self):
        self.sig_replot.emit("gouraud")

    def set_interpolation_menu_options(self):
        plot_type = self._menu_plot_type.get_plot_type()
        self._menu_interpolation.change_interpolation_menu(plot_type)


class PlotTypeMenu(QMenu):
    def __init__(self, parent):
        super().__init__("Plot Type")
        self.parent = parent
        # adding action
        action_triangulation_mesh = self.addAction("Triangulation")
        action_quad_mesh = self.addAction("Quadmesh (like in dnsplot)")
        action_scatter_mesh = self.addAction("Scatter")
        # setting checkable
        action_triangulation_mesh.setCheckable(True)
        action_quad_mesh.setCheckable(True)
        action_scatter_mesh.setCheckable(True)
        action_triangulation_mesh.setChecked(True)
        # action group
        plot_type_action_group = QActionGroup(self)
        plot_type_action_group.addAction(action_triangulation_mesh)
        plot_type_action_group.addAction(action_quad_mesh)
        plot_type_action_group.addAction(action_scatter_mesh)
        self.plot_type_action_group = plot_type_action_group
        self.action_triangulation_mesh = action_triangulation_mesh
        self.action_quad_mesh = action_quad_mesh
        self.action_scatter = action_scatter_mesh
        # connecting signals
        self.plot_type_action_group.triggered.connect(self._type_changed)

    def _type_changed(self):
        self.parent.set_interpolation_menu_options()
        self.parent.sig_replot.emit("type")

    def set_type(self, ptype):
        plot_type_list = {"triangulation": "action_triangulation_mesh", "quadmesh": "action_quad_mesh", "scatter": "action_scatter"}
        getattr(self, plot_type_list[ptype]).setChecked(True)

    def get_plot_type(self):
        index = self.plot_type_action_group.actions().index(self.plot_type_action_group.checkedAction())
        plot_type_list = {0: "triangulation", 1: "quadmesh", 2: "scatter"}
        return plot_type_list[index]


class AxesMenu(QMenu):
    def __init__(self, parent):
        super().__init__("Axes")
        self.parent = parent
        action_two_theta_omega = self.addAction("(2\u03b8, \u03c9)")
        action_qxqy = self.addAction("(q_x, q_y)")
        action_hkl = self.addAction("(n_x, n_y)")
        self.addSeparator()
        action_fix_aspect = self.addAction("Fix Aspect Ratio")
        action_switch_axis = self.addAction("Switch Axes")
        # setting checkable and standard option checked
        action_two_theta_omega.setCheckable(True)
        action_qxqy.setCheckable(True)
        action_switch_axis.setCheckable(True)
        action_fix_aspect.setCheckable(True)
        action_hkl.setCheckable(True)
        action_hkl.setChecked(True)
        # action group
        axes_action_group = QActionGroup(self)
        axes_action_group.addAction(action_two_theta_omega)
        axes_action_group.addAction(action_qxqy)
        axes_action_group.addAction(action_hkl)
        axes_action_group.setExclusive(True)
        # connect Signals
        self.axes_action_group = axes_action_group
        self.action_switch_axis = action_switch_axis
        self.action_fix_aspect = action_fix_aspect
        self.axes_action_group.triggered.connect(self._axis_type_changed)
        self.action_fix_aspect.triggered.connect(self._fix_aspect_changed)
        self.action_switch_axis.triggered.connect(self._switch_axis_changed)

    def _axis_type_changed(self):
        self.parent.sig_axes_changed.emit()

    def _fix_aspect_changed(self):
        self.parent.sig_replot.emit("fix_aspect")

    def _switch_axis_changed(self):
        self.parent.sig_switch_state_changed.emit()

    def get_axis_settings(self):
        axis_list = {0: "angular", 1: "qxqy", 2: "hkl"}
        index = self.axes_action_group.actions().index(self.axes_action_group.checkedAction())
        axis_type = axis_list[index]
        switch = self.action_switch_axis.isChecked()
        fix_aspect = self.action_fix_aspect.isChecked()
        return {"type": axis_type, "switch": switch, "fix_aspect": fix_aspect}


class InterpolationMenu(QMenu):
    def __init__(self, parent):
        super().__init__("Interpolation")
        self.parent = parent
        # adding actions
        action_interpolation_off = self.addAction("Off")
        # default values for triangulation interpolation
        action_interpolation_1 = self.addAction("1 -> 4 (like in dnsplot)")
        action_interpolation_2 = self.addAction("1 -> 16 (slow)")
        action_interpolation_3 = self.addAction("1 -> 64 (very slow)")
        # setting checkable and check standard option
        action_interpolation_off.setCheckable(True)
        action_interpolation_1.setCheckable(True)
        action_interpolation_2.setCheckable(True)
        action_interpolation_3.setCheckable(True)
        action_interpolation_off.setChecked(True)
        # action group
        interpolation_action_group = QActionGroup(self)
        interpolation_action_group.addAction(action_interpolation_off)
        interpolation_action_group.addAction(action_interpolation_1)
        interpolation_action_group.addAction(action_interpolation_2)
        interpolation_action_group.addAction(action_interpolation_3)
        interpolation_action_group.setExclusive(True)
        self.action_interpolation_1 = action_interpolation_1
        self.action_interpolation_2 = action_interpolation_2
        self.action_interpolation_3 = action_interpolation_3
        self.interpolation_action_group = interpolation_action_group
        self.interpolation_action_group.triggered.connect(self._interpolation_changed)

    def _interpolation_changed(self):
        self.parent.sig_replot.emit("interpolation")

    def set_intp(self, interpolation):
        getattr(self, f"action_interpolation_{interpolation}").setChecked(True)

    def change_interpolation_menu(self, plot_type):
        self.setEnabled(plot_type != "scatter")
        self._set_interpolation_options(plot_type)

    def _set_interpolation_options(self, plot_type):
        if plot_type == "triangulation":
            labels = ["1 -> 4 (like in dnsplot)", "1 -> 16 (slow)", "1 -> 64 (very slow)"]
        else:  # quadmesh
            labels = ["1 -> 4", "1 -> 9 (like in dnsplot)", "1 -> 16"]
        self.action_interpolation_1.setText(labels[0])
        self.action_interpolation_2.setText(labels[1])
        self.action_interpolation_3.setText(labels[2])

    def get_interpolation_setting_index(self):
        return self.interpolation_action_group.actions().index(self.interpolation_action_group.checkedAction())


class ZoomMenu(QMenu):
    def __init__(self, parent):
        super().__init__("Synchronize Zooming")
        self.parent = parent
        self.zoom = {"fix_xy": False, "fix_z": False}
        self.action_xy_zoom = self.addAction("x and y")
        self.action_xy_zoom.setCheckable(True)
        self.action_z_zoom = self.addAction("z (intensity)")
        self.action_z_zoom.setCheckable(True)
        # connect Signals
        self.action_xy_zoom.triggered.connect(self._sync_zoom_changed)
        self.action_z_zoom.triggered.connect(self._sync_zoom_changed)

    def _sync_zoom_changed(self):
        xy = self.action_xy_zoom.isChecked()
        z = self.action_z_zoom.isChecked()
        self.zoom = {"fix_xy": xy, "fix_z": z}
        self.parent.sig_replot.emit("zoom")

    def get_value(self):
        return self.zoom
