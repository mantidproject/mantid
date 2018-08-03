from __future__ import (absolute_import, division, print_function)


import math
from Muon.GUI.Common import thread_model


class MaxEntPresenter(object):

    """
    This class links the MaxEnt model to the GUI
    """

    def __init__(self, view, alg, load):
        self.view = view
        self.alg = alg
        self.load = load
        self.thread = None
        # set data
        self.getWorkspaceNames()
        # connect
        self.view.maxEntButtonSignal.connect(self.handleMaxEntButton)
        self.view.cancelSignal.connect(self.cancel)
        self.view.phaseSignal.connect(self.handlePhase)

    @property
    def widget(self):
        return self.view

    # functions
    def getWorkspaceNames(self):
        final_options = self.load.getGroupedWorkspaceNames()
        run = self.load.getRunName()
        self.view.setRun(run)
        final_options.append(run)
        self.view.addItems(final_options)
        start = int(
            math.ceil(math.log(self.load.getNPoints()) / math.log(2.0)))
        values = [str(2**k) for k in range(start, 21)]
        self.view.addNPoints(values)

    def cancel(self):
        if self.thread is not None:
            self.thread.cancel()

    # turn on button
    def activate(self):
        self.view.activateCalculateButton()

    # turn off button
    def deactivate(self):
        self.view.deactivateCalculateButton()

    def handlePhase(self, row, col):
        if col == 1 and row == 4:
            self.view.changedPhaseBox()

    def createThread(self):
        return thread_model.ThreadModel(self.alg)

    # constructs the inputs for the MaxEnt algorithms
    # then executes them (see maxent_model to see the order
    # of execution
    def handleMaxEntButton(self):
        if self.load.hasDataChanged():
            self.getWorkspaceNames()
            return
        # put this on its own thread so not to freeze Mantid
        self.thread = self.createThread()
        self.thread.threadWrapperSetUp(self.deactivate, self.handleFinished)

        # make some inputs
        inputs = {}
        inputs["Run"] = self.load.getRunName()
        maxentInputs = self.getMaxEntInput()

        if self.view.usePhases():
            phaseTable = {}
            if self.view.calcPhases():
                phaseTable["FirstGoodData"] = self.view.getFirstGoodData()
                phaseTable["LastGoodData"] = self.view.getLastGoodData()
                phaseTable["InputWorkspace"] = self.view.getInputWS()

                if "MuonAnalysisGrouped" not in phaseTable["InputWorkspace"]:
                    phaseTable["InputWorkspace"] = "MuonAnalysis"

                phaseTable["DetectorTable"] = "PhaseTable"
                phaseTable["DataFitted"] = "fits"

                inputs["phaseTable"] = phaseTable
            self.view.addPhaseTable(maxentInputs)

        inputs["maxent"] = maxentInputs
        self.thread.loadData(inputs)
        self.thread.start()

    # kills the thread at end of execution
    def handleFinished(self):
        self.activate()
        self.thread.threadWrapperTearDown(self.deactivate, self.handleFinished)
        self.thread.deleteLater()
        self.thread = None

    def getMaxEntInput(self):
        inputs = self.view.initMaxEntInput()
        if "MuonAnalysisGrouped" not in inputs["InputWorkspace"]:
            inputs["InputWorkspace"] = "MuonAnalysis"
        if self.view.outputPhases():
            self.view.addOutputPhases(inputs)

        if self.view.outputDeadTime():
            self.view.addOutputDeadTime(inputs)

        if self.view.outputPhaseEvo():
            self.view.addOutputPhaseEvo(inputs)

        if self.view.outputTime():
            self.view.addOutputTime(inputs)
        return inputs
