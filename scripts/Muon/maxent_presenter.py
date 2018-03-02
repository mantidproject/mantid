from __future__ import (absolute_import, division, print_function)

from Muon import thread_model
from Muon import maxent_model

import mantid.simpleapi as mantid
import math


class MaxEntPresenter(object):
    """
    This is the presenter for the maximum entropy widget.
    It connects the view and model together and deals with
    logic.
    """
    def __init__(self,view,alg,load):
        self.view=view
        self.alg=alg
        self.calcAlg = maxent_model.PhaseModel()
        self.load=load
        # set data
        self.getWorkspaceNames()
        #connect
        self.view.maxEntButtonSignal.connect(self.handleMaxEntButton)
        self.view.cancelSignal.connect(self.cancel)
        self.view.phaseSignal.connect(self.handlePhase)

    #functions
    def getWorkspaceNames(self):
        final_options=self.load.getGroupedWorkspaceNames()
        run = self.load.getRunName()
        self.view.setRun(run)
        final_options.append(run)
        self.view.addItems(final_options)
        start = int(math.ceil(math.log(self.load.getNPoints())/math.log(2.0)))
        values = [str(2**k) for k in range(start,21)]
        self.view.addNPoints(values)

    def createThread(self):
        return thread_model.ThreadModel(self.alg)

    def createPhaseThread(self):
        return thread_model.ThreadModel(self.calcAlg)

    def handleMaxEntButton(self):
        if  self.view.calcPhases() and self.view.usePhases():
            self.DoPhase()
        else:
            self.DoMaxEnt()

    def DoMaxEnt(self):
        self.thread=self.createThread()
        self.thread.started.connect(self.deactivate)
        self.thread.finished.connect(self.handleFinished)

        inputs = self.getMaxEntInput()
        if self.view.usePhases():
            self.view.addPhaseTable(inputs)
        runName=self.load.getRunName()

        self.thread.setInputs(inputs,runName)
        self.thread.start()

    def DoPhase(self):

        if self.view.usePhases() and self.view.calcPhases():
            inputs_phase = self.getCalPhasesInput()
            if inputs_phase["InputWorkspace"] != "MuonAnalysis":
                mantid.logger.error("Cannot currently generate phase table from this data using CalMuonDetecotrPhases")
                return
            self.calcThread=self.createPhaseThread()
            self.calcThread.started.connect(self.deactivate)
            self.calcThread.finished.connect(self.handleFinishedCalc)
            self.calcThread.loadData(inputs_phase)
            self.calcThread.start()

    def handlePhase(self,row,col):
        if col==1 and row == 4:
            self.view.changedPhaseBox()

    def cancel(self):
        if self.thread is not None:
            self.thread.cancel()

    def handleFinished(self):
        self.activate()
        self.thread.deleteLater()
        self.thread=None

    def deactivate(self):
        self.view.deactivateCalculateButton()

    def handleFinishedCalc(self):
        self.activate()
        self.calcThread.deleteLater()
        self.calcThread=None
        # ensures maxent is done after calcMuonDetectorPhases
        self.DoMaxEnt()

    def activate(self):
        self.view.activateCalculateButton()

    def getMaxEntInput(self):
        inputs=self.view.initMaxEntInput()
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

    def getCalPhasesInput(self):
        inputs_phase = None
        if self.view.calcPhases():
            inputs_phase =self.view.calcPhasesInit()
            if "MuonAnalysisGrouped" not in inputs_phase["InputWorkspace"]:
                inputs_phase["InputWorkspace"] = "MuonAnalysis"

        return inputs_phase
