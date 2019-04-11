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
        final_options = self.load.get_workspace_names_for_FFT_analysis(self.view.isRaw())

        self.view.addItems(final_options)
        self.view.removeRe('PhaseQuad')
        self.removePhaseFromIM(final_options)

        self.view.setReTo(name)

    def handle_use_raw_data_changed(self):
        if not self.view.isRaw() and not self.load._do_rebin():
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
        # put this on its own thread so not to freeze Mantid
        self.thread = self.createThread()
        self.thread.threadWrapperSetUp(self.deactivate, self.handleFinished)

        inputs = self.create_algorithm_inputs()

        try:
            self.thread.loadData(inputs)
            self.thread.start()
            self.view.setPhaseBox()
        except:
            pass

    def create_algorithm_inputs(self):
        # make some inputs
        inputs = {}

        inputs["Run"] = self.get_input_run()
        # do apodization and padding to real data

        inputs["preRe"] = self.get_pre_inputs()

        if "PhaseQuad" in self.view.getWS():
            inputs['phaseTable'] = self.get_phase_table_inputs()

        # do apodization and padding to complex data
        if self.view.isComplex() and "PhaseQuad" not in self.view.getWS():
            inputs['preIm'] = self.get_imaginary_inputs()

        # do FFT to transformed data
        inputs['FFT'] = self.get_fft_inputs()

        return inputs

    def get_input_run(self):
        return self.getRun(
            self.view.getInputWS().split(";", 1)[0].split("_", 1)[0])

    def get_pre_inputs(self):
        preInputs = self.view.initAdvanced()

        if "PhaseQuad" in self.view.getWS():
            preInputs['InputWorkspace'] = "__phaseQuad__"
            preInputs['OutputWorkspace'] = "__ReTmp__"
        else:
            preInputs['OutputWorkspace'] = "__ReTmp__"
            preInputs["InputWorkspace"] = self.clean(
                self.view.getInputWS())

        return preInputs

    def get_phase_table_inputs(self):
        phaseTable = {}
        phaseTable["newTable"] = self.view.isNewPhaseTable()
        phaseTable["FirstGoodData"] = self.view.getFirstGoodData()
        phaseTable["LastGoodData"] = self.view.getLastGoodData()
        phaseTable["Instrument"] = self.load.data_context.instrument
        phaseTable["InputWorkspace"] = self.clean(
            self.view.getInputWS())
        phaseTable['MaskedDetectors'] = self.load.get_detectors_excluded_from_default_grouping_tables()

        return phaseTable

    def get_imaginary_inputs(self):
        ImPreInputs = self.view.initAdvanced()
        ImPreInputs['OutputWorkspace'] = "__ImTmp__"
        ImPreInputs["InputWorkspace"] = self.clean(
            self.view.getInputImWS())

        return ImPreInputs

    def get_fft_inputs(self):
        FFTInputs = self.get_FFT_input()
        if "PhaseQuad" in self.view.getWS():
            self.view.getFFTRePhase(FFTInputs)
            if self.view.isComplex():
                self.view.getFFTImPhase(FFTInputs)

            FFTInputs["OutputWorkspace"] = self.getRun(
                self.view.getInputWS()) + ";PhaseQuad;FFT"
        else:
            FFTInputs["OutputWorkspace"] = self.getRun(
                self.view.getInputWS()) + ";FFT"

        return FFTInputs

    # kills the thread at end of execution
    def handleFinished(self):
        self.activate()
        self.thread.deleteLater()
        self.thread = None

    def get_FFT_input(self):
        FFTInputs = self.view.initFFTInput(
            self.clean(self.view.getInputWS()))

        if self.view.isAutoShift():
            FFTInputs["AutoShift"] = True
        else:
            self.view.addFFTShift(FFTInputs)
        if self.view.isComplex():
            self.view.addFFTComplex(FFTInputs)
        return FFTInputs

    def clean(self, name):
        if "PhaseQuad" in name:
            return self.getRun(name)
        return name

    def getRun(self, name):
        return name.split(" (PhaseQuad)", 1)[0]
