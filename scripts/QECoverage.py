# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=line-too-long, too-many-instance-attributes, invalid-name, missing-docstring, too-many-statements
# pylint: disable= too-many-branches, no-self-use
from __future__ import (absolute_import, division, print_function)
import sys

import numpy as np
import mantid
from PyQt4 import QtGui
from matplotlib.backends.backend_qt4agg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.backends.backend_qt4agg import NavigationToolbar2QT as NavigationToolbar
from matplotlib.figure import Figure
from scipy import constants


def calcQE(efix, tthlims, **kwargs):
    fc = np.sign(efix[0])
    efix = np.abs(efix)
    qe_array = []
    # Conversion factors
    E2q = 2. * constants.m_n / (constants.hbar ** 2)  # Energy to (neutron momentum)^2 (==2m_n/hbar^2)
    meV2J = constants.e / 1000.  # meV to Joules
    m2A = 1.e10  # metres to Angstrom
    for myE in efix:
        emax = -myE * 5
        if 'emax' in kwargs:
            emax = -kwargs['emax']
        elif 'emin' in kwargs:
            emax = kwargs['emin']
        Et0 = np.linspace(emax, myE, 200)
        q1 = np.sqrt(
            E2q * (2 * myE - Et0 - 2 * np.sqrt(myE * (myE - Et0)) * np.cos(np.deg2rad(tthlims[0]))) * meV2J) / m2A
        q1 = np.concatenate((np.flipud(q1), q1))
        Et = np.concatenate((np.flipud(fc * Et0), fc * Et0))
        for lines in range(1, len(tthlims)):
            q2 = np.sqrt(E2q * (
                2 * myE - Et0 - 2 * np.sqrt(myE * (myE - Et0)) * np.cos(np.deg2rad(tthlims[lines]))) * meV2J) / m2A
            q2 = np.concatenate((np.flipud(q2), q2))
            q1 = np.append(q1, q2)
            Et = np.append(Et, np.concatenate((np.flipud(fc * Et0), fc * Et0)))
        qe_array.append([q1, Et])
    return qe_array


