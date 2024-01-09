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
        action_omega_offset = self.addAction("Change \u03C9 Offset")
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
        self.menus = [self._menu_plot_type, self._menu_axes, self._menu_interpolation]

    sig_replot = Signal(str)

    def get_value(self):
        plot_type = self._menu_plot_type.get_value()
        axis_type = self._menu_axes.get_value()
        axis_type["interpolate"] = self._menu_interpolation.get_value()
        axis_type["plot_type"] = plot_type
        return axis_type


class PlotTypeMenu(QMenu):
    def __init__(self, parent):
        super().__init__("Plot Type")
        self.parent = parent
        # adding action
        action_triangulation_mesh = self.addAction("Triangulation")
        action_triangulation_mesh.setCheckable(True)
        action_triangulation_mesh.setChecked(True)
        # action group
        p_tag = QActionGroup(self)
        p_tag.addAction(action_triangulation_mesh)
        self.p_tag = p_tag
        self.action_triangulation_mesh = action_triangulation_mesh

    def get_value(self):
        index = self.p_tag.actions().index(self.p_tag.checkedAction())
        plot_type_list = {0: "triangulation"}
        return plot_type_list[index]


class AxesMenu(QMenu):
    def __init__(self, parent):
        super().__init__("Axes")
        self.parent = parent
        action_hkl = self.addAction("(n_x, n_y)")
        self.addSeparator()
        # setting checkable and standard option checked
        action_hkl.setCheckable(True)
        action_hkl.setChecked(True)
        # action group
        qag = QActionGroup(self)
        qag.addAction(action_hkl)
        qag.setExclusive(True)
        # connect Signals
        self.qag = qag

    def get_value(self):
        axis_list = {0: "hkl"}
        index = self.qag.actions().index(self.qag.checkedAction())
        axis_type = axis_list[index]
        return {"type": axis_type, "switch": False, "fix_aspect": False}


class InterpolationMenu(QMenu):
    def __init__(self, parent):
        super().__init__("Interpolation")
        self.parent = parent
        # adding actions
        action_interpolation_off = self.addAction("Off")
        # setting checkable and check standard option
        action_interpolation_off.setCheckable(True)
        action_interpolation_off.setChecked(True)
        # action group
        i_pag = QActionGroup(self)
        i_pag.addAction(action_interpolation_off)
        i_pag.setExclusive(True)
        self.i_pag = i_pag

    def get_value(self):
        return self.i_pag.actions().index(self.i_pag.checkedAction())
