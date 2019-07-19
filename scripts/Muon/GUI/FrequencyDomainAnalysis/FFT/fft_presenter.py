# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)


import mantid.simpleapi as mantid

from Muon.GUI.Common import thread_model


class FFTPresenter(object):

    """
    This class links the FFT model to the GUI
    """

    def __init__(self, view, alg, load):
        self.view = view
        self.alg = alg
        self.load = load
        self.thread = None
        # set data
        self.getWorkspaceNames()
        # connect
        self.view.tableClickSignal.connect(self.tableClicked)
        self.view.buttonSignal.connect(self.handleButton)
        self.view.phaseCheckSignal.connect(self.phaseCheck)

        if self.load.version == 2:
            self.view.setup_raw_checkbox_changed(self.handle_use_raw_data_changed)

    def cancel(self):
        if self.thread is not None:
            self.thread.cancel()

    def runChanged(self):
        self.getWorkspaceNames()

    @property
    def widget(self):
        return self.view

    # turn on button
    def activate(self):
        self.view.activateButton()

    # turn off button
    def deactivate(self):
        self.view.deactivateButton()

    def getWorkspaceNames(self):
        name = self.view.getInputWS()
        if self.load.version == 2:
            final_options = self.load.getWorkspaceNames(self.view.isRaw())
        else:
            final_options = self.load.getWorkspaceNames()
        self.view.addItems(final_options)

        if self.load.version == 2:
            self.view.removeRe("PhaseQuad")
            self.removePhaseFromIM(final_options)

            self.view.setReTo(name)

    def handle_use_raw_data_changed(self):
        if not self.view.isRaw() and not self.load.context._do_rebin():
            self.view.set_raw_checkbox_state(True)
            self.view.warning_popup('No rebin options specified')
            return

        self.getWorkspaceNames()

    def removePhaseFromIM(self, final_options):
        for option in final_options:
            if "PhaseQuad" in option:
                self.view.removeIm(option)

    # functions
    def phaseCheck(self):
        self.view.phaseQuadChanged()
        # check if a phase table exists
        if mantid.AnalysisDataService.doesExist("PhaseTable"):
            self.view.setPhaseBox()

    def tableClicked(self, row, col):
        if row == self.view.getImBoxRow() and col == 1 and "PhaseQuad" not in self.view.getWS():
            self.view.changedHideUnTick(
                self.view.getImBox(),
                self.view.getImBoxRow() + 1)
        elif row == self.view.getShiftBoxRow() and col == 1:
            self.view.changed(
                self.view.getShiftBox(),
                self.view.getShiftBoxRow() + 1)

    def createThread(self):
        return thread_model.ThreadModel(self.alg)

    # constructs the inputs for the FFT algorithms
    # then executes them (see fft_model to see the order
    # of execution
    def handleButton(self):
        if self.load.hasDataChanged():
            self.getWorkspaceNames()
            return
        # put this on its own thread so not to freeze Mantid
        self.thread = self.createThread()
        self.thread.threadWrapperSetUp(self.deactivate, self.handleFinished)

        # make some inputs
        inputs = {}
        inputs["Run"] = self.load.getRunName()

        # for new version 2 of FDA
        if self.load.version == 2:
            inputs["Run"] = self.getRun(
                self.view.getInputWS().split(";", 1)[0].split("_", 1)[0])
        # do apodization and padding to real data

        preInputs = self.view.initAdvanced()

        if "PhaseQuad" in self.view.getWS():
            phaseTable = {}
            phaseTable["newTable"] = self.view.isNewPhaseTable()
            phaseTable["FirstGoodData"] = self.view.getFirstGoodData()
            phaseTable["LastGoodData"] = self.view.getLastGoodData()
            phaseTable["Instrument"] = self.load.getInstrument()
            phaseTable["InputWorkspace"] = "MuonAnalysis"
            phaseTable['MaskedDetectors'] = []

            if self.load.version == 2:
                phaseTable["InputWorkspace"] = self.clean(
                    self.view.getInputWS())
                phaseTable['MaskedDetectors'] = self.load.get_detectors_excluded_from_default_grouping_tables()

            inputs["phaseTable"] = phaseTable
            self.view.RePhaseAdvanced(preInputs)
        else:
            self.view.ReAdvanced(preInputs)
            if self.view.isRaw() and self.load.version == 1:
                self.view.addRaw(preInputs, "InputWorkspace")
            if self.load.version == 2:
                preInputs["InputWorkspace"] = self.clean(
                    self.view.getInputWS())
        inputs["preRe"] = preInputs

        # do apodization and padding to complex data
        if self.view.isComplex() and "PhaseQuad" not in self.view.getWS():
            ImPreInputs = self.view.initAdvanced()
            self.view.ImAdvanced(ImPreInputs)
            if self.view.isRaw() and self.load.version == 1:
                self.view.addRaw(ImPreInputs, "InputWorkspace")

            if self.load.version == 2:
                ImPreInputs["InputWorkspace"] = self.clean(
                    self.view.getInputImWS())

            inputs["preIm"] = ImPreInputs

        # do FFT to transformed data
        FFTInputs = self.get_FFT_input()
        if "PhaseQuad" in self.view.getWS():
            self.view.getFFTRePhase(FFTInputs)
            if self.view.isComplex():
                self.view.getFFTImPhase(FFTInputs)
            if self.load.version == 2:
                FFTInputs["OutputWorkspace"] = self.getRun(
                    self.view.getInputWS()) + ";PhaseQuad;FFT"
        else:
            if self.load.version == 2:
                FFTInputs["OutputWorkspace"] = self.getRun(
                    self.view.getInputWS()) + ";FFT"
            if self.view.isRaw() and self.load.version == 1:
                self.view.addRaw(FFTInputs, "OutputWorkspace")
        inputs["FFT"] = FFTInputs
        try:
            self.thread.loadData(inputs)
            self.thread.start()
            self.view.setPhaseBox()
        except:
            pass

    # kills the thread at end of execution
    def handleFinished(self):
        self.activate()
        self.thread.deleteLater()
        self.thread = None

    def get_FFT_input(self):
        FFTInputs = {}
        if self.load.version == 2:
            FFTInputs = self.view.initFFTInput(
                self.clean(self.view.getInputWS()))
        else:
            FFTInputs = self.view.initFFTInput()

        if self.view.isAutoShift():
            FFTInputs["AutoShift"] = True
        else:
            self.view.addFFTShift(FFTInputs)
        if self.view.isComplex():
            self.view.addFFTComplex(FFTInputs)
        return FFTInputs

    def clean(self, name):
        if "PhaseQuad" in name and not self.load.version == 2:
            return self.getRun(name) + "_raw_data"
        elif "PhaseQuad" in name:
            return self.getRun(name)
        return name

    def getRun(self, name):
        return name.split(" (PhaseQuad)", 1)[0]
