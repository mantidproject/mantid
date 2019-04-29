#!/usr/bin/python
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=line-too-long, invalid-name, unused-argument, unused-import, multiple-statements
# pylint: disable=attribute-defined-outside-init, protected-access, super-on-old-class, redefined-outer-name
# pylint: disable=too-many-statements, too-many-instance-attributes, too-many-locals, too-many-branches
# pylint: disable=too-many-public-methods

"""
This module contains a class to create a graphical user interface for PyChop.
"""

from __future__ import (absolute_import, division, print_function)
from six import string_types
import sys
import re
import numpy as np
import os
import warnings
import copy
from .Instruments import Instrument
from qtpy.QtCore import (QEventLoop, Qt)  # noqa
from qtpy.QtWidgets import (QAction, QCheckBox, QComboBox, QDialog, QFileDialog, QGridLayout, QHBoxLayout, QMenu, QLabel,
                            QLineEdit, QMainWindow, QMessageBox, QPushButton, QSizePolicy, QSpacerItem, QTabWidget,
                            QTextEdit, QVBoxLayout, QWidget)  # noqa
from MPLwidgets import FigureCanvasQTAgg as FigureCanvas
from MPLwidgets import NavigationToolbar2QT as NavigationToolbar
import matplotlib
from matplotlib.figure import Figure
from matplotlib.widgets import Slider


