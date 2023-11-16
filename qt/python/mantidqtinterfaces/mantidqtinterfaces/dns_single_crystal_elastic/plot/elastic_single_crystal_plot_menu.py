# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import os

from mantidqt import icons
from qtpy.QtCore import Signal
from qtpy.QtGui import QIcon
from qtpy.QtWidgets import QActionGroup, QMenu

from mantidqtinterfaces.dns_single_crystal_elastic.plot.colormaps.matplotcolors \
    import colormaps


def set_mdi_icons(mapping):
    mapping['down'].setIcon(icons.get_icon("mdi.arrow-down"))
    mapping['up'].setIcon(icons.get_icon("mdi.arrow-up"))
    mapping['grid'].setIcon(icons.get_icon("mdi.grid"))
    mapping['linestyle'].setIcon(icons.get_icon("mdi.ray-vertex"))
    mapping['crystal_axes'].setIcon(icons.get_icon("mdi.axis-arrow"))
    mapping['projections'].setIcon(icons.get_icon("mdi.chart-bell-curve"))
    mapping['invert_cb'].setIcon(icons.get_icon("mdi.invert-colors"))
    mapping['save_data'].setIcon(icons.get_icon("mdi.database-export"))


def set_up_colormap_selector(mapping):
    plotdir = os.path.dirname(__file__)
    plotdir = plotdir.replace('\\', '/')
    for m in colormaps:
        mapping['colormap'].addItem(
            QIcon(f"{plotdir}/colormaps/{m}.png"), m)
        mapping['colormap'].setCurrentIndex(colormaps.index('jet'))


class DNSElasticSCPlotOptionsMenu(QMenu):
    def __init__(self, parent):
        super().__init__('Options')
        # adding action
        ac_omegaoffset = self.addAction('Change Omegaoffset')
        ac_dx_dy = self.addAction('Change dx/dy')

        # connections
        ac_omegaoffset.triggered.connect(parent.change_omegaoffset)
        ac_dx_dy.triggered.connect(parent.change_dxdy)


class DNSElasticSCPlotViewMenu(QMenu):
    def __init__(self):
        super().__init__('View')
        # adding actions
        self._menu_plottype = PlotTypeMenu(self)
        self.addMenu(self._menu_plottype)
        self._menu_axes = AxesMenu(self)
        self.addMenu(self._menu_axes)
        self._menu_interpolation = InterpolationMenu(self)
        self.addMenu(self._menu_interpolation)
        self._ac_gouraud = self.addAction('Gouraud shading')
        self._ac_gouraud.setCheckable(True)
        self._menu_zoom = ZoomMenu(self)
        self.addMenu(self._menu_zoom)
        self.menues = [
            self._menu_plottype, self._menu_axes, self._menu_interpolation,
            self._menu_zoom
        ]
        # connecting signals
        self._ac_gouraud.triggered.connect(self._gourad_changed)

    sig_replot = Signal(str)

    def _get_shading(self):
        if self._ac_gouraud.isChecked():
            return 'gouraud'
        return 'flat'

    def get_value(self):
        plot_type = self._menu_plottype.get_value()
        axis_type = self._menu_axes.get_value()
        axis_type['shading'] = self._get_shading()
        self._menu_interpolation.change_menu(plot_type)
        axis_type['interpolate'] = self._menu_interpolation.get_value()
        axis_type['plot_type'] = plot_type
        axis_type['zoom'] = self._menu_zoom.get_value()
        return axis_type

    def _gourad_changed(self):
        self.sig_replot.emit('gourad')

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
        super().__init__('Plot Type')
        self.parent = parent
        # adding action
        ac_trimesh = self.addAction('Triangulation')
        ac_quadmesh = self.addAction('Quadmesh (dnsplot like)')
        ac_scatter = self.addAction('Scatter')
        # setting checkable
        ac_trimesh.setCheckable(True)
        ac_quadmesh.setCheckable(True)
        ac_quadmesh.setChecked(True)
        ac_scatter.setCheckable(True)
        # action group
        ptag = QActionGroup(self)
        ptag.addAction(ac_trimesh)
        ptag.addAction(ac_quadmesh)
        ptag.addAction(ac_scatter)
        # ptag.triggered.connect(self.type_changed)
        self.ptag = ptag
        self.ac_trimesh = ac_trimesh
        self.ac_quadmesh = ac_quadmesh
        self.ac_scatter = ac_scatter
        # connecting signals
        self.ptag.triggered.connect(self._type_changed)

    def deactivate_quadmesh(self, deactivate=True):
        # quadmesh only possible on rectangular grid
        # if not rectangular change to triangulation
        if deactivate:
            # self.ac_inp_off.setChecked(True)
            self.ac_trimesh.setChecked(True)
            # self.type_changed()
        self.ac_quadmesh.setEnabled(not deactivate)

    def _type_changed(self):
        # plot_type = self.get_value()
        self.parent.sig_replot.emit('type')

    def set_type(self, ptype):
        polttype_list = {
            'triangulation': 'ac_trimesh',
            'quadmesh': 'ac_quadmesh',
            'scatter': 'ac_scatter'
        }
        getattr(self, polttype_list[ptype]).setChecked(True)

    def get_value(self):
        index = self.ptag.actions().index(self.ptag.checkedAction())
        polttype_list = {0: 'triangulation', 1: 'quadmesh', 2: 'scatter'}
        return polttype_list[index]


