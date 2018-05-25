#!/usr/bin/python
# pylint: disable=line-too-long, invalid-name, unused-argument, unused-import, multiple-statements
# pylint: disable=attribute-defined-outside-init, protected-access, super-on-old-class, redefined-outer-name
# pylint: disable=too-many-statements, too-many-instance-attributes, too-many-locals, too-many-branches
# pylint: disable=too-many-public-methods

"""
This module contains a class to create a graphical user interface for PyChop.
"""

from __future__ import (absolute_import, division, print_function)
import sys
import re
import numpy as np
from .Instruments import Instrument
from PyQt4 import QtGui, QtCore
from matplotlib.backends.backend_qt4agg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.backends.backend_qt4agg import NavigationToolbar2QT as NavigationToolbar
from matplotlib.figure import Figure
from matplotlib.widgets import Slider


class PyChopGui(QtGui.QMainWindow):
    """
    GUI Class using PyQT for PyChop to help users plan inelastic neutron experiments
    at spallation sources by calculating the resolution and flux at a given neutron energies.
    """

    instruments = {'MAPS': 'maps.yaml', 'MARI': 'mari.yaml', 'MERLIN': 'merlin.yaml', 'LET': 'let.yaml'}
    choppers = {
        'MAPS':['A', 'B', 'S'],
        'MARI':['A', 'B', 'R', 'G', 'S'],
        'MERLIN':['G', 'S'],
        'LET':['High Flux', 'Intermediate', 'High Resolution']
    }
    minE = {'MAPS':0.1, 'MARI':0.1, 'MERLIN':0.1, 'LET':0.1}
    maxE = {'MAPS':1000, 'MARI':1000, 'MERLIN':1000, 'LET':100}

    def setInstrument(self, instname):
        """
        Defines the instrument parameters by the name of the instrument.
        Allowed values: 'LET', 'MAPS', 'MARI', 'MERLIN' (case-insensitive)
        """
        self.tabs.setTabEnabled(self.tdtabID, False)
        self.widgets['ChopperCombo']['Combo'].clear()
        self.widgets['FrequencyCombo']['Combo'].clear()
        self.widgets['PulseRemoverCombo']['Combo'].clear()
        for item in self.choppers[str(instname)]:
            self.widgets['ChopperCombo']['Combo'].addItem(item)
        if 'LET' in str(instname):
            self.widgets['PulseRemoverCombo']['Combo'].show()
            self.widgets['PulseRemoverCombo']['Label'].show()
            self.widgets['PulseRemoverCombo']['Label'].setText('Pulse remover chopper freq')
            for fq in range(10, 301, 10):
                self.widgets['FrequencyCombo']['Combo'].addItem(str(fq))
            for fq in range(10, 151, 10):
                self.widgets['PulseRemoverCombo']['Combo'].addItem(str(fq))
        elif 'MAPS' in str(instname):
            self.widgets['PulseRemoverCombo']['Combo'].show()
            self.widgets['PulseRemoverCombo']['Label'].show()
            self.widgets['PulseRemoverCombo']['Label'].setText('Disk Chopper frequency')
            for fq in range(50, 601, 50):
                self.widgets['FrequencyCombo']['Combo'].addItem(str(fq))
            for fq in [50, 100]:
                self.widgets['PulseRemoverCombo']['Combo'].addItem(str(fq))
        else:
            self.widgets['PulseRemoverCombo']['Combo'].hide()
            self.widgets['PulseRemoverCombo']['Label'].hide()
            self.widgets['Chopper2Phase']['Edit'].hide()
            self.widgets['Chopper2Phase']['Label'].hide()
            for fq in range(50, 601, 50):
                self.widgets['FrequencyCombo']['Combo'].addItem(str(fq))
        if 'LET' in str(instname):
            self.widgets['MultiRepCheck'].setEnabled(True)
            self.tabs.setTabEnabled(self.tdtabID, True)
            self.widgets['Chopper2Phase']['Edit'].show()
            self.widgets['Chopper2Phase']['Label'].show()
            self.widgets['Chopper2Phase']['Edit'].setText('5')
            self.widgets['Chopper2Phase']['Label'].setText('Chopper 2 phase delay time')
        elif 'MERLIN' in str(instname):
            self.widgets['MultiRepCheck'].setEnabled(True)
            self.tabs.setTabEnabled(self.tdtabID, True)
            self.widgets['Chopper2Phase']['Edit'].setText('1500')
            self.widgets['Chopper2Phase']['Label'].setText('Disk chopper phase delay time')
            if self.instSciAct.isChecked():
                self.widgets['Chopper2Phase']['Edit'].show()
                self.widgets['Chopper2Phase']['Label'].show()
        elif 'MARI' in str(instname) or 'MAPS' in str(instname):
            self.widgets['MultiRepCheck'].setEnabled(True)
            self.tabs.setTabEnabled(self.tdtabID, True)
            self.widgets['Chopper2Phase']['Edit'].show()
            self.widgets['Chopper2Phase']['Label'].show()
            self.widgets['Chopper2Phase']['Edit'].setText('0')
            self.widgets['Chopper2Phase']['Label'].setText('Multirep mode')
        else:
            self.widgets['MultiRepCheck'].setEnabled(False)
            self.widgets['MultiRepCheck'].setChecked(False)
        self.engine.setInstrument(self.folder + self.instruments[str(instname)])
        self.engine.setChopper(str(self.widgets['ChopperCombo']['Combo'].currentText()))
        self.engine.setFrequency(float(self.widgets['FrequencyCombo']['Combo'].currentText()))
        val = self.flxslder.val * self.maxE[self.engine.instname] / 100
        self.flxedt.setText('%3.2f' % (val))

    def setChopper(self, choppername):
        """
        Defines the Fermi chopper slit package type by name, or the disk chopper arrangement variant.
        Allowed values: 'A', 'B', 'C', 'G', 'R', 'S', 'With Chopper 3', 'Without Chopper 3'
        Note that not all values apply to all instruments!
        """
        self.engine.setChopper(str(choppername))
        self.engine.setFrequency(float(self.widgets['FrequencyCombo']['Combo'].currentText()))
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
        if 'LET' in str(self.engine.instname):
            freqpr = float(self.widgets['PulseRemoverCombo']['Combo'].currentText())
            chop2phase = float(self.widgets['Chopper2Phase']['Edit'].text()) % 1e5
            self.engine.setFrequency([freq_in, freqpr], phase=chop2phase)
        elif 'MERLIN' in str(self.engine.instname) and 'G' in self.engine.getChopper():
            chop2phase = float(self.widgets['Chopper2Phase']['Edit'].text()) % 2e4
            self.engine.setFrequency(freq_in, phase=chop2phase)
        elif 'MAPS' in str(self.engine.instname): 
            freqpr = float(self.widgets['PulseRemoverCombo']['Combo'].currentText())
            chop2phase = str(self.widgets['Chopper2Phase']['Edit'].text())
            self.engine.setFrequency([freq_in, freqpr], phase=chop2phase)
        elif 'MARI' in str(self.engine.instname):
            chop2phase = str(self.widgets['Chopper2Phase']['Edit'].text())
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
        self.plot_flux_ei()
        self.plot_flux_hz()
        try:
            if self.engine.getChopper() is None:
                self.setChopper(self.widgets['ChopperCombo']['Combo'].currentText())
            self.setEi()
            self.setFreq()
            self.calculate()
            self.plot_res()
            self.plot_frame()
            if self.instSciAct.isChecked():
                self.update_script()
        except ValueError as err:
            msg = QtGui.QMessageBox()
            msg.setText(str(err))
            msg.setStandardButtons(QtGui.QMessageBox.Ok)
            msg.exec_()

    def calculate(self):
        """
        Performs the resolution and flux calculations.
        """
        if self.engine.getEi() is None:
            self.setEi()
        if self.widgets['MultiRepCheck'].isChecked():
            en = np.linspace(0, 0.95, 200)
            self.res = self.engine.getMultiRepResolution(en)
            self.flux = self.engine.getMultiRepFlux()
        else:
            en = np.linspace(0, 0.95*self.engine.getEi(), 200)
            self.res = self.engine.getResolution(en)
            self.flux = self.engine.getFlux()

    def plot_res(self):
        """
        Plots the resolution in the resolution tab
        """
        overplot = self.widgets['HoldCheck'].isChecked()
        multiplot = self.widgets['MultiRepCheck'].isChecked()
        if overplot:
            self.resaxes.hold(True)
        else:
            self.xlim = 0
            self.resaxes.clear()
            self.resaxes.axhline(color='k')
        if multiplot:
            Eis = self.engine.getAllowedEi()
            inst = self.engine.instname
            freq = self.engine.getFrequency()
            if hasattr(freq, '__len__'):
                freq = freq[0]
            self.resaxes.hold(True)
            for ie, Ei in enumerate(Eis):
                en = np.linspace(0, 0.95*Ei, 200)
                if any(self.res[ie]):
                    if not self.flux[ie]:
                        continue
                    line, = self.resaxes.plot(en, self.res[ie])
                    line.set_label('%s_%3.2fmeV_%dHz_Flux=%fn/cm2/s' % (inst, Ei, freq, self.flux[ie]))
                    if Ei > self.xlim:
                        self.xlim = Ei
            self.resaxes.hold(False)
        else:
            en = np.linspace(0, 0.95*self.engine.getEi(), 200)
            line, = self.resaxes.plot(en, self.res)
            inst = self.engine.instname
            chopper = self.engine.getChopper()
            ei = self.engine.getEi()
            freq = self.engine.getFrequency()
            if hasattr(freq, '__len__'):
                freq = freq[-1] if 'LET' in inst else freq[0]
            line.set_label('%s_%s_%3.2fmeV_%dHz_Flux=%fn/cm2/s' % (inst, chopper, ei, freq, self.flux))
            if ei > self.xlim:
                self.xlim = ei
        self.resaxes.set_xlim([0, self.xlim])
        lg = self.resaxes.legend()
        lg.draggable()
        self.resaxes.set_xlabel('Energy Transfer (meV)')
        self.resaxes.set_ylabel(r'$\Delta$E (meV FWHM)')
        self.rescanvas.draw()

    def plot_flux_ei(self, **kwargs):
        """
        Plots the flux vs Ei in the middle tab
        """
        inst = self.engine.instname
        chop = self.engine.getChopper()
        freq = self.engine.getFrequency()
        overplot = self.widgets['HoldCheck'].isChecked()
        if hasattr(freq, '__len__'):
            freq = freq[-1]
        update = kwargs['update'] if 'update' in kwargs.keys() else False
        # Do not recalculate if all relevant parameters still the same.
        _, labels = self.flxaxes2.get_legend_handles_labels()
        searchStr = '([A-Z]+) "([A-z ]+)" ([0-9]+) Hz'
        tmpinst = []
        if (labels and (overplot or len(labels) == 1)) or update:
            for prevtitle in labels:
                prevInst, prevChop, prevFreq = re.search(searchStr, prevtitle).groups()
                if update:
                    tmpinst.append(PyChop2(prevInst, prevChop, float(prevFreq)))
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
            self.flxaxes1.hold(True)
            self.flxaxes2.hold(True)
            for ii, instrument in enumerate(tmpinst):
                for ie, ei in enumerate(eis):
                    try:
                        flux[ie] = instrument.getFlux(ei)
                        elres[ie] = instrument.getResolution(0., ei)[0]
                    except ValueError:
                        pass
                self.flxaxes1.plot(eis, flux)
                line, = self.flxaxes2.plot(eis, elres)
                line.set_label(labels[ii])
        else:
            for ie, ei in enumerate(eis):
                try:
                    flux[ie] = self.engine.getFlux(ei)
                    elres[ie] = self.engine.getResolution(0., ei)[0]
                except ValueError:
                    pass
            if overplot:
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
                msg = QtGui.QMessageBox()
                msg.setText("Max Ei must be greater than %2.1f" % (self.minE[self.engine.instname]))
                msg.setStandardButtons(QtGui.QMessageBox.Ok)
                msg.exec_()
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
        searchStr = '([A-Z]+) "([A-z ]+)" Ei = ([0-9.-]+) meV'
        if labels and (overplot or len(labels) == 1):
            for prevtitle in labels:
                prevInst, prevChop, prevEi = re.search(searchStr, prevtitle).groups()
                if inst == prevInst and chop == prevChop and abs(ei-float(prevEi)) < 0.01:
                    return
        freq0 = self.engine.getFrequency()
        freqs = np.arange(10, 201, 10) if 'LET' in inst else np.arange(50, 601, 50)
        flux = np.zeros(len(freqs))
        elres = np.zeros(len(freqs))
        for ie, freq in enumerate(freqs):
            try:
                self.setFreq(manual_freq=freq)
                flux[ie] = self.engine.getFlux(ei)
                elres[ie] = self.engine.getResolution(0., ei)[0]
            except ValueError:
                pass
        if overplot:
            self.frqaxes1.hold(True)
            self.frqaxes2.hold(True)
        else:
            self.frqaxes1.clear()
            self.frqaxes2.clear()
        self.setFreq(manual_freq=(freq0[-1] if hasattr(freq0, '__len__') else freq0))
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
        if any([instname in self.engine.instname for instname in ['LET', 'MAPS', 'MARI']]):
            self.widgets['Chopper2Phase']['Edit'].show()   # Widget should show all the time for LET.
            self.widgets['Chopper2Phase']['Label'].show()
        elif self.instSciAct.isChecked() and 'MERLIN' in self.engine.instname and 'G' in self.engine.getChopper():
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

    def plot_frame(self):
        """
        Plots the distance-time diagram in the right tab
        """
        self.repaxes.clear()
        self.engine.plotMultiRepFrame(self.repaxes)
        self.repcanvas.draw()

    def genText(self):
        """
        Generates text output of the resolution function versus energy transfer and other information.
        """
        en = np.linspace(0, 0.95*self.engine.getEi(), 10)
        try:
            flux = self.engine.getFlux()
            res = self.engine.getResolution(en)
        except ValueError as err:
            msg = QtGui.QMessageBox()
            msg.setText(str(err))
            msg.setStandardButtons(QtGui.QMessageBox.Ok)
            msg.exec_()
            raise ValueError(err)
        obj = self.engine.getObject()
        instname, chtyp, freqs, ei_in = tuple([obj.instname, obj.getChopper(), obj.getFrequency(), obj.getEi()])
        if 'LET' in str(instname):
            eis = obj.getAllowedEi()
            enmr = np.linspace(0, 0.95, 10)
            resmr = obj.getMultiRepResolution(enmr)
            if not hasattr(resmr, '__len__'):
                resmr = [[resmr]]
            txt = '# ------------------------------------------------------------- #\n'
            txt += '# Resolution calculation for LET %s\n' % (chtyp)
            txt += '#   with the resolution chopper at %3i Hz,\n' % (freqs[-1])
            txt += '#   and the pulse remover chopper at %3i Hz\n' % (freqs[2])
            txt += '# ------------------------------------------------------------- #\n'
            txt += '# Ei = %6.2f meV\n' % (ei_in)
            txt += '#  EN (meV)   dE (meV)\n'
            for ii in range(len(res)):
                txt += '%12.5f %12.5f\n' % (en[ii], res[ii])
            txt += '# ------------------------------------------------------------- #\n'
            for ie, ei in enumerate(eis):
                if np.abs(ei-ei_in) > 0.1:
                    txt += '# Ei = %6.2f meV\n' % (ei)
                    txt += '#  EN (meV)   dE (meV)\n'
                    for ii in range(len(resmr[ie])):
                        txt += '%12.5f %12.5f\n' % (enmr[ii]*ei, resmr[ie][ii])
                    txt += '# ------------------------------------------------------------- #\n'
        else:
            ei = ei_in
            v_van, tmod, tchop = obj.getVanVar()
            x0, x1, x2 = tuple([obj._ISISFermi__Instruments[obj.instname][ii] for ii in [0, 2, 3]])
            geom = x1/x0 + x2/x0
            v_mod = np.sqrt(tmod**2 / geom**2)
            v_chop = np.sqrt(tchop**2 / (1+geom)**2)
            txt = '# ------------------------------------------------------------- #\n'
            txt += '# Chop calculation for %s ''%s'' Chopper at %3i Hz, Ei=%6.2f meV\n' % (instname, chtyp, freqs, ei)
            txt += '# ------------------------------------------------------------- #\n'
            txt += '# Flux = %8.2f n/cm2/s/150uAhrs\n' % (flux)
            txt += '# Elastic resolution = %6.2f meV\n' % (res[0])
            txt += '# Time width at detector = %6.2f us\n' % (1e6*np.sqrt(v_van))
            txt += '#     of which: Moderator width = %6.2f us\n' % (1e6*tmod)
            txt += '#             : Chopper width   = %6.2f us\n' % (1e6*tchop)
            txt += '# Time width at Fermi = %6.2f us\n' % (1e6*(v_mod + v_chop))
            txt += '#     of which: Moderator v_mod = %6.2f us\n' % (1e6*v_mod)
            txt += '#             : Chopper v_chop  = %6.2f us\n' % (1e6*v_chop)
            txt += '# %s distances:\n' % (instname)
            txt += '#     x0 = %6.2f m (moderator to Fermi)\n' % (x0)
            txt += '#     x1 = %6.2f m (Fermi to sample)\n' % (x1)
            txt += '#     x2 = %6.2f m (sample to detector)\n' % (x2)
            txt += '# Approximate inelastic resolution is given by:\n'
            txt += '#     dE = convfac * sqrt(en**3 * v_van2) / x2\n'
            txt += '#     where:  convfac = 2059.956975\n'
            txt += '#             v_van2 = (geom*v_mod)**2 + ((1+geom)*v_chop)**2\n'
            txt += '#             geom = x1/x0 + ((ei/ef)**1.5)*(x2/x0)\n'
            txt += '# Which in this case is:\n'
            txt += '#     %.2f*sqrt(ef**3 * ( %.5e*(%.3f+%.3f*(ei/ef)**1.5)**2 \n' % (2059.956975/x2, v_mod**2, x1/x0, x2/x0)
            txt += '#                         + %.5e*(%.3f+%.3f*(ei/ef)**1.5)**2) )\n' % (v_chop**2, 1+x1/x0, x2/x0)
            txt += '#  EN (meV)   Full dE (meV)   Approx dE (meV)\n'
            for ii in range(len(res)):
                ef = ei-en[ii]
                approx = (2059.956975/x2)*np.sqrt(ef**3 * ((v_mod**2)*((x1/x0)+(x2/x0)*(ei/ef)**1.5)**2
                                                           + (v_chop**2)*(1+(x1/x0)+(x2/x0)*(ei/ef)**1.5)**2))
                txt += '%12.5f %12.5f %12.5f\n' % (en[ii], res[ii], approx)
        return txt

    def showText(self):
        """
        Creates a dialog to show the generated text output.
        """
        try:
            generatedText = self.genText()
        except ValueError:
            return
        self.txtwin = QtGui.QDialog()
        self.txtedt = QtGui.QTextEdit()
        self.txtbtn = QtGui.QPushButton('OK')
        self.txtwin.layout = QtGui.QVBoxLayout(self.txtwin)
        self.txtwin.layout.addWidget(self.txtedt)
        self.txtwin.layout.addWidget(self.txtbtn)
        self.txtbtn.clicked.connect(self.txtwin.deleteLater)
        self.txtedt.setText(generatedText)
        self.txtedt.setReadOnly(True)
        self.txtwin.setWindowTitle('Resolution information')
        self.txtwin.setWindowModality(QtCore.Qt.ApplicationModal)
        self.txtwin.setAttribute(QtCore.Qt.WA_DeleteOnClose)
        self.txtwin.setMinimumSize(400, 600)
        self.txtwin.resize(400, 600)
        self.txtwin.show()
        self.txtloop = QtCore.QEventLoop()
        self.txtloop.exec_()

    def saveText(self):
        """
        Saves the generated text to a file (opens file dialog).
        """
        fname = QtGui.QFileDialog.getSaveFileName(self, 'Open file', '')
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
                chop_width = out['Chopper'][ie]
                mod_width = out['Moderator'][ie]
                new_str += 'Ei is %6.2f meV, resolution is %6.2f ueV, percentage resolution is %6.3f\n' % (ee, res * 1000, percent)
                new_str += 'FWHM at detectors from chopper and moderator are %6.2f us, %6.2f us\n' % (chop_width, mod_width)
        else:
            ei =  self.engine.getEi()
            out = self.engine.getWidths()
            res = out['Energy']
            percent = res / ei * 100
            chop_width = out['Chopper']
            mod_width = out['Moderator']
            new_str = '\nEi is %6.2f meV, resolution is %6.2f ueV, percentage resolution is %6.3f\n' % (ei, res * 1000, percent)
            new_str += 'FWHM at detectors from chopper and moderator are %6.2f us, %6.2f us\n' % (chop_width, mod_width)
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
            self.hlpwin = QtGui.QDialog()
            self.hlpedt = QtGui.QLabel(helpTxt)
            self.hlpbtn = QtGui.QPushButton('OK')
            self.hlpwin.layout = QtGui.QVBoxLayout(self.hlpwin)
            self.hlpwin.layout.addWidget(self.hlpedt)
            self.hlpwin.layout.addWidget(self.hlpbtn)
            self.hlpbtn.clicked.connect(self.hlpwin.deleteLater)
            self.hlpwin.setWindowTitle('Help')
            self.hlpwin.setWindowModality(QtCore.Qt.ApplicationModal)
            self.hlpwin.setAttribute(QtCore.Qt.WA_DeleteOnClose)
            self.hlpwin.setMinimumSize(370, 300)
            self.hlpwin.resize(370, 300)
            self.hlpwin.show()
            self.hlploop = QtCore.QEventLoop()
            self.hlploop.exec_()

    def dummy(self, text):
        """
        Does nothing.
        """
        pass

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
            ['single', 'show', 'Hold current plot', 'check', self.dummy, 'HoldCheck'],
            ['single', 'show', 'Show multi-reps', 'check', self.dummy, 'MultiRepCheck'],
            ['spacer'],
            ['single', 'show', 'Show data ascii window', 'button', self.showText, 'ShowAsciiButton'],
            ['single', 'show', 'Save data as ascii', 'button', self.saveText, 'SaveAsciiButton']
        ]
        self.droplabels = []
        self.dropboxes = []
        self.singles = []
        self.widgets = {}

        self.leftPanel = QtGui.QVBoxLayout()
        self.rightPanel = QtGui.QVBoxLayout()
        self.tabs = QtGui.QTabWidget(self)
        self.fullWindow = QtGui.QGridLayout()
        for widget in self.widgetslist:
            if 'pair' in widget[0]:
                self.droplabels.append(QtGui.QLabel(widget[2]))
                if 'combo' in widget[3]:
                    self.dropboxes.append(QtGui.QComboBox(self))
                    self.dropboxes[-1].activated['QString'].connect(widget[5])
                    for item in widget[4]:
                        self.dropboxes[-1].addItem(item)
                    self.widgets[widget[-1]] = {'Combo':self.dropboxes[-1], 'Label':self.droplabels[-1]}
                elif 'edit' in widget[3]:
                    self.dropboxes.append(QtGui.QLineEdit(self))
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
                    self.singles.append(QtGui.QCheckBox(widget[2], self))
                    self.singles[-1].stateChanged.connect(widget[4])
                elif 'button' in widget[3]:
                    self.singles.append(QtGui.QPushButton(widget[2]))
                    self.singles[-1].clicked.connect(widget[4])
                else:
                    raise RuntimeError('Bug in code - widget %s is not recognised.' % (widget[3]))
                self.leftPanel.addWidget(self.singles[-1])
                if 'hide' in widget[1]:
                    self.singles[-1].hide()
                self.widgets[widget[-1]] = self.singles[-1]
            elif 'spacer' in widget[0]:
                self.leftPanel.addItem(QtGui.QSpacerItem(0, 35))
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
        self.restab = QtGui.QWidget(self.tabs)
        self.restabbox = QtGui.QVBoxLayout()
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
        self.flxedt = QtGui.QLineEdit()
        self.flxedt.setText('1000')
        self.flxedt.returnPressed.connect(self.update_slider)
        self.flxtab = QtGui.QWidget(self.tabs)
        self.flxsldbox = QtGui.QHBoxLayout()
        self.flxsldbox.addWidget(self.flxsldcv)
        self.flxsldbox.addWidget(self.flxedt)
        self.flxsldwdg = QtGui.QWidget()
        self.flxsldwdg.setLayout(self.flxsldbox)
        sz = self.flxsldwdg.maximumSize()
        sz.setHeight(50)
        self.flxsldwdg.setMaximumSize(sz)
        self.flxtabbox = QtGui.QVBoxLayout()
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
        self.frqtab = QtGui.QWidget(self.tabs)
        self.frqtabbox = QtGui.QVBoxLayout()
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
        self.reptab = QtGui.QWidget(self.tabs)
        self.reptabbox = QtGui.QVBoxLayout()
        self.reptabbox.addWidget(self.repcanvas)
        self.reptabbox.addWidget(self.repfig_controls)
        self.reptab.setLayout(self.reptabbox)

        self.scrtab = QtGui.QWidget(self.tabs)
        self.scredt = QtGui.QTextEdit()
        self.scrcls = QtGui.QPushButton("Clear")
        self.scrcls.clicked.connect(lambda: self.scredt.clear())
        self.scrbox = QtGui.QVBoxLayout()
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
        self.scrtabID = 4
        self.rightPanel.addWidget(self.tabs)

        self.menuOptions = QtGui.QMenu('Options')
        self.instSciAct = QtGui.QAction('Instrument Scientist Mode', self.menuOptions, checkable=True)
        self.instSciAct.triggered.connect(self.instSciCB)
        self.menuOptions.addAction(self.instSciAct)
        self.eiPlots = QtGui.QAction('Press Enter in Ei box updates plots', self.menuOptions, checkable=True)
        self.menuOptions.addAction(self.eiPlots)
        self.menuBar().addMenu(self.menuOptions)

        self.fullWindow.addLayout(self.leftPanel, 0, 0)
        self.fullWindow.addLayout(self.rightPanel, 0, 1)
        self.helpbtn = QtGui.QPushButton("?", self)
        self.helpbtn.setMaximumWidth(30)
        self.helpbtn.clicked.connect(self.onHelp)
        self.fullWindow.addWidget(self.helpbtn, 1, 0, 1, -1)

        self.mainWidget = QtGui.QWidget()
        self.mainWidget.setLayout(self.fullWindow)
        self.setCentralWidget(self.mainWidget)
        self.setWindowTitle('PyChopGUI')
        self.show()

    def __init__(self):
        super(PyChopGui, self).__init__()
        import sys, os
        self.folder = os.path.dirname(sys.modules[self.__module__].__file__) + '/'
        self.engine = Instrument(self.folder+self.instruments['MAPS'], 'A', 50)
        self.drawLayout()
        self.setInstrument('MAPS')
        self.xlim = 0
        self.isFramePlotted = 0


def show():
    """
    Create a Qt window in Python, or interactively in IPython with Qt GUI
    event loop integration.
    """
    app_created = False
    app = QtCore.QCoreApplication.instance()
    if app is None:
        app = QtGui.QApplication(sys.argv)
        app_created = True
    app.references = set()
    window = PyChopGui()
    app.references.add(window)
    window.show()
    if app_created:
        app.exec_()
    return window


if __name__ == '__main__':
    if QtGui.QApplication.instance():
        app = QtGui.QApplication.instance()
    else:
        app = QtGui.QApplication(sys.argv)
    window = PyChopGui()
    window.show()
    try: # check if started from within mantidplot
        import mantidplot # noqa
    except ImportError:
        sys.exit(app.exec_())