class PyChopGui(QMainWindow):
    """
    GUI Class using PyQT for PyChop to help users plan inelastic neutron experiments
    at spallation sources by calculating the resolution and flux at a given neutron energies.
    """

    instruments = {}
    choppers = {}
    minE = {}
    maxE = {}

    def __init__(self):
        super(PyChopGui, self).__init__()
        self.folder = os.path.dirname(sys.modules[self.__module__].__file__)
        for fname in os.listdir(self.folder):
            if fname.endswith('.yaml'):
                instobj = Instrument(os.path.join(self.folder, fname))
                self.instruments[instobj.name] = instobj
                self.choppers[instobj.name] = instobj.getChopperNames()
                self.minE[instobj.name] = max([instobj.emin, 0.01])
                self.maxE[instobj.name] = instobj.emax
        self.drawLayout()
        self.setInstrument(list(self.instruments.keys())[0])
        self.resaxes_xlim = 0
        self.qeaxes_xlim = 0
        self.isFramePlotted = 0

    def setInstrument(self, instname):
        """
        Defines the instrument parameters by the name of the instrument.
        """
        self.engine = self.instruments[str(instname)]
        self.tabs.setTabEnabled(self.tdtabID, False)
        self.widgets['ChopperCombo']['Combo'].clear()
        self.widgets['FrequencyCombo']['Combo'].clear()
        self.widgets['FrequencyCombo']['Label'].setText('Frequency')
        self.widgets['PulseRemoverCombo']['Combo'].clear()
        for item in self.choppers[str(instname)]:
            self.widgets['ChopperCombo']['Combo'].addItem(item)
        rep = self.engine.moderator.source_rep
        maxfreq = self.engine.chopper_system.max_frequencies
        # At the moment, the GUI only supports up to two independent frequencies
        if not hasattr(maxfreq, '__len__') or len(maxfreq) == 1:
            self.widgets['PulseRemoverCombo']['Combo'].hide()
            self.widgets['PulseRemoverCombo']['Label'].hide()
            for fq in range(rep, (maxfreq[0] if hasattr(maxfreq, '__len__') else maxfreq) + 1, rep):
                self.widgets['FrequencyCombo']['Combo'].addItem(str(fq))
            if hasattr(self.engine.chopper_system, 'frequency_names'):
                self.widgets['FrequencyCombo']['Label'].setText(self.engine.chopper_system.frequency_names[0])
        else:
            self.widgets['PulseRemoverCombo']['Combo'].show()
            self.widgets['PulseRemoverCombo']['Label'].show()
            if hasattr(self.engine.chopper_system, 'frequency_names'):
                for idx, chp in enumerate([self.widgets['FrequencyCombo']['Label'], self.widgets['PulseRemoverCombo']['Label']]):
                    chp.setText(self.engine.chopper_system.frequency_names[idx])
            for fq in range(rep, maxfreq[0] + 1, rep):
                self.widgets['FrequencyCombo']['Combo'].addItem(str(fq))
            for fq in range(rep, maxfreq[1] + 1, rep):
                self.widgets['PulseRemoverCombo']['Combo'].addItem(str(fq))
        if len(self.engine.chopper_system.choppers) > 1:
            self.widgets['MultiRepCheck'].setEnabled(True)
            self.tabs.setTabEnabled(self.tdtabID, True)
        else:
            self.widgets['MultiRepCheck'].setEnabled(False)
            self.widgets['MultiRepCheck'].setChecked(False)
        self.widgets['Chopper2Phase']['Edit'].hide()
        self.widgets['Chopper2Phase']['Label'].hide()
        if self.engine.chopper_system.isPhaseIndependent:
            self.widgets['Chopper2Phase']['Edit'].show()
            self.widgets['Chopper2Phase']['Label'].show()
            self.widgets['Chopper2Phase']['Edit'].setText(str(self.engine.chopper_system.defaultPhase[0]))
            self.widgets['Chopper2Phase']['Label'].setText(self.engine.chopper_system.phaseNames[0])
            # Special case for MERLIN - hide phase control from normal users
            if 'MERLIN' in str(instname) and not self.instSciAct.isChecked():
                self.widgets['Chopper2Phase']['Edit'].hide()
                self.widgets['Chopper2Phase']['Label'].hide()
        self.engine.setChopper(str(self.widgets['ChopperCombo']['Combo'].currentText()))
        self.engine.setFrequency(float(self.widgets['FrequencyCombo']['Combo'].currentText()))
        val = self.flxslder.val * self.maxE[self.engine.instname] / 100
        self.flxedt.setText('%3.2f' % (val))
        nframe = self.engine.moderator.n_frame if hasattr(self.engine.moderator, 'n_frame') else 1
        self.repfig_nframe_edit.setText(str(nframe))
        self.repfig_nframe_rep1only.setChecked(False)
        if hasattr(self.engine.chopper_system, 'default_frequencies'):
            cb = [self.widgets['FrequencyCombo']['Combo'], self.widgets['PulseRemoverCombo']['Combo']]
            for idx, freq in enumerate(self.engine.chopper_system.default_frequencies):
                cb[idx].setCurrentIndex([i for i in range(cb[idx].count()) if str(freq) in cb[idx].itemText(i)][0])
                if idx > 1:
                    break
        self.tabs.setTabEnabled(self.qetabID, False)
        if self.engine.has_detector and hasattr(self.engine.detector, 'tthlims'):
            self.tabs.setTabEnabled(self.qetabID, True)

    def setChopper(self, choppername):
        """
        Defines the Fermi chopper slit package type by name, or the disk chopper arrangement variant.
        """
        self.engine.setChopper(str(choppername))
        self.engine.setFrequency(float(self.widgets['FrequencyCombo']['Combo'].currentText()))
        # Special case for MERLIN - only enable multirep for 'G' chopper
        if 'MERLIN' in self.engine.instname:
            if 'G' in str(choppername):
                self.widgets['MultiRepCheck'].setEnabled(True)
                self.tabs.setTabEnabled(self.tdtabID, True)
                self.widgets['Chopper2Phase']['Edit'].setText('1500')
                self.widgets['Chopper2Phase']['Label'].setText('Disk chopper phase delay time')
                if self.instSciAct.isChecked():
                    self.widgets['Chopper2Phase']['Edit'].show()
                    self.widgets['Chopper2Phase']['Label'].show()
            else:
                self.widgets['MultiRepCheck'].setEnabled(False)
                self.widgets['MultiRepCheck'].setChecked(False)
                self.tabs.setTabEnabled(self.tdtabID, False)
                self.widgets['Chopper2Phase']['Edit'].hide()
                self.widgets['Chopper2Phase']['Label'].hide()

    def setFreq(self, freqtext=None, **kwargs):
        """
        Sets the chopper frequency(ies), in Hz.
        """
        freq_gui = float(self.widgets['FrequencyCombo']['Combo'].currentText())
        freq_in = kwargs['manual_freq'] if ('manual_freq' in kwargs.keys()) else freq_gui
        if len(self.engine.getFrequency()) > 1 and (not hasattr(freq_in, '__len__') or len(freq_in)==1):
            freqpr = float(self.widgets['PulseRemoverCombo']['Combo'].currentText())
            freq_in = [freq_in, freqpr]
        if not self.widgets['Chopper2Phase']['Label'].isHidden():
            chop2phase = self.widgets['Chopper2Phase']['Edit'].text()
            if isinstance(self.engine.chopper_system.defaultPhase[0], string_types):
                chop2phase = str(chop2phase)
            else:
                chop2phase = float(chop2phase) % (1e6 / self.engine.moderator.source_rep)
            self.engine.setFrequency(freq_in, phase=chop2phase)
        else:
            self.engine.setFrequency(freq_in)

    def setEi(self):
        """
        Sets the incident energy (or focused incident energy for multi-rep case).
        """
        try:
            eitxt = float(self.widgets['EiEdit']['Edit'].text())
            self.engine.setEi(eitxt)
            if self.eiPlots.isChecked():
                self.calc_callback()
        except ValueError:
            raise ValueError('No Ei specified, or Ei string not understood')

    def calc_callback(self):
        """
        Calls routines to calculate the resolution / flux and to update the Matplotlib graphs.
        """
        try:
            if self.engine.getChopper() is None:
                self.setChopper(self.widgets['ChopperCombo']['Combo'].currentText())
            self.setEi()
            self.setFreq()
            self.calculate()
            if self.errormess:
                idx = [i for i, ei in enumerate(self.eis) if np.abs(ei - self.engine.getEi()) < 1.e-4]
                if idx and self.flux[idx[0]] == 0:
                    raise ValueError(self.errormess)
                self.errormessage(self.errormess)
            self.plot_res()
            self.plot_frame()
            if self.instSciAct.isChecked():
                self.update_script()
        except ValueError as err:
            self.errormessage(err)
        self.plot_flux_ei()
        self.plot_flux_hz()

    def calculate(self):
        """
        Performs the resolution and flux calculations.
        """
        self.errormess = None
        if self.engine.getEi() is None:
            self.setEi()
        if self.widgets['MultiRepCheck'].isChecked():
            en = np.linspace(0, 0.95, 200)
            self.eis = self.engine.getAllowedEi()
            with warnings.catch_warnings(record=True) as w:
                warnings.simplefilter('always', UserWarning)
                self.res = self.engine.getMultiRepResolution(en)
                self.flux = self.engine.getMultiRepFlux()
                if len(w) > 0:
                    mess = [str(w[i].message) for i in range(len(w))]
                    self.errormess = '\n'.join([m for m in mess if 'tchop' in m])
        else:
            en = np.linspace(0, 0.95*self.engine.getEi(), 200)
            with warnings.catch_warnings(record=True) as w:
                warnings.simplefilter('always', UserWarning)
                self.res = self.engine.getResolution(en)
                self.flux = self.engine.getFlux()
                if len(w) > 0:
                    raise ValueError(w[0].message)

    def _set_overplot(self, overplot, axisname):
        axis = getattr(self, axisname)
        if overplot:
            if matplotlib.compare_versions('2.1.0',matplotlib.__version__):
                axis.hold(True)
        else:
            setattr(self, axisname+'_xlim', 0)
            axis.clear()
            axis.axhline(color='k')

    def plot_res(self):
        """
        Plots the resolution in the resolution tab
        """
        overplot = self.widgets['HoldCheck'].isChecked()
        multiplot = self.widgets['MultiRepCheck'].isChecked()
        self._set_overplot(overplot, 'resaxes')
        self._set_overplot(overplot, 'qeaxes')
        inst = self.engine.instname
        freq = self.engine.getFrequency()
        if hasattr(freq, '__len__'):
            freq = freq[0]
        if multiplot:
            if matplotlib.compare_versions('2.1.0',matplotlib.__version__):
                self.resaxes.hold(True)
            for ie, Ei in enumerate(self.eis):
                en = np.linspace(0, 0.95*Ei, 200)
                if any(self.res[ie]):
                    if not self.flux[ie]:
                        continue
                    line, = self.resaxes.plot(en, self.res[ie])
                    label_text = '%s_%3.2fmeV_%dHz_Flux=%fn/cm2/s' % (inst, Ei, freq, self.flux[ie])
                    line.set_label(label_text)
                    if self.tabs.isTabEnabled(self.qetabID):
                        self.plot_qe(Ei, label_text, hold=True)
                    self.resaxes_xlim = max(Ei, self.resaxes_xlim)
            if matplotlib.compare_versions('2.1.0',matplotlib.__version__):
                self.resaxes.hold(False)
        else:
            ei = self.engine.getEi()
            en = np.linspace(0, 0.95*ei, 200)
            line, = self.resaxes.plot(en, self.res)
            chopper = self.engine.getChopper()
            label_text = '%s_%s_%3.2fmeV_%dHz_Flux=%fn/cm2/s' % (inst, chopper, ei, freq, self.flux)
            line.set_label(label_text)
            if self.tabs.isTabEnabled(self.qetabID):
                self.plot_qe(ei, label_text, overplot)
            self.resaxes_xlim = max(ei, self.resaxes_xlim)
        self.resaxes.set_xlim([0, self.resaxes_xlim])
        self.resaxes.legend().draggable()
        self.resaxes.set_xlabel('Energy Transfer (meV)')
        self.resaxes.set_ylabel(r'$\Delta$E (meV FWHM)')
        self.rescanvas.draw()

    def plot_qe(self, Ei, label_text, hold=False):
        """ Plots the Q-E diagram """
        from scipy import constants
        E2q, meV2J = (2. * constants.m_n / (constants.hbar ** 2), constants.e / 1000.)
        en = np.linspace(-Ei / 5., Ei, 100)
        q2 = []
        for tth in self.engine.detector.tthlims:
            q = np.sqrt(E2q * (2 * Ei - en - 2 * np.sqrt(Ei * (Ei - en)) * np.cos(np.deg2rad(tth))) * meV2J) / 1e10
            q2.append(np.concatenate((np.flipud(q), q)))
        self._set_overplot(hold, 'qeaxes')
        self.qeaxes_xlim = max(np.max(q2), self.qeaxes_xlim)
        line, = self.qeaxes.plot(np.hstack(q2), np.concatenate((np.flipud(en), en)).tolist() * len(self.engine.detector.tthlims))
        line.set_label(label_text)
        self.qeaxes.set_xlim([0, self.qeaxes_xlim])
        self.qeaxes.legend().draggable()
        self.qeaxes.set_xlabel(r'$|Q| (\mathrm{\AA}^{-1})$')
        self.qeaxes.set_ylabel('Energy Transfer (meV)')
        self.qecanvas.draw()

    def plot_flux_ei(self, **kwargs):
        """
        Plots the flux vs Ei in the middle tab
        """
        inst = self.engine.instname
        chop = self.engine.getChopper()
        freq = self.engine.getFrequency()
        overplot = self.widgets['HoldCheck'].isChecked()
        if hasattr(freq, '__len__'):
            freq = freq[0]
        update = kwargs['update'] if 'update' in kwargs.keys() else False
        # Do not recalculate if all relevant parameters still the same.
        _, labels = self.flxaxes2.get_legend_handles_labels()
        searchStr = '([A-Z]+) "(.+)" ([0-9]+) Hz'
        tmpinst = []
        if (labels and (overplot or len(labels) == 1)) or update:
            for prevtitle in labels:
                prevInst, prevChop, prevFreq = re.search(searchStr, prevtitle).groups()
                if update:
                    tmpinst.append(copy.deepcopy(Instrument(self.instruments[prevInst], prevChop, float(prevFreq))))
                else:
                    if inst == prevInst and chop == prevChop and freq == float(prevFreq):
                        return
        ne = 25
        mn = self.minE[inst]
        mx = (self.flxslder.val/100)*self.maxE[inst]
        eis = np.linspace(mn, mx, ne)
        flux = eis*0
        elres = eis*0
        if update:
            self.flxaxes1.clear()
            self.flxaxes2.clear()
            if matplotlib.compare_versions('2.1.0',matplotlib.__version__):
                self.flxaxes1.hold(True)
                self.flxaxes2.hold(True)
            for ii, instrument in enumerate(tmpinst):
                for ie, ei in enumerate(eis):
                    with warnings.catch_warnings(record=True):
                        warnings.simplefilter('always', UserWarning)
                        flux[ie] = instrument.getFlux(ei)
                        elres[ie] = instrument.getResolution(0., ei)[0]
                self.flxaxes1.plot(eis, flux)
                line, = self.flxaxes2.plot(eis, elres)
                line.set_label(labels[ii])
        else:
            for ie, ei in enumerate(eis):
                with warnings.catch_warnings(record=True):
                    warnings.simplefilter('always', UserWarning)
                    flux[ie] = self.engine.getFlux(ei)
                    elres[ie] = self.engine.getResolution(0., ei)[0]
            if overplot:
                if matplotlib.compare_versions('2.1.0',matplotlib.__version__):
                    self.flxaxes1.hold(True)
                    self.flxaxes2.hold(True)
            else:
                self.flxaxes1.clear()
                self.flxaxes2.clear()
            self.flxaxes1.plot(eis, flux)
            line, = self.flxaxes2.plot(eis, elres)
            line.set_label('%s "%s" %d Hz' % (inst, chop, freq))
        self.flxaxes1.set_xlim([mn, mx])
        self.flxaxes2.set_xlim([mn, mx])
        self.flxaxes1.set_xlabel('Incident Energy (meV)')
        self.flxaxes1.set_ylabel('Flux (n/cm$^2$/s)')
        self.flxaxes1.set_xlabel('Incident Energy (meV)')
        self.flxaxes2.set_ylabel('Elastic Resolution FWHM (meV)')
        lg = self.flxaxes2.legend()
        lg.draggable()
        self.flxcanvas.draw()

    def update_slider(self, val=None):
        """
        Callback function for the x-axis slider of the flux tab
        """
        if val is None:
            val = float(self.flxedt.text()) / self.maxE[self.engine.instname] * 100
            if val < self.minE[self.engine.instname]:
                self.errormessage("Max Ei must be greater than %2.1f" % (self.minE[self.engine.instname]))
                val = (self.minE[self.engine.instname]+0.1) / self.maxE[self.engine.instname] * 100
            self.flxslder.set_val(val)
        else:
            val = self.flxslder.val * self.maxE[self.engine.instname] / 100
            self.flxedt.setText('%3.2f' % (val))
        self.plot_flux_ei(update=True)
        self.flxcanvas.draw()

    def plot_flux_hz(self):
        """
        Plots the flux vs freq in the middle tab
        """
        inst = self.engine.instname
        chop = self.engine.getChopper()
        ei = float(self.widgets['EiEdit']['Edit'].text())
        overplot = self.widgets['HoldCheck'].isChecked()
        # Do not recalculate if one of the plots has the same parametersc
        _, labels = self.frqaxes2.get_legend_handles_labels()
        searchStr = '([A-Z]+) "(.+)" Ei = ([0-9.-]+) meV'
        if labels and (overplot or len(labels) == 1):
            for prevtitle in labels:
                prevInst, prevChop, prevEi = re.search(searchStr, prevtitle).groups()
                if inst == prevInst and chop == prevChop and abs(ei-float(prevEi)) < 0.01:
                    return
        freq0 = self.engine.getFrequency()
        rep = self.engine.moderator.source_rep
        maxfreq = self.engine.chopper_system.max_frequencies
        freqs = range(rep, (maxfreq[0] if hasattr(maxfreq, '__len__') else maxfreq) + 1, rep)
        flux = np.zeros(len(freqs))
        elres = np.zeros(len(freqs))
        for ie, freq in enumerate(freqs):
            if hasattr(freq0, '__len__'):
                self.setFreq(manual_freq=[freq] + freq0[1:])
            else:
                self.setFreq(manual_freq=freq)
            with warnings.catch_warnings(record=True):
                warnings.simplefilter('always', UserWarning)
                flux[ie] = self.engine.getFlux(ei)
                elres[ie] = self.engine.getResolution(0., ei)[0]
        if overplot:
            if matplotlib.compare_versions('2.1.0',matplotlib.__version__):
                self.frqaxes1.hold(True)
                self.frqaxes2.hold(True)
        else:
            self.frqaxes1.clear()
            self.frqaxes2.clear()
        self.setFreq(manual_freq=freq0)
        self.frqaxes1.set_xlabel('Chopper Frequency (Hz)')
        self.frqaxes1.set_ylabel('Flux (n/cm$^2$/s)')
        line, = self.frqaxes1.plot(freqs, flux, 'o-')
        self.frqaxes1.set_xlim([0, np.max(freqs)])
        self.frqaxes2.set_xlabel('Chopper Frequency (Hz)')
        self.frqaxes2.set_ylabel('Elastic Resolution FWHM (meV)')
        line, = self.frqaxes2.plot(freqs, elres, 'o-')
        line.set_label('%s "%s" Ei = %5.3f meV' % (inst, chop, ei))
        lg = self.frqaxes2.legend()
        lg.draggable()
        self.frqaxes2.set_xlim([0, np.max(freqs)])
        self.frqcanvas.draw()

    def instSciCB(self):
        """
        Callback function for the "Instrument Scientist Mode" menu option
        """
        # MERLIN is a special case - want to hide ability to change phase from users
        if 'MERLIN' in self.engine.instname and 'G' in self.engine.getChopper():
            if self.instSciAct.isChecked():
                self.widgets['Chopper2Phase']['Edit'].show()
                self.widgets['Chopper2Phase']['Label'].show()
                self.widgets['Chopper2Phase']['Edit'].setText('1500')
                self.widgets['Chopper2Phase']['Label'].setText('Disk chopper phase delay time')
            else:
                self.widgets['Chopper2Phase']['Edit'].hide()
                self.widgets['Chopper2Phase']['Label'].hide()
        if self.instSciAct.isChecked():
            self.tabs.insertTab(self.scrtabID, self.scrtab, 'ScriptOutput')
            self.scrtab.show()
        else:
            self.tabs.removeTab(self.scrtabID)
            self.scrtab.hide()

    def errormessage(self, message):
        msg = QMessageBox()
        msg.setText(str(message))
        msg.setStandardButtons(QMessageBox.Ok)
        msg.exec_()

    def loadYaml(self):
        yaml_file = QFileDialog().getOpenFileName(self.mainWidget, 'Open Instrument YAML File', self.folder, 'Files (*.yaml)')
        if isinstance(yaml_file, tuple):
            yaml_file = yaml_file[0]
        yaml_file = str(yaml_file)
        new_folder = os.path.dirname(yaml_file)
        if new_folder != self.folder:
            self.folder = new_folder
        try:
            new_inst = Instrument(yaml_file)
        except (RuntimeError, AttributeError, ValueError) as err:
            self.errormessage(err)
        newname = new_inst.name
        if newname in self.instruments.keys() and not self.overwriteload.isChecked():
            overwrite, newname = self._ask_overwrite()
            if overwrite == 1:
                return
            elif overwrite == 0:
                newname = new_inst.name
        self.instruments[newname] = new_inst
        self.choppers[newname] = new_inst.getChopperNames()
        self.minE[newname] = max([new_inst.emin, 0.01])
        self.maxE[newname] = new_inst.emax
        self.updateInstrumentList()
        combo = self.widgets['InstrumentCombo']['Combo']
        idx = [i for i in range(combo.count()) if str(combo.itemText(i)) == newname]
        combo.setCurrentIndex(idx[0])
        self.setInstrument(newname)

    def _ask_overwrite(self):
        msg = QDialog()
        msg.setWindowTitle('Load overwrite')
        layout = QGridLayout()
        layout.addWidget(QLabel('Instrument %s already exists in memory. Overwrite this?'), 0, 0, 1, -1)
        buttons = [QPushButton(label) for label in ['Load and overwrite', 'Cancel Load', 'Load and rename to']]
        locations = [[1, 0], [1, 1], [2, 0]]
        self.overwrite_flag = 1

        def overwriteCB(idx):
            self.overwrite_flag = idx
            msg.accept()
        for idx, button in enumerate(buttons):
            button.clicked.connect(lambda _, idx=idx: overwriteCB(idx))
            layout.addWidget(button, locations[idx][0], locations[idx][1])
        newname = QLineEdit()
        newname.editingFinished.connect(lambda: overwriteCB(2))
        layout.addWidget(newname, 2, 1)
        msg.setLayout(layout)
        msg.exec_()
        newname = str(newname.text())
        if not newname or newname in self.instruments:
            self.errormessage('Invalid instrument name. Cancelling load.')
            self.overwrite_flag = 1
        return self.overwrite_flag, newname

    def updateInstrumentList(self):
        combo = self.widgets['InstrumentCombo']['Combo']
        old_instruments = [str(combo.itemText(i)) for i in range(combo.count())]
        new_instruments = [inst for inst in self.instruments if inst not in old_instruments]
        for inst in new_instruments:
            combo.addItem(inst)

    def plot_frame(self):
        """
        Plots the distance-time diagram in the right tab
        """
        if len(self.engine.chopper_system.choppers) > 1:
            self.engine.n_frame = int(self.repfig_nframe_edit.text())
            self.repaxes.clear()
            self.engine.plotMultiRepFrame(self.repaxes, first_rep=self.repfig_nframe_rep1only.isChecked())
            self.repcanvas.draw()

    def _gen_text_ei(self, ei, obj_in):
        obj = Instrument(obj_in)
        obj.setEi(ei)
        en = np.linspace(0, 0.95*ei, 10)
        try:
            flux = self.engine.getFlux()
            res = self.engine.getResolution(en)
        except ValueError as err:
            self.errormessage(err)
            raise ValueError(err)
        tsqvan, tsqdic, tsqmodchop = obj.getVanVar()
        v_mod, v_chop = tuple(np.sqrt(tsqmodchop[:2]) * 1e6)
        x0, _, x1, x2, _ = obj.chopper_system.getDistances()
        first_component = 'moderator'
        if x0 != tsqmodchop[2]:
            x0 = tsqmodchop[2]
            first_component = 'chopper 1'
        txt = '# ------------------------------------------------------------- #\n'
        txt += '# Ei = %8.2f meV\n' % (ei)
        txt += '# Flux = %8.2f n/cm2/s\n' % (flux)
        txt += '# Elastic resolution = %6.2f meV\n' % (res[0])
        txt += '# Time width at sample = %6.2f us, of which:\n' % (1e6*np.sqrt(tsqvan))
        for ky, val in list(tsqdic.items()):
            txt += '#     %20s : %6.2f us\n' % (ky, 1e6*np.sqrt(val))
        txt += '# %s distances:\n' % (obj.instname)
        txt += '#     x0 = %6.2f m (%s to Fermi)\n' % (x0, first_component)
        txt += '#     x1 = %6.2f m (Fermi to sample)\n' % (x1)
        txt += '#     x2 = %6.2f m (sample to detector)\n' % (x2)
        txt += '# Approximate inelastic resolution is given by:\n'
        txt += '#     dE = 2 * E2V * sqrt(ef**3 * t_van**2) / x2\n'
        txt += '#     where:  E2V = 4.373e-4 meV/(m/us) conversion from energy to speed\n'
        txt += '#             t_van**2 = (geom*t_mod)**2 + ((1+geom)*t_chop)**2\n'
        txt += '#             geom = (x1 + x2*(ei/ef)**1.5) / x0\n'
        txt += '#     and t_mod and t_chop are the moderator and chopper time widths at the\n'
        txt += '#     moderator and chopper positions (not at the sample as listed above).\n'
        txt += '# Which in this case is:\n'
        txt += '#     %.4e*sqrt(ef**3 * ( (%6.5f*(%.3f+%.3f*(ei/ef)**1.5))**2 \n' % (874.78672e-6/x2, v_mod, x1/x0, x2/x0)
        txt += '#                              + (%6.5f*(%.3f+%.3f*(ei/ef)**1.5))**2) )\n' % (v_chop, 1+x1/x0, x2/x0)
        txt += '#  EN (meV)   Full dE (meV)   Approx dE (meV)\n'
        for ii in range(len(res)):
            ef = ei-en[ii]
            approx = (874.78672e-6/x2)*np.sqrt(ef**3 * ((v_mod*((x1/x0)+(x2/x0)*(ei/ef)**1.5))**2
                                                        + (v_chop*(1+(x1/x0)+(x2/x0)*(ei/ef)**1.5))**2))
            txt += '%12.5f %12.5f %12.5f\n' % (en[ii], res[ii], approx)
        return txt

    def genText(self):
        """
        Generates text output of the resolution function versus energy transfer and other information.
        """
        multiplot = self.widgets['MultiRepCheck'].isChecked()
        obj = self.engine
        if obj.getChopper() is None:
            self.setChopper(self.widgets['ChopperCombo']['Combo'].currentText())
        if obj.getEi() is None:
            self.setEi()
        instname, chtyp, freqs, ei_in = tuple([obj.instname, obj.getChopper(), obj.getFrequency(), obj.getEi()])
        txt = '# ------------------------------------------------------------- #\n'
        txt += '# Chop calculation for instrument %s\n' % (instname)
        if obj.isFermi:
            txt += '#     with chopper %s at %3i Hz\n' % (chtyp, freqs[0])
        else:
            txt += '#     in %s mode with:\n' % (chtyp)
            freq_names = obj.chopper_system.frequency_names
            for idx in range(len(freq_names)):
                txt += '#     %s at %3i Hz\n' % (freq_names[idx], freqs[idx])
        txt += self._gen_text_ei(ei_in, obj)
        if multiplot:
            for ei in sorted(self.engine.getAllowedEi()):
                if np.abs(ei - ei_in) > 0.001:
                    txt += self._gen_text_ei(ei, obj)
        return txt

    def showText(self):
        """
        Creates a dialog to show the generated text output.
        """
        try:
            generatedText = self.genText()
        except ValueError:
            return
        self.txtwin = QDialog()
        self.txtedt = QTextEdit()
        self.txtbtn = QPushButton('OK')
        self.txtwin.layout = QVBoxLayout(self.txtwin)
        self.txtwin.layout.addWidget(self.txtedt)
        self.txtwin.layout.addWidget(self.txtbtn)
        self.txtbtn.clicked.connect(self.txtwin.deleteLater)
        self.txtedt.setText(generatedText)
        self.txtedt.setReadOnly(True)
        self.txtwin.setWindowTitle('Resolution information')
        self.txtwin.setWindowModality(Qt.ApplicationModal)
        self.txtwin.setAttribute(Qt.WA_DeleteOnClose)
        self.txtwin.setMinimumSize(400, 600)
        self.txtwin.resize(400, 600)
        self.txtwin.show()
        self.txtloop = QEventLoop()
        self.txtloop.exec_()

    def saveText(self):
        """
        Saves the generated text to a file (opens file dialog).
        """
        fname = QFileDialog.getSaveFileName(self, 'Open file', '')
        if isinstance(fname, tuple):
            fname = fname[0]
        fid = open(fname, 'w')
        fid.write(self.genText())
        fid.close()

    def update_script(self):
        """
        Updates the text window with information about the previous calculation.
        """
        if self.widgets['MultiRepCheck'].isChecked():
            out = self.engine.getMultiWidths()
            new_str = '\n'
            for ie, ee in enumerate(out['Eis']):
                res = out['Energy'][ie]
                percent = res / ee * 100
                chop_width = out['chopper'][ie]
                mod_width = out['moderator'][ie]
                new_str += 'Ei is %6.2f meV, resolution is %6.2f ueV, percentage resolution is %6.3f\n' % (ee, res * 1000, percent)
                new_str += 'FWHM at sample from chopper and moderator are %6.2f us, %6.2f us\n' % (chop_width, mod_width)
        else:
            ei =  self.engine.getEi()
            out = self.engine.getWidths()
            res = out['Energy']
            percent = res / ei * 100
            chop_width = out['chopper']
            mod_width = out['moderator']
            new_str = '\nEi is %6.2f meV, resolution is %6.2f ueV, percentage resolution is %6.3f\n' % (ei, res * 1000, percent)
            new_str += 'FWHM at sample from chopper and moderator are %6.2f us, %6.2f us\n' % (chop_width, mod_width)
        self.scredt.append(new_str)

    def onHelp(self):
        """
        Shows the help page
        """
        try:
            from pymantidplot.proxies import showCustomInterfaceHelp
            showCustomInterfaceHelp("PyChop")
        except ImportError:
            helpTxt = "PyChop is a tool to allow direct inelastic neutron\nscattering users to estimate the inelastic resolution\n"
            helpTxt += "and incident flux for a given spectrometer setting.\n\nFirst select the instrument, chopper settings and\n"
            helpTxt += "Ei, and then click 'Calculate and Plot'. Data for all\nthe graphs will be generated (may take 1-2s) and\n"
            helpTxt += "all graphs will be updated. If the 'Hold current plot'\ncheck box is ticked, additional settings will be\n"
            helpTxt += "overplotted on the existing graphs if they are\ndifferent from previous settings.\n\nMore in-depth help "
            helpTxt += "can be obtained from the\nMantid help pages."
            self.hlpwin = QDialog()
            self.hlpedt = QLabel(helpTxt)
            self.hlpbtn = QPushButton('OK')
            self.hlpwin.layout = QVBoxLayout(self.hlpwin)
            self.hlpwin.layout.addWidget(self.hlpedt)
            self.hlpwin.layout.addWidget(self.hlpbtn)
            self.hlpbtn.clicked.connect(self.hlpwin.deleteLater)
            self.hlpwin.setWindowTitle('Help')
            self.hlpwin.setWindowModality(Qt.ApplicationModal)
            self.hlpwin.setAttribute(Qt.WA_DeleteOnClose)
            self.hlpwin.setMinimumSize(370, 300)
            self.hlpwin.resize(370, 300)
            self.hlpwin.show()
            self.hlploop = QEventLoop()
            self.hlploop.exec_()

    def drawLayout(self):
        """
        Draws the GUI layout.
        """
        self.widgetslist = [
            ['pair', 'show', 'Instrument', 'combo', self.instruments, self.setInstrument, 'InstrumentCombo'],
            ['pair', 'show', 'Chopper', 'combo', '', self.setChopper, 'ChopperCombo'],
            ['pair', 'show', 'Frequency', 'combo', '', self.setFreq, 'FrequencyCombo'],
            ['pair', 'hide', 'Pulse remover chopper freq', 'combo', '', self.setFreq, 'PulseRemoverCombo'],
            ['pair', 'show', 'Ei', 'edit', '', self.setEi, 'EiEdit'],
            ['pair', 'hide', 'Chopper 2 phase delay time', 'edit', '5', self.setFreq, 'Chopper2Phase'],
            ['spacer'],
            ['single', 'show', 'Calculate and Plot', 'button', self.calc_callback, 'CalculateButton'],
            ['single', 'show', 'Hold current plot', 'check', lambda: None, 'HoldCheck'],
            ['single', 'show', 'Show multi-reps', 'check', lambda: None, 'MultiRepCheck'],
            ['spacer'],
            ['single', 'show', 'Show data ascii window', 'button', self.showText, 'ShowAsciiButton'],
            ['single', 'show', 'Save data as ascii', 'button', self.saveText, 'SaveAsciiButton']
        ]
        self.droplabels = []
        self.dropboxes = []
        self.singles = []
        self.widgets = {}

        self.leftPanel = QVBoxLayout()
        self.rightPanel = QVBoxLayout()
        self.tabs = QTabWidget(self)
        self.fullWindow = QGridLayout()
        for widget in self.widgetslist:
            if 'pair' in widget[0]:
                self.droplabels.append(QLabel(widget[2]))
                if 'combo' in widget[3]:
                    self.dropboxes.append(QComboBox(self))
                    self.dropboxes[-1].activated['QString'].connect(widget[5])
                    for item in widget[4]:
                        self.dropboxes[-1].addItem(item)
                    self.widgets[widget[-1]] = {'Combo':self.dropboxes[-1], 'Label':self.droplabels[-1]}
                elif 'edit' in widget[3]:
                    self.dropboxes.append(QLineEdit(self))
                    self.dropboxes[-1].returnPressed.connect(widget[5])
                    self.widgets[widget[-1]] = {'Edit':self.dropboxes[-1], 'Label':self.droplabels[-1]}
                else:
                    raise RuntimeError('Bug in code - widget %s is not recognised.' % (widget[3]))
                self.leftPanel.addWidget(self.droplabels[-1])
                self.leftPanel.addWidget(self.dropboxes[-1])
                if 'hide' in widget[1]:
                    self.droplabels[-1].hide()
                    self.dropboxes[-1].hide()
            elif 'single' in widget[0]:
                if 'check' in widget[3]:
                    self.singles.append(QCheckBox(widget[2], self))
                    self.singles[-1].stateChanged.connect(widget[4])
                elif 'button' in widget[3]:
                    self.singles.append(QPushButton(widget[2]))
                    self.singles[-1].clicked.connect(widget[4])
                else:
                    raise RuntimeError('Bug in code - widget %s is not recognised.' % (widget[3]))
                self.leftPanel.addWidget(self.singles[-1])
                if 'hide' in widget[1]:
                    self.singles[-1].hide()
                self.widgets[widget[-1]] = self.singles[-1]
            elif 'spacer' in widget[0]:
                self.leftPanel.addItem(QSpacerItem(0, 35))
            else:
                raise RuntimeError('Bug in code - widget class %s is not recognised.' % (widget[0]))

        # Right panel, matplotlib figures
        self.resfig = Figure()
        self.resfig.patch.set_facecolor('white')
        self.rescanvas = FigureCanvas(self.resfig)
        self.resaxes = self.resfig.add_subplot(111)
        self.resaxes.axhline(color='k')
        self.resaxes.set_xlabel('Energy Transfer (meV)')
        self.resaxes.set_ylabel(r'$\Delta$E (meV FWHM)')
        self.resfig_controls = NavigationToolbar(self.rescanvas, self)
        self.restab = QWidget(self.tabs)
        self.restabbox = QVBoxLayout()
        self.restabbox.addWidget(self.rescanvas)
        self.restabbox.addWidget(self.resfig_controls)
        self.restab.setLayout(self.restabbox)

        self.flxfig = Figure()
        self.flxfig.patch.set_facecolor('white')
        self.flxcanvas = FigureCanvas(self.flxfig)
        self.flxaxes1 = self.flxfig.add_subplot(121)
        self.flxaxes1.set_xlabel('Incident Energy (meV)')
        self.flxaxes1.set_ylabel('Flux (n/cm$^2$/s)')
        self.flxaxes2 = self.flxfig.add_subplot(122)
        self.flxaxes2.set_xlabel('Incident Energy (meV)')
        self.flxaxes2.set_ylabel('Elastic Resolution FWHM (meV)')
        self.flxfig_controls = NavigationToolbar(self.flxcanvas, self)
        self.flxsldfg = Figure()
        self.flxsldfg.patch.set_facecolor('white')
        self.flxsldcv = FigureCanvas(self.flxsldfg)
        self.flxsldax = self.flxsldfg.add_subplot(111)
        self.flxslder = Slider(self.flxsldax, 'Ei (meV)', 0, 100, valinit=100)
        self.flxslder.valtext.set_visible(False)
        self.flxslder.on_changed(self.update_slider)
        self.flxedt = QLineEdit()
        self.flxedt.setText('1000')
        self.flxedt.returnPressed.connect(self.update_slider)
        self.flxtab = QWidget(self.tabs)
        self.flxsldbox = QHBoxLayout()
        self.flxsldbox.addWidget(self.flxsldcv)
        self.flxsldbox.addWidget(self.flxedt)
        self.flxsldwdg = QWidget()
        self.flxsldwdg.setLayout(self.flxsldbox)
        sz = self.flxsldwdg.maximumSize()
        sz.setHeight(50)
        self.flxsldwdg.setMaximumSize(sz)
        self.flxtabbox = QVBoxLayout()
        self.flxtabbox.addWidget(self.flxcanvas)
        self.flxtabbox.addWidget(self.flxsldwdg)
        self.flxtabbox.addWidget(self.flxfig_controls)
        self.flxtab.setLayout(self.flxtabbox)

        self.frqfig = Figure()
        self.frqfig.patch.set_facecolor('white')
        self.frqcanvas = FigureCanvas(self.frqfig)
        self.frqaxes1 = self.frqfig.add_subplot(121)
        self.frqaxes1.set_xlabel('Chopper Frequency (Hz)')
        self.frqaxes1.set_ylabel('Flux (n/cm$^2$/s)')
        self.frqaxes2 = self.frqfig.add_subplot(122)
        self.frqaxes1.set_xlabel('Chopper Frequency (Hz)')
        self.frqaxes2.set_ylabel('Elastic Resolution FWHM (meV)')
        self.frqfig_controls = NavigationToolbar(self.frqcanvas, self)
        self.frqtab = QWidget(self.tabs)
        self.frqtabbox = QVBoxLayout()
        self.frqtabbox.addWidget(self.frqcanvas)
        self.frqtabbox.addWidget(self.frqfig_controls)
        self.frqtab.setLayout(self.frqtabbox)

        self.repfig = Figure()
        self.repfig.patch.set_facecolor('white')
        self.repcanvas = FigureCanvas(self.repfig)
        self.repaxes = self.repfig.add_subplot(111)
        self.repaxes.axhline(color='k')
        self.repaxes.set_xlabel(r'TOF ($\mu$sec)')
        self.repaxes.set_ylabel('Distance (m)')
        self.repfig_controls = NavigationToolbar(self.repcanvas, self)
        self.repfig_nframe_label = QLabel('Number of frames to plot')
        self.repfig_nframe_edit = QLineEdit('1')
        self.repfig_nframe_button = QPushButton('Replot')
        self.repfig_nframe_button.clicked.connect(lambda: self.plot_frame())
        self.repfig_nframe_rep1only = QCheckBox('First Rep Only')
        self.repfig_nframe_box = QHBoxLayout()
        self.repfig_nframe_box.addWidget(self.repfig_nframe_label)
        self.repfig_nframe_box.addWidget(self.repfig_nframe_edit)
        self.repfig_nframe_box.addWidget(self.repfig_nframe_button)
        self.repfig_nframe_box.addWidget(self.repfig_nframe_rep1only)
        self.reptab = QWidget(self.tabs)
        self.repfig_nframe = QWidget(self.reptab)
        self.repfig_nframe.setLayout(self.repfig_nframe_box)
        self.repfig_nframe.setSizePolicy(QSizePolicy(QSizePolicy.Preferred, QSizePolicy.Fixed))
        self.reptabbox = QVBoxLayout()
        self.reptabbox.addWidget(self.repcanvas)
        self.reptabbox.addWidget(self.repfig_nframe)
        self.reptabbox.addWidget(self.repfig_controls)
        self.reptab.setLayout(self.reptabbox)

        self.qefig = Figure()
        self.qefig.patch.set_facecolor('white')
        self.qecanvas = FigureCanvas(self.qefig)
        self.qeaxes = self.qefig.add_subplot(111)
        self.qeaxes.axhline(color='k')
        self.qeaxes.set_xlabel(r'$|Q| (\mathrm{\AA}^{-1})$')
        self.qeaxes.set_ylabel('Energy Transfer (meV)')
        self.qefig_controls = NavigationToolbar(self.qecanvas, self)
        self.qetabbox = QVBoxLayout()
        self.qetabbox.addWidget(self.qecanvas)
        self.qetabbox.addWidget(self.qefig_controls)
        self.qetab = QWidget(self.tabs)
        self.qetab.setLayout(self.qetabbox)

        self.scrtab = QWidget(self.tabs)
        self.scredt = QTextEdit()
        self.scrcls = QPushButton("Clear")
        self.scrcls.clicked.connect(lambda: self.scredt.clear())
        self.scrbox = QVBoxLayout()
        self.scrbox.addWidget(self.scredt)
        self.scrbox.addWidget(self.scrcls)
        self.scrtab.setLayout(self.scrbox)
        self.scrtab.hide()

        self.tabs.addTab(self.restab, 'Resolution')
        self.tabs.addTab(self.flxtab, 'Flux-Ei')
        self.tabs.addTab(self.frqtab, 'Flux-Freq')
        self.tabs.addTab(self.reptab, 'Time-Distance')
        self.tdtabID = 3
        self.tabs.setTabEnabled(self.tdtabID, False)
        self.tabs.addTab(self.qetab, 'Q-E')
        self.qetabID = 4
        self.tabs.setTabEnabled(self.qetabID, False)
        self.scrtabID = 5
        self.rightPanel.addWidget(self.tabs)

        self.menuLoad = QMenu('Load')
        self.loadAct = QAction('Load YAML', self.menuLoad)
        self.loadAct.triggered.connect(self.loadYaml)
        self.menuLoad.addAction(self.loadAct)
        self.menuOptions = QMenu('Options')
        self.instSciAct = QAction('Instrument Scientist Mode', self.menuOptions, checkable=True)
        self.instSciAct.triggered.connect(self.instSciCB)
        self.menuOptions.addAction(self.instSciAct)
        self.eiPlots = QAction('Press Enter in Ei box updates plots', self.menuOptions, checkable=True)
        self.menuOptions.addAction(self.eiPlots)
        self.overwriteload = QAction('Always overwrite instruments in memory', self.menuOptions, checkable=True)
        self.menuOptions.addAction(self.overwriteload)
        self.menuBar().addMenu(self.menuLoad)
        self.menuBar().addMenu(self.menuOptions)

        self.leftPanelWidget = QWidget()
        self.leftPanelWidget.setLayout(self.leftPanel)
        self.leftPanelWidget.setSizePolicy(QSizePolicy(QSizePolicy.Fixed, QSizePolicy.Preferred))
        self.fullWindow.addWidget(self.leftPanelWidget, 0, 0)
        self.fullWindow.addLayout(self.rightPanel, 0, 1)
        self.helpbtn = QPushButton("?", self)
        self.helpbtn.setMaximumWidth(30)
        self.helpbtn.clicked.connect(self.onHelp)
        self.fullWindow.addWidget(self.helpbtn, 1, 0, 1, -1)

        self.mainWidget = QWidget()
        self.mainWidget.setLayout(self.fullWindow)
        self.setCentralWidget(self.mainWidget)
        self.setWindowTitle('PyChopGUI')
        self.show()