class QECoverageGUI(QtGui.QWidget):
    """
    QECoverage - Calculates the Q(E) limits multi-detector spectrometers
    """

    # Initial Mantid Algorithm by Helen Walker (2015)
    # Rewritten as a Mantid interface by Duc Le (2016)

    def __init__(self, parent=None):
        QtGui.QWidget.__init__(self, parent)
        self.setWindowTitle("QECoverage")
        self.grid = QtGui.QVBoxLayout()
        self.setLayout(self.grid)
        self.mainframe = QtGui.QFrame(self)
        self.mainframe_grid = QtGui.QHBoxLayout()
        self.mainframe.setLayout(self.mainframe_grid)
        # Left panel - inputs
        self.tabs = QtGui.QTabWidget(self)
        #      Direct geometry spectrometer tab
        self.tab_direct = QtGui.QWidget(self.tabs)
        self.direct_grid = QtGui.QVBoxLayout()
        self.tab_direct.setLayout(self.direct_grid)
        self.direct_inst_list = ['LET', 'MAPS', 'MARI', 'MERLIN', 'ARCS', 'CNCS', 'HYSPEC', 'SEQUOIA',
                                 'IN4', 'IN5', 'IN6', 'FOCUS', 'MIBEMOL', 'DNS', 'TOFTOF']
        self.direct_inst_box = QtGui.QComboBox(self.tab_direct)
        for inst in self.direct_inst_list:
            self.direct_inst_box.addItem(inst)
        self.direct_grid.addWidget(self.direct_inst_box)
        self.direct_inst_box.activated[str].connect(self.onDirectInstActivated)
        self.direct_ei = QtGui.QFrame(self.tab_direct)
        self.direct_ei_grid = QtGui.QHBoxLayout()
        self.direct_ei.setLayout(self.direct_ei_grid)
        self.direct_ei_label = QtGui.QLabel("Ei", self.direct_ei)
        self.direct_ei_grid.addWidget(self.direct_ei_label)
        self.direct_ei_input = QtGui.QLineEdit("55", self.direct_ei)
        self.direct_ei_input.setToolTip("Incident Energy in meV")
        self.direct_ei_grid.addWidget(self.direct_ei_input)
        self.emptyfield_msgbox = QtGui.QMessageBox()
        self.emptyfield_msgbox.setText("Invalid input has been provided for Ei or Emin! Please try again")
        self.ei_msgbox = QtGui.QMessageBox()
        self.ei_msgbox.setText("Ei cannot be negative! Please try again")
        self.ei_emin_msgbox = QtGui.QMessageBox()
        self.ei_emin_msgbox.setText("Emin must be less than the values provided for Ei! Please try again")
        self.direct_grid.addWidget(self.direct_ei)
        self.emaxfield_msgbox = QtGui.QMessageBox()
        self.direct_plotover = QtGui.QCheckBox("Plot Over", self.tab_direct)
        self.direct_plotover.setToolTip("Hold this plot?")
        self.direct_grid.addWidget(self.direct_plotover)
        self.direct_plotover.stateChanged.connect(self.onDirectPlotOverChanged)
        self.direct_createws = QtGui.QCheckBox("Create Workspace", self.tab_direct)
        self.direct_createws.setToolTip("Create a Mantid workspace?")
        self.direct_grid.addWidget(self.direct_createws)
        self.direct_createws.stateChanged.connect(self.onDirectCreateWSChanged)
        self.direct_emin = QtGui.QFrame(self.tab_direct)
        self.direct_emin_grid = QtGui.QHBoxLayout()
        self.direct_emin.setLayout(self.direct_emin_grid)
        self.direct_emin_label = QtGui.QLabel("Emin", self.direct_emin)
        self.direct_emin_grid.addWidget(self.direct_emin_label)
        self.direct_emin_input = QtGui.QLineEdit("-10", self.direct_emin)
        self.direct_emin_input.setToolTip("Minimum energy transfer to plot down to.")
        self.direct_emin_grid.addWidget(self.direct_emin_input)
        self.direct_grid.addWidget(self.direct_emin)
        self.direct_plotbtn = QtGui.QPushButton("Plot Q-E", self.tab_direct)
        self.direct_grid.addWidget(self.direct_plotbtn)
        self.direct_plotbtn.clicked.connect(self.onClickDirectPlot)
        self.direct_s2 = QtGui.QFrame(self.tab_direct)
        self.direct_s2_grid = QtGui.QHBoxLayout()
        self.direct_s2.setLayout(self.direct_s2_grid)
        self.direct_s2_label = QtGui.QLabel("s2", self.direct_s2)
        self.direct_s2_grid.addWidget(self.direct_s2_label)
        self.direct_s2_input = QtGui.QLineEdit("30", self.direct_s2)
        self.direct_s2_input.setToolTip("Scattering angle of middle of the HYSPEC detector bank.")
        self.direct_s2_grid.addWidget(self.direct_s2_input)
        self.direct_s2_input.textChanged[str].connect(self.onS2Changed)
        self.direct_grid.addWidget(self.direct_s2)
        self.direct_s2.hide()
        self.direct_grid.addStretch(10)
        self.tabs.addTab(self.tab_direct, "Direct")
        self.tthlims = [2.65, 140]
        #      Indirect geometry spectrometer tab
        self.tab_indirect = QtGui.QWidget(self.tabs)
        self.indirect_grid = QtGui.QVBoxLayout()
        self.tab_indirect.setLayout(self.indirect_grid)
        self.indirect_inst_list = ['IRIS', 'OSIRIS', 'TOSCA', 'VESUVIO', 'BASIS', 'VISION']
        self.indirect_inst_box = QtGui.QComboBox(self.tab_indirect)
        for inst in self.indirect_inst_list:
            self.indirect_inst_box.addItem(inst)
        self.indirect_grid.addWidget(self.indirect_inst_box)
        self.indirect_inst_box.activated[str].connect(self.onIndirectInstActivated)
        self.indirect_ef = QtGui.QFrame(self.tab_indirect)
        self.indirect_ef_grid = QtGui.QHBoxLayout()
        self.indirect_ef.setLayout(self.indirect_ef_grid)
        self.indirect_ef_label = QtGui.QLabel("Ef", self.indirect_ef)
        self.indirect_ef_grid.addWidget(self.indirect_ef_label)
        self.indirect_ef_input = QtGui.QComboBox(self.indirect_ef)
        self.indirect_analysers = \
            {'IRIS': {'PG002': 1.84, 'PG004': 7.38, 'Mica002': 0.207, 'Mica004': 0.826, 'Mica006': 1.86},
             'OSIRIS': {'PG002': 1.84, 'PG004': 7.38},
             # Assuming the PG analysers scatter through 90deg (theta=45deg)
             'TOSCA': {'PG002': 3.634},
             'VESUVIO': {'AuFoil': 4897},
             'BASIS': {'Si111': 2.08, 'Si311': 7.64},
             # From IDF - implies analyser scattering angle is 90deg
             'VISION': {'PG002': 3.64}}
        for ana in self.indirect_analysers[str(self.indirect_inst_box.currentText())]:
            self.indirect_ef_input.addItem(ana)
        self.indirect_ef_grid.addWidget(self.indirect_ef_input)
        self.indirect_ef_input.activated[str].connect(self.onIndirectEfActivated)
        self.indirect_grid.addWidget(self.indirect_ef)
        self.indirect_plotover = QtGui.QCheckBox("Plot Over", self.tab_indirect)
        self.indirect_plotover.setToolTip("Hold this plot?")
        self.indirect_grid.addWidget(self.indirect_plotover)
        self.indirect_plotover.stateChanged.connect(self.onIndirectPlotOverChanged)
        self.indirect_createws = QtGui.QCheckBox("Create Workspace", self.tab_indirect)
        self.indirect_createws.setToolTip("Create a Mantid workspace?")
        self.indirect_grid.addWidget(self.indirect_createws)
        self.indirect_createws.stateChanged.connect(self.onIndirectCreateWSChanged)
        self.indirect_emax = QtGui.QFrame(self.tab_direct)
        self.indirect_emax = QtGui.QFrame(self.tab_indirect)
        self.indirect_emax_grid = QtGui.QHBoxLayout()
        self.indirect_emax.setLayout(self.indirect_emax_grid)
        self.indirect_emax_label = QtGui.QLabel("Emax", self.indirect_emax)
        self.indirect_emax_grid.addWidget(self.indirect_emax_label)
        self.indirect_emax_input = QtGui.QLineEdit("10", self.indirect_emax)
        self.indirect_emax_input.setToolTip("Max energy loss to plot up to.")
        self.indirect_emax_grid.addWidget(self.indirect_emax_input)
        self.indirect_grid.addWidget(self.indirect_emax)
        self.indirect_plotbtn = QtGui.QPushButton("Plot Q-E", self.tab_indirect)
        self.indirect_grid.addWidget(self.indirect_plotbtn)
        self.indirect_plotbtn.clicked.connect(self.onClickIndirectPlot)
        self.indirect_grid.addStretch(10)
        self.tabs.addTab(self.tab_indirect, "Indirect")
        self.mainframe_grid.addWidget(self.tabs)
        self.tabs.currentChanged.connect(self.onTabChange)
        # Right panel, matplotlib figure to show Q-E
        self.figure_frame = QtGui.QFrame(self.mainframe)
        self.figure_grid = QtGui.QVBoxLayout()
        self.figure_frame.setLayout(self.figure_grid)
        self.figure = Figure()
        self.figure.patch.set_facecolor('white')
        self.canvas = FigureCanvas(self.figure)
        self.axes = self.figure.add_subplot(111)
        self.axes.axhline(color='k')
        self.axes.set_xlabel(r'$|Q|$ ($\AA^{-1}$)')
        self.axes.set_ylabel('Energy Transfer (meV)')
        self.mainframe_grid.addWidget(self.canvas)
        self.figure_grid.addWidget(self.canvas)
        self.figure_controls = NavigationToolbar(self.canvas, self.figure_frame)
        self.figure_grid.addWidget(self.figure_controls)
        self.mainframe_grid.addWidget(self.figure_frame)
        self.grid.addWidget(self.mainframe)
        self.helpbtn = QtGui.QPushButton("?", self)
        self.helpbtn.setMaximumWidth(30)
        self.helpbtn.clicked.connect(self.onHelp)
        self.grid.addWidget(self.helpbtn)
        # Matplotlib does seem to rescale x-axis properly after axes.clear()
        self.xlim = 0

        #register startup
        mantid.UsageService.registerFeatureUsage("Interface","QECoverage",False)

    def onHelp(self):
        from pymantidplot.proxies import showCustomInterfaceHelp
        showCustomInterfaceHelp("QE Coverage")

    def onDirectPlotOverChanged(self, state):
        self.indirect_plotover.setCheckState(state)

    def onIndirectPlotOverChanged(self, state):
        self.direct_plotover.setCheckState(state)

    def onDirectCreateWSChanged(self, state):
        self.indirect_createws.setCheckState(state)

    def onIndirectCreateWSChanged(self, state):
        self.direct_createws.setCheckState(state)

    def onDirectInstActivated(self, Inst):
        self.direct_s2.hide()
        if Inst == 'LET':
            self.tthlims = [2.65, 140]
        elif Inst == 'MAPS':
            self.tthlims = [3.0, 19.8, 21.1, 29.8, 31.1, 39.8, 41.1, 49.8, 51.1, 59.8]
        elif Inst == 'MARI':
            self.tthlims = [3.43, 29.14, 30.86, 44.14, 45.86, 59.15, 60.86, 74.14, 75.86, 89.14, 90.86, 104.14, 105.86,
                            119.14, 120.86, 134.14]
        elif Inst == 'MERLIN':
            self.tthlims = [2.838, 135.69]
        elif Inst == 'ARCS':
            self.tthlims = [2.373, 135.955]
        elif Inst == 'CNCS':
            self.tthlims = [3.806, 132.609]
        # HYSPEC special case - detectors can rotate about sample. Coverage is approximately +/-30deg either
        #  side of center.
        elif Inst == 'HYSPEC':
            self.tthlims = [0, 60]
            # reset s2
            self.direct_s2_input.setText("30")
            self.direct_s2.show()
        elif Inst == 'SEQUOIA':
            self.tthlims = [1.997, 61.926]
        elif Inst == 'IN4':
            self.tthlims = [2.435, 8.738, 13.075, 120.96]
        elif Inst == 'IN5':
            self.tthlims = [0.372, 134.817]
        elif Inst == 'IN6':
            self.tthlims = [10.323, 115.048]
        elif Inst == 'FOCUS':
            self.tthlims = [9.64, 129.4]
        elif Inst == 'MIBEMOL':
            self.tthlims = [23.5, 147.157]
        # DNS information from web page because only polarised IDF available in Mantid.
        elif Inst == 'DNS':
            self.tthlims = [0, 135]
        elif Inst == 'TOFTOF':
            self.tthlims = [7.591, 140.194]

    def onIndirectInstActivated(self, Inst):
        self.indirect_ef_input.clear()
        for ana in self.indirect_analysers[str(Inst)]:
            self.indirect_ef_input.addItem(ana)
        # IRIS has separate PG and Mica analysers in different positions. Default is PG.
        if Inst == 'IRIS':
            self.tthlims = [27.07, 158.4]
        elif Inst == 'OSIRIS':
            self.tthlims = [11.5, 148]
        # TOSCA is more complicated than here because different analysers select different energies...
        elif Inst == 'TOSCA':
            self.tthlims = [38.92, 140.84]
        elif Inst == 'VESUVIO':
            self.tthlims = [32.7, 163.5]
        elif Inst == 'BASIS':
            self.tthlims = [6.04, 161.2]
        elif Inst == 'VISION':
            self.tthlims = [81.5, 98.5]

    def onIndirectEfActivated(self, Ana):
        Inst = self.indirect_inst_box.currentText()
        if Inst == 'IRIS' and str(Ana).startswith('PG'):
            self.tthlims = [27.07, 158.4]
        elif Inst == 'IRIS' and str(Ana).startswith('Mica'):
            self.tthlims = [21.7, 158.02]

    def onTabChange(self):
        if self.tabs.currentIndex() == 0:
            self.onDirectInstActivated(self.direct_inst_box.currentText())
        else:
            self.onIndirectInstActivated(self.indirect_inst_box.currentText())

    def onS2Changed(self, text):
        try:
            s2 = float(text)
        except ValueError:
            s2 = 0
        if abs(s2) <= 30:
            self.tthlims = [0, abs(s2) + 30]
        else:
            self.tthlims = [abs(s2) - 30, abs(s2) + 30]

    def onClickDirectPlot(self):
        overplot = self.direct_plotover.isChecked()
        createws = self.direct_createws.isChecked()
        ei_str = self.direct_ei_input.text()
        eierr = '-----------------------------------------------------------------------------------\n'
        eierr += 'Error: Invalid input Ei. This must be a number or a comma-separated list of numbers\n'
        eierr += '-----------------------------------------------------------------------------------\n'
        ei_vec = []
        if ',' not in ei_str:
            try:
                ei_vec.append(float(ei_str))
                ei_vec = self.direct_input_check(ei_vec)

            except ValueError:
                self.emptyfield_msgbox.show()
                raise ValueError(eierr)

        else:
            try:
                ei_vec = [float(val) for val in ei_str.split(',')]
                ei_vec = self.direct_input_check(ei_vec)

            except ValueError:
                self.emptyfield_msgbox.show()
                raise ValueError(eierr)

        try:
            Emin = float(self.direct_emin_input.text())
        except ValueError:
            Emin = -max(ei_vec)
        qe = calcQE(ei_vec, self.tthlims, emin=Emin)

        if not overplot:
            self.xlim = 0
            self.axes.clear()
            self.axes.axhline(color='k')
        self.axes.hold(True)
        Inst = self.direct_inst_box.currentText()
        for n in range(len(qe)):
            name = Inst + '_Ei=' + str(ei_vec[n])
            line, = self.axes.plot(qe[n][0], qe[n][1])
            line.set_label(name)
            if max(qe[n][0]) > self.xlim:
                self.xlim = max(qe[n][0])
            if createws:
                mantid.simpleapi.CreateWorkspace(DataX=qe[n][0], DataY=qe[n][1], NSpec=1,
                                                 OutputWorkspace=str('QECoverage_' + name))
        self.axes.set_xlim([0, self.xlim])
        self.axes.set_xlabel(r'$|Q|$ ($\AA^{-1}$)')
        self.axes.set_ylabel('Energy Transfer (meV)')
        self.axes.legend()
        self.canvas.draw()

    def onClickIndirectPlot(self):
        overplot = self.indirect_plotover.isChecked()
        createws = self.indirect_createws.isChecked()
        inst = str(self.indirect_inst_box.currentText())
        ana = str(self.indirect_ef_input.currentText())
        ef = self.indirect_analysers[inst][ana]

        try:
            self.indirect_input_check(ana)
            Emax = float(self.indirect_emax_input.text())
        except ValueError:
            Emax = abs(ef)
        qe = calcQE([-ef], self.tthlims, emax=Emax)
        if not overplot:
            self.xlim = 0
            self.axes.clear()
            self.axes.axhline(color='k')
        else:
            self.axes.hold(True)
        line, = self.axes.plot(qe[0][0], qe[0][1])
        line.set_label(inst + '_' + ana)
        if max(qe[0][0]) > self.xlim:
            self.xlim = max(qe[0][0])
        if createws:
            mantid.simpleapi.CreateWorkspace(DataX=qe[0][0], DataY=qe[0][1], NSpec=1,
                                             OutputWorkspace=str('QECoverage_' + inst + '_' + ana))
        self.axes.set_xlim([0, self.xlim])
        self.axes.set_xlabel(r'$|Q|$ ($\AA^{-1}$)')
        self.axes.set_ylabel('Energy Transfer (meV)')
        self.axes.legend()
        self.canvas.draw()

    def direct_input_check(self, ei_vec):

        if len(ei_vec) > 0:
            ei_str = ""
            update_ei_str = False
            insert_comma = True

            for ei in ei_vec:
                # if ei is equal to last value in ei_vec
                if ei_vec[len(ei_vec) - 1] == ei:
                    insert_comma = False

                if ei < 0:
                    update_ei_str = True
                    self.ei_msgbox.show()
                    ei = abs(ei)

                    ei_int = int(ei)
                    if insert_comma:
                        ei_str += str(ei_int) + ", "
                    else:
                        ei_str += str(ei_int)

                else:
                    ei_int = int(ei)
                    if insert_comma:
                        ei_str += str(ei_int) + ", "
                    else:
                        ei_str += str(ei_int)

            if update_ei_str:
                self.direct_ei_input.setText(ei_str)
                ei_vec = [float(val) for val in ei_str.split(',')]

            if self.direct_emin_input.text() == "":
                self.direct_emin_input.setText(str((ei_vec[0] / 2) * -1))

            # reset min_emin according to list provided
            min_emin = float(self.direct_emin_input.text())
            minimum = min(ei_vec)
            if minimum <= min_emin:
                self.ei_emin_msgbox.show()
                renew_val = str(minimum - 1)
                self.direct_emin_input.setText(renew_val)

        return ei_vec

    def indirect_input_check(self, ana):

        Emax_min = 0
        if ana == 'PG002' or ana == 'Mica006':
            Emax_min = -1
        elif ana == 'Si111':
            Emax_min = -2
        elif ana == 'PG004' or ana == 'Si311':
            Emax_min = -7
        elif ana == 'AuFoil':
            Emax_min = -4896

        self.emaxfield_msgbox.setText("Invalid input has been provided for Emax. "
                                      "Emax cannot be less than " +
                                      str(Emax_min) + ", when Ef is set as " +
                                      ana + "! Please try again.")

        if float(self.indirect_emax_input.text()) < Emax_min:
            self.indirect_emax_input.setText(str(Emax_min))
            self.emaxfield_msgbox.show()


def qapp():
    if QtGui.QApplication.instance():
        _app = QtGui.QApplication.instance()
    else:
        _app = QtGui.QApplication(sys.argv)
    return _app


app = qapp()
mainForm = QECoverageGUI()
mainForm.show()
app.exec_()
