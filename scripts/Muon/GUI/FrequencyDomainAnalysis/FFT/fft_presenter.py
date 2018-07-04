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

    def cancel(self):
        if self.thread is not None:
            self.thread.cancel()

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
        final_options = self.load.getWorkspaceNames()
        self.view.addItems(final_options)

    # functions
    def phaseCheck(self):
        self.view.phaseQuadChanged()
        # check if a phase table exists
        if mantid.AnalysisDataService.doesExist("PhaseTable"):
            self.view.setPhaseBox()

    def tableClicked(self, row, col):
        if row == self.view.getImBoxRow() and col == 1 and self.view.getWS() != "PhaseQuad":
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
        self.thread.threadWrapperSetUp(self.deactivate,self.handleFinished)

        # make some inputs
        inputs = {}
        inputs["Run"] = self.load.getRunName()

        # do apodization and padding to real data

        preInputs = self.view.initAdvanced()

        if self.view.getWS() == "PhaseQuad":
            phaseTable = {}
            phaseTable["newTable"] = self.view.isNewPhaseTable()
            phaseTable["FirstGoodData"] = self.view.getFirstGoodData()
            phaseTable["LastGoodData"] = self.view.getLastGoodData()
            phaseTable["Instrument"] = self.load.getInstrument()
            inputs["phaseTable"] = phaseTable
            self.view.RePhaseAdvanced(preInputs)
        else:
            self.view.ReAdvanced(preInputs)
            if self.view.isRaw():
                self.view.addRaw(preInputs, "InputWorkspace")
        inputs["preRe"] = preInputs

        # do apodization and padding to complex data
        if self.view.isComplex() and self.view.getWS() != "PhaseQuad":
            ImPreInputs = self.view.initAdvanced()
            self.view.ImAdvanced(ImPreInputs)
            if self.view.isRaw():
                self.view.addRaw(ImPreInputs, "InputWorkspace")
            inputs["preIm"] = ImPreInputs

        # do FFT to transformed data
        FFTInputs = self.get_FFT_input()
        if self.view.getWS() == "PhaseQuad":
            self.view.getFFTRePhase(FFTInputs)
            if self.view.isComplex():
                self.view.getFFTImPhase(FFTInputs)
        else:
            if self.view.isRaw():
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
        self.thread.threadWrapperTearDown(self.deactivate,self.handleFinished)
        self.thread.deleteLater()
        self.thread = None

    def get_FFT_input(self):
        FFTInputs = self.view.initFFTInput()
        if self.view.isAutoShift():
            FFTInputs["AutoShift"] = True
        else:
            self.view.addFFTShift(FFTInputs)
        if self.view.isComplex():
            self.view.addFFTComplex(FFTInputs)
        return FFTInputs
