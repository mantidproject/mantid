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

    def _get_shading(self):
        if self._action_gouraud.isChecked():
            return "gouraud"
        return "flat"

    def get_value(self):
        plot_type = self._menu_plot_type.get_value()
        axis_type = self._menu_axes.get_value()
        axis_type["shading"] = self._get_shading()
        self._menu_interpolation.change_menu(plot_type)
        axis_type["interpolate"] = self._menu_interpolation.get_value()
        axis_type["plot_type"] = plot_type
        axis_type["zoom"] = self._menu_zoom.get_value()
        return axis_type

    def _gouraud_changed(self):
        self.sig_replot.emit("gouraud")

    def deactivate_quadmesh(self, deactivate=True):
        # quadmesh only possible on rectangular grid
        # if not rectangular change to triangulation
        self.disconnect()
        if deactivate:
            self._menu_axes.set_intp(0)
            self.ac_trimesh.setChecked(True)
        self.ac_quadmesh.setEnabled(not deactivate)
        self.connect()


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
        action_triangulation_mesh.setChecked(True)
        # action group
        p_tag = QActionGroup(self)
        p_tag.addAction(action_triangulation_mesh)
        p_tag.addAction(action_quad_mesh)
        p_tag.addAction(action_scatter_mesh)
        self.p_tag = p_tag
        self.action_triangulation_mesh = action_triangulation_mesh
        self.action_quad_mesh = action_quad_mesh
        self.action_scatter = action_scatter_mesh
        # connecting signals
        self.p_tag.triggered.connect(self._type_changed)

    def deactivate_quadmesh(self, deactivate=True):
        # quadmesh only possible on rectangular grid
        # if not rectangular change to triangulation
        if deactivate:
            # self.ac_inp_off.setChecked(True)
            self.action_triangulation_mesh.setChecked(True)
            # self.type_changed()
        self.action_quad_mesh.setEnabled(not deactivate)

    def _type_changed(self):
        # plot_type = self.get_value()
        self.parent.sig_replot.emit("type")

    def set_type(self, ptype):
        plot_type_list = {"triangulation": "action_triangulation_mesh", "quadmesh": "action_quad_mesh", "scatter": "action_scatter"}
        getattr(self, plot_type_list[ptype]).setChecked(True)

    def get_value(self):
        index = self.p_tag.actions().index(self.p_tag.checkedAction())
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
        qag = QActionGroup(self)
        qag.addAction(action_two_theta_omega)
        qag.addAction(action_qxqy)
        qag.addAction(action_hkl)
        qag.setExclusive(True)
        # connect Signals
        self.qag = qag
        self.action_switch_axis = action_switch_axis
        self.action_fix_aspect = action_fix_aspect
        self.qag.triggered.connect(self._axis_changed)
        self.action_fix_aspect.triggered.connect(self._axis_changed)
        self.action_switch_axis.triggered.connect(self._axis_changed)

    def _axis_changed(self):
        sender = self.sender()
        if sender == self.qag:
            self.parent.sig_replot.emit("axis_type")
        if sender == self.action_switch_axis:
            self.parent.sig_replot.emit("switch_axis")
        if sender == self.action_fix_aspect:
            self.parent.sig_replot.emit("fix_aspect")

    def get_value(self):
        axis_list = {0: "angular", 1: "qxqy", 2: "hkl"}
        index = self.qag.actions().index(self.qag.checkedAction())
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
        action_interpolation_1 = self.addAction("1 -> 4")
        action_interpolation_2 = self.addAction("1 -> 9 (like in dnsplot)")
        action_interpolation_3 = self.addAction("1 -> 16")
        # setting checkable and check standard option
        action_interpolation_off.setCheckable(True)
        action_interpolation_1.setCheckable(True)
        action_interpolation_2.setCheckable(True)
        action_interpolation_3.setCheckable(True)
        action_interpolation_off.setChecked(True)
        # action group
        i_pag = QActionGroup(self)
        i_pag.addAction(action_interpolation_off)
        i_pag.addAction(action_interpolation_1)
        i_pag.addAction(action_interpolation_2)
        i_pag.addAction(action_interpolation_3)
        i_pag.setExclusive(True)
        self.action_interpolation_1 = action_interpolation_1
        self.action_interpolation_2 = action_interpolation_2
        self.action_interpolation_3 = action_interpolation_3
        self.i_pag = i_pag
        self.i_pag.triggered.connect(self._interpolation_changed)

    def _interpolation_changed(self):
        self.parent.sig_replot.emit("interpolation")

    def set_intp(self, interpolation):
        getattr(self, f"action_interpolation_{interpolation}").setChecked(True)

    def change_menu(self, plot_type):
        self.setEnabled(plot_type != "scatter")
        self._set_inp_text(plot_type)

    def _set_inp_text(self, plot_type):
        if plot_type == "triangulation":
            labels = ["1 -> 4 (like in dnsplot)", "1 -> 16 (slow)", "1 -> 64 (very slow)"]
        else:
            labels = ["1 -> 4", "1 -> 9 (like in dnsplot)", "1 -> 16"]
        self.action_interpolation_1.setText(labels[0])
        self.action_interpolation_2.setText(labels[1])
        self.action_interpolation_3.setText(labels[2])

    def get_value(self):
        return self.i_pag.actions().index(self.i_pag.checkedAction())


class ZoomMenu(QMenu):
    def __init__(self, parent):
        super().__init__("Synchronize Zooming")
        self.parent = parent
        self.zoom = {"fix_xy": False, "fix_z": False}
        self.action_xy_zoom = self.addAction("x and y")
        self.action_xy_zoom.setCheckable(True)
        self.action_z_zoom = self.addAction("z")
        self.action_z_zoom.setCheckable(True)
        self.action_xyz_zoom = self.addAction("x and y and z")
        self.action_xyz_zoom.setCheckable(True)
        # connect Signals
        self.action_z_zoom.triggered.connect(self._synch_zoom)
        self.action_xy_zoom.triggered.connect(self._synch_zoom)
        self.action_xyz_zoom.triggered.connect(self._synch_zoom)

    def _synch_zoom(self):
        xy = self.action_xy_zoom.isChecked()
        z = self.action_z_zoom.isChecked()
        self.zoom = {"fix_xy": xy, "fix_z": z}
        xyz = self.action_xyz_zoom.isChecked()
        if self.sender() == self.action_xyz_zoom:
            self.action_z_zoom.setChecked(xyz)
            self.action_xy_zoom.setChecked(xyz)
            self.zoom = {"fix_xy": xyz, "fix_z": xyz}
        else:
            self.action_xyz_zoom.setChecked(xy and z)
        self.parent.sig_replot.emit("zoom")

    def get_value(self):
        return self.zoom