class AxesMenu(QMenu):
    def __init__(self, parent):
        super().__init__('Axes')
        self.parent = parent
        # ading actions
        ac_tthomega = self.addAction('2theta / omega')
        ac_qxqx = self.addAction('qx / qy')
        ac_hkl = self.addAction('hkl1 / hkl2')
        self.addSeparator()
        ac_fix_aspect = self.addAction('fix aspect ratio')
        ac_switch_axis = self.addAction('switch axes')
        # setting checable and standard option checked
        ac_tthomega.setCheckable(True)
        ac_qxqx.setCheckable(True)
        ac_switch_axis.setCheckable(True)
        ac_fix_aspect.setCheckable(True)
        ac_hkl.setCheckable(True)
        ac_hkl.setChecked(True)
        # action group
        qag = QActionGroup(self)
        qag.addAction(ac_tthomega)
        qag.addAction(ac_qxqx)
        qag.addAction(ac_hkl)
        qag.setExclusive(True)
        # connect Signals
        # ac_switch_axis.toggled.connect(parent.axis_change)
        # ac_fix_aspect.toggled.connect(parent.axis_change)
        # ac_gouraud.toggled.connect(parent.axis_change)
        # ipag.triggered.connect(parent.axis_change)
        # qag.triggered.connect(parent.axis_change)
        self.qag = qag
        self.ac_switch_axis = ac_switch_axis
        self.ac_fix_aspect = ac_fix_aspect
        self.qag.triggered.connect(self._axis_changed)
        self.ac_fix_aspect.triggered.connect(self._axis_changed)
        self.ac_switch_axis.triggered.connect(self._axis_changed)

    def _axis_changed(self):
        sender = self.sender()
        if sender == self.qag:
            self.parent.sig_replot.emit('axis_type')
        if sender == self.ac_switch_axis:
            self.parent.sig_replot.emit('switch_axis')
        if sender == self.ac_fix_aspect:
            self.parent.sig_replot.emit('fix_aspect')

    def get_value(self):
        axis_list = {0: 'tthomega', 1: 'qxqy', 2: 'hkl'}
        index = self.qag.actions().index(self.qag.checkedAction())
        atype = axis_list[index]
        switch = self.ac_switch_axis.isChecked()
        fix_aspect = self.ac_fix_aspect.isChecked()
        return {'type': atype, 'switch': switch, 'fix_aspect': fix_aspect}


class InterpolationMenu(QMenu):
    def __init__(self, parent):
        super().__init__('Interpolation')
        self.parent = parent
        # adding actions
        ac_inp_off = self.addAction('off')
        ac_inp_1 = self.addAction('1 -> 4')
        ac_inp_2 = self.addAction('1 -> 9 (dnsplot like')
        ac_inp_3 = self.addAction('1 -> 16 ')
        # setting checable and check standard option
        ac_inp_off.setCheckable(True)
        ac_inp_1.setCheckable(True)
        ac_inp_2.setCheckable(True)
        ac_inp_3.setCheckable(True)
        ac_inp_2.setChecked(True)
        # action group
        ipag = QActionGroup(self)
        ipag.addAction(ac_inp_off)
        ipag.addAction(ac_inp_1)
        ipag.addAction(ac_inp_2)
        ipag.addAction(ac_inp_3)
        ipag.setExclusive(True)
        self.ac_inp_1 = ac_inp_1
        self.ac_inp_2 = ac_inp_2
        self.ac_inp_3 = ac_inp_3
        self.ipag = ipag
        self.ipag.triggered.connect(self._intp_changed)

    def _intp_changed(self):
        self.parent.sig_replot.emit('interpolation')

    def set_intp(self, intp):
        getattr(self, f'ac_intp{intp}').setChecked(True)

    def change_menu(self, plot_type):
        self.setEnabled(plot_type != 'scatter')
        self._set_inp_text(plot_type)

    def _set_inp_text(self, plot_type):
        if plot_type == 'triangulation':
            labels = [
                '1 -> 4 (dnsplot like)', '1 -> 16 (slow)',
                '1 -> 64 (very slow)'
            ]
        else:
            labels = ['1 -> 4', '1 -> 9 (dnsplot like)', '1 -> 16']
        self.ac_inp_1.setText(labels[0])
        self.ac_inp_2.setText(labels[1])
        self.ac_inp_3.setText(labels[2])

    def get_value(self):
        return self.ipag.actions().index(self.ipag.checkedAction())


class ZoomMenu(QMenu):
    def __init__(self, parent):
        super().__init__('Synchronize zooming')
        self.parent = parent
        self.zoom = {'fix_xy': False, 'fix_z': False}
        self.ac_xy_zoom = self.addAction('x and y')
        self.ac_xy_zoom.setCheckable(True)
        self.ac_z_zoom = self.addAction('z')
        self.ac_z_zoom.setCheckable(True)
        self.ac_xyz_zoom = self.addAction('x and y and z')
        self.ac_xyz_zoom.setCheckable(True)
        # connect Signals
        self.ac_z_zoom.triggered.connect(self._synch_zoom)
        self.ac_xy_zoom.triggered.connect(self._synch_zoom)
        self.ac_xyz_zoom.triggered.connect(self._synch_zoom)

    def _synch_zoom(self):
        xy = self.ac_xy_zoom.isChecked()
        z = self.ac_z_zoom.isChecked()
        self.zoom = {'fix_xy': xy, 'fix_z': z}
        xyz = self.ac_xyz_zoom.isChecked()
        if self.sender() == self.ac_xyz_zoom:
            self.ac_z_zoom.setChecked(xyz)
            self.ac_xy_zoom.setChecked(xyz)
            self.zoom = {'fix_xy': xyz, 'fix_z': xyz}
        else:
            self.ac_xyz_zoom.setChecked(xy and z)
        self.parent.sig_replot.emit('zoom')

    def get_value(self):
        return self.zoom
