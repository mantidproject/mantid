# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
DNS View for simulation elastic DNS data
"""
from __future__ import (absolute_import, division, print_function)

from collections import OrderedDict

from qtpy.QtCore import Signal
from qtpy import QtWidgets, QtGui, QtCore
from qtpy.QtWidgets import QTableWidgetItem, QFileDialog

from matplotlib.backends.backend_qt5agg import (FigureCanvas,
                                                NavigationToolbar2QT as
                                                NavigationToolbar)
from matplotlib.figure import Figure
from matplotlib.cm import get_cmap

from DNSReduction.data_structures.dns_view import DNSView
import DNSReduction.simulation.simulation_helpers as sim_help

try:
    from mantidqt.utils.qt import load_ui
except ImportError:
    from mantidplot import load_ui


class MyTableWidgetItem(QTableWidgetItem):
    def __init__(self, number):
        QTableWidgetItem.__init__(self, number, QTableWidgetItem.UserType)
        self.__number = float(number)

    def __lt__(self, other):
        # pylint: disable=W0212
        return self.__number < other.__number


class DNSSimulation_view(DNSView):
    """
        Widget that lets user select DNS data directories
    """
    def __init__(self, parent):
        super(DNSSimulation_view, self).__init__(parent)
        self.name = "Simulation"
        self._content = load_ui(__file__, 'simulation.ui', baseinstance=self)
        self.cid = None

        ### connect signals
        self._content.pB_load_cif.clicked.connect(self.open_filename_dialog)
        self._content.pB_clear_cif.clicked.connect(self.clearfilename)
        self._content.dSB_a.valueChanged.connect(self.unitcellchanged)
        self._content.dSB_c.valueChanged.connect(self.unitcellchanged)
        self._content.dSB_b.valueChanged.connect(self.unitcellchanged)
        self._content.dSB_alpha.valueChanged.connect(self.unitcellchanged)
        self._content.dSB_beta.valueChanged.connect(self.unitcellchanged)
        self._content.dSB_gamma.valueChanged.connect(self.unitcellchanged)
        self._content.lE_Spacegroup.textEdited.connect(self.unitcellchanged)
        self._content.cB_inplane.toggled.connect(self.inplane_unique_switched)
        self._content.cB_unique.toggled.connect(self.inplane_unique_switched)
        self._content.dSB_sample_rot.valueChanged.connect(
            self.sample_rotchanged)
        self._content.lE_hkl1.textEdited.connect(self.clear_d_tooltip)
        self._content.lE_hkl2.textEdited.connect(self.clear_d_tooltip)
        self._content.lE_hkl2_p.textEdited.connect(self.clear_d_tooltip)
        self._content.cB_fix_omega.toggled.connect(self.fixomegaoffset)
        self._content.dSB_wavelength.valueChanged.connect(
            self.wavelength_changed)
        self._content.pB_ps.clicked.connect(self.powderplot_clicked)
        self._content.pB_sc.clicked.connect(self.scplot_clicked)
        self._content.pB.clicked.connect(self.calculate_clicked)

        self._content.table.setEditTriggers(
            QtWidgets.QTableWidget.NoEditTriggers)
        self._content.table.sortByColumn(3, 0)
        # sort after q unlesss user changes it

        self._mapping = {
            'powder_start': self._content.dSB_powder_start,
            'hkl2': self._content.lE_hkl2,
            'cif_filename': self._content.lE_cif_filename,
            'omega_offset': self._content.dSB_omega_offset,
            'spacegroup': self._content.lE_Spacegroup,
            'wavelength': self._content.dSB_wavelength,
            'det_rot': self._content.dSB_det_rot,
            'det_number': self._content.SB_det_number,
            'fix_omega': self._content.cB_fix_omega,
            'hkl1': self._content.lE_hkl1,
            'sc_det_start': self._content.dSB_sc_det_start,
            'sc_sam_end': self._content.dSB_sc_sam_end,
            'sc_det_end': self._content.dSB_sc_det_end,
            'beta': self._content.dSB_beta,
            'alpha': self._content.dSB_alpha,
            'unique': self._content.cB_unique,
            'hkl2_p': self._content.lE_hkl2_p,
            'a': self._content.dSB_a,
            'c': self._content.dSB_c,
            'b': self._content.dSB_b,
            'shift': self._content.dSB_shift,
            'powder_end': self._content.dSB_powder_end,
            'sample_rot': self._content.dSB_sample_rot,
            'inplane': self._content.cB_inplane,
            'sc_sam_start': self._content.dSB_sc_sam_start,
            'gamma': self._content.dSB_gamma,
            'labels': self._content.cB_labels,
        }

        #setting up matplotlib widget
        self.sc_layout = self._content.sc_plot_layout
        self.sc_static_canvas = FigureCanvas(Figure(figsize=(5, 3), dpi=200))
        self.sc_static_canvas.setSizePolicy(QtWidgets.QSizePolicy.Expanding,
                                            QtWidgets.QSizePolicy.Expanding)
        self.sc_toolbar = NavigationToolbar(self.sc_static_canvas, self)
        self.sc_layout.addWidget(self.sc_toolbar)
        self.sc_layout.addWidget(self.sc_static_canvas)
        self.powd_layout = self._content.powd_plot_layout
        self.powd_static_canvas = FigureCanvas(Figure(figsize=(5, 3), dpi=200))
        self.powd_static_canvas.setSizePolicy(QtWidgets.QSizePolicy.Expanding,
                                              QtWidgets.QSizePolicy.Expanding)
        self.powd_toolbar = NavigationToolbar(self.powd_static_canvas, self)
        self.powd_layout.addWidget(self.powd_toolbar)
        self.powd_layout.addWidget(self.powd_static_canvas)

        ## Signals for MPL
        self.sc_static_canvas.mpl_connect('axes_enter_event', self.mouseonplot)
        self.sc_static_canvas.mpl_connect('axes_leave_event',
                                          self.mouseoutplot)
        self._mapping['labels'].toggled.connect(self.powderplot_clicked)
## custom signals for presenter

    sig_cif_set = Signal(str)
    sig_unitcell_changed = Signal()
    sig_inplane_unique_switched = Signal()
    sig_wavelength_changed = Signal()
    sig_mouse_pos_changed = Signal(float, float)
    sig_calculate_clicked = Signal()
    sig_powderplot_clicked = Signal()
    sig_scplot_clicked = Signal()
    sig_fixomegaoffset = Signal()
    sig_table_item_clicked = Signal(float, float)

    ### emitting custom signals for presenter
    def calculate_clicked(self):
        self.sig_calculate_clicked.emit()

    def fixomegaoffset(self, checked):
        self._mapping['omega_offset'].setEnabled(not checked)
        self.calculate_clicked()
        if checked:
            self._content.l_warning_ooset.setText('')
        else:
            self._content.l_warning_ooset.setText(
                'Warning: omega offset not set')
        return

    def inplane_unique_switched(self):
        self.sig_inplane_unique_switched.emit()

    def mouse_pos_changed(self, event):
        x = event.xdata
        y = event.ydata
        self.sig_mouse_pos_changed.emit(x, y)

    def powderplot_clicked(self):
        self.sig_powderplot_clicked.emit()

    def scplot_clicked(self):
        self.sig_scplot_clicked.emit()

    def tableitemdclicked(self, item):
        dr = float(
            self._content.table.item(item.row(),
                                     item.column() - 2).text())
        sr = float(item.text())
        self.sig_table_item_clicked.emit(dr, sr)
        return

    def unitcellchanged(self):
        self.sig_unitcell_changed.emit()

    def wavelength_changed(self):
        self.sig_wavelength_changed.emit()

## end signal emiting
    def clear_d_tooltip(self):
        """called if hkl changed but no new calculation of d was done"""
        self._mapping['hkl1'].setToolTip('')
        self._mapping['hkl2'].setToolTip('')
        self._mapping['hkl2_p'].setToolTip('')
        return

    def clearfilename(self):
        """removes cif filename"""
        self._content.lE_cif_filename.setText('')
        return

    def get_state(self):
        """
        returns a dictionary with the names of the widgets as keys and
        the values
        """
        state_dict = OrderedDict()
        for key, target_object in self._mapping.items():
            state_dict[key] = self.get_single_state(target_object)
        for hkl in ['hkl1', 'hkl2']:
            state_dict[hkl + '_v'] = sim_help.hkl_string_to_float_list(
                state_dict[hkl])
        return state_dict

    def open_filename_dialog(self):
        """open dialog to select ciffile and calls loadCif()"""
        options = QFileDialog.Options()
        options |= QFileDialog.DontUseNativeDialog
        file_name = QFileDialog.getOpenFileName(
            self,
            "QFileDialog.getOpenFileName()",
            "",
            "Cif Files (*.cif);;All Files (*)",
            options=options)
        if isinstance(
                file_name, tuple
        ):  ## Qt4 vs QT5 hack QT5 returns string, filter as second argument
            file_name = file_name[0]
        if file_name:
            self._content.lE_cif_filename.setText(file_name)
            self.sig_cif_set.emit(file_name)
        return

    def mouseonplot(self, event):
        #pylint: disable=unused-argument
        self.cid = self.sc_static_canvas.mpl_connect('motion_notify_event',
                                                     self.mouse_pos_changed)

    def mouseoutplot(self, event):
        #pylint: disable=unused-argument
        self.sc_static_canvas.mpl_disconnect(self.cid)

    def powderplot(self, x, y, refl_to_annotate):
        self.powd_static_canvas.figure.clear()
        ax = self.powd_static_canvas.figure.subplots()
        ax.plot(x, y, zorder=1)
        ax.set_xlabel('2 theta', fontsize=14)
        ax.set_ylabel("Intensity (M*F2)", fontsize=14)
        if self._mapping['labels'].isChecked():
            for i, hkl in enumerate(refl_to_annotate[1]):
                ax.annotate('  [{:4.2f}, {:4.2f}, {:4.2f}]'.format(*hkl),
                            (refl_to_annotate[0][i], refl_to_annotate[2][i]),
                            fontsize=10)
        self.powd_toolbar.update()
        ax.figure.canvas.draw()

    def sample_rotchanged(self, value):
        #pylint: disable=unused-argument
        """ if sample rot of identifiying reflection changes set offset = 0 """
        if not self._content.cB_fix_omega.isChecked():
            self._content.dSB_omega_offset.setValue(0)
        return

    def sc_plot(self, line, refls, maxI, minI, q1, q2):
        """ returns a simulation of dns single crystal measurement in sc tab"""
        self.sc_static_canvas.figure.clear()
        ax = self.sc_static_canvas.figure.subplots()
        ax.fill(line[:, 0], line[:, 1], zorder=1)
        cm = get_cmap('plasma')
        if refls.any():
            sc = ax.scatter(refls[:, 0],
                            refls[:, 1],
                            c=refls[:, 2],
                            vmin=minI,
                            vmax=maxI,
                            s=300,
                            cmap=cm,
                            zorder=20)
            for i in range(len(refls[:, 0])):
                ax.annotate(
                    '    {:.0f}\n    [{:5.2f}, {:5.2f}, {:5.2f}]'.format(
                        refls[i, 2], refls[i, 3], refls[i, 4],
                        refls[i, 5]), (refls[i, 0], refls[i, 1]),
                    fontsize=10,
                    zorder=200)

        ax.set_xlabel(q1, fontsize=14)
        ax.set_ylabel("[{0:5.3f}, {1:5.3f}, {2:5.3f}]".format(*q2),
                      fontsize=14)
        self.sc_toolbar.update()
        if refls.any():
            cb = self.sc_static_canvas.figure.colorbar(sc)
            cb.set_label('Intensity', fontsize=14)
            cb.ax.zorder = -1
        ax.grid(color='grey', linestyle=':', linewidth=1)
        #ax.set_axisbelow(True)
        ax.figure.canvas.draw()
        return

    def set_d_tooltip(self, d_hkl1, d_hkl2, d_hkl2_p):
        self._mapping['hkl1'].setToolTip('d: {0:.3f}'.format(d_hkl1))
        self._mapping['hkl2'].setToolTip('d: {0:.3f}'.format(d_hkl2))
        self._mapping['hkl2_p'].setToolTip('d: {0:.3f}'.format(d_hkl2_p))
        return

    def set_hkl_position_on_plot(self, hkl):
        self._content.l_show_hkl.setText(
            'hkl = [{:5.2f} , {:5.2f} , {:5.2f}]'.format(*hkl))

    def set_hkl2_p(self, q2_p):
        self.set_single_state(self._mapping['hkl2_p'],
                              "[{0:5.3f},{1:5.3f},{2:5.3f}]".format(*q2_p))

    def set_ki(self, ki):
        self._content.l_kival.setText("{0:.3f}".format(ki))
        return

    def set_omega_offset(self, offset):
        self.set_single_state(self._mapping['omega_offset'], offset)

    def set_spacegroup(self, spacegroup):
        self.set_single_state(self._mapping['spacegroup'], spacegroup)

    def set_unitcell(self, a, b, c, alpha, beta, gamma, spacegroup_symbol):
        """writes lattice parameters and spacegroup to gui"""
        self.set_single_state(self._mapping['a'], a)
        self.set_single_state(self._mapping['b'], b)
        self.set_single_state(self._mapping['c'], c)
        self.set_single_state(self._mapping['alpha'], alpha)
        self.set_single_state(self._mapping['beta'], beta)
        self.set_single_state(self._mapping['gamma'], gamma)
        self.set_single_state(self._mapping['spacegroup'], spacegroup_symbol)
        return

    ### should replace this with qtableview and custom qabstractitem model
    def writetable(self, refls, tthlimit):  #
        """writes a list of reflections to the table"""
        self._content.table.clearContents()
        self._content.table.setSortingEnabled(False)
        tablehead = [
            'h', 'k', 'l', 'q', 'd', 'tth', 'fs', 'M', 'diff', 'det_rot',
            'channel', 'sample_rot'
        ]
        self._content.table.setColumnCount(len(tablehead))

        self._content.table.setRowCount(len(refls))
        formatstring = [
            ' {0:.0f} ', ' {0:.0f} ', ' {0:.0f} ', ' {0:.2f} ', ' {0:.2f} ',
            ' {0:.2f} ', ' {0:.0f} ', ' {0:.0f} ', ' {0:.2f} ', ' {0:.2f} ',
            ' {0:.0f} ', ' {0:.2f} '
        ]
        row = 0
        for refl in refls:
            for col, head in enumerate(tablehead):
                cellinfo = MyTableWidgetItem(formatstring[col].format(
                    getattr(refl, head)))
                cellinfo.setTextAlignment(QtCore.Qt.AlignRight
                                          | QtCore.Qt.AlignVCenter)
                if tablehead[col] == 'M':
                    cellinfo.setToolTip(str(refl.equivalents))
                if refl.diff < tthlimit:
                    cellinfo.setBackground(QtGui.QColor(100, 200, 100))
                else:
                    cellinfo.setBackground(QtGui.QColor(255, 255, 255))
                self._content.table.setItem(row, col, cellinfo)
            row += 1
        self._content.table.resizeColumnsToContents()
        self._content.table.itemDoubleClicked.connect(self.tableitemdclicked)
        self._content.table.setSortingEnabled(True)
        return
