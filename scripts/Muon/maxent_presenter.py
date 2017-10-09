from __future__ import (absolute_import, division, print_function)

from Muon import thread_model


class MaxEntPresenter(object):

    def __init__(self,view,alg,load):
        self.view=view
        self.alg=alg
        self.load=load
        # set data
        self.getWorkspaceNames()
        #connect
        self.view.maxEntButtonSignal.connect(self.handleMaxEntButton)

    # only get ws that are groups or pairs
    # ignore raw
    # move the generating of the list to a helper?
    def getWorkspaceNames(self):
        # gets all WS in the ADS
        runName,options = self.load.getCurrentWS()
        final_options=[]
        # only keep the relevant WS (same run as Muon Analysis)
        for pick in options:
            if ";" in pick and "Raw" not in pick and runName in pick:
                final_options.append(pick)
        self.view.addItems(final_options)

    #functions
    def createThread(self):
        return thread_model.ThreadModel(self.alg)

    def handleMaxEntButton(self):
        self.thread=self.createThread()
        self.thread.started.connect(self.deactivate)
        self.thread.finished.connect(self.handleFinished)

        inputs = self.getMaxEntInput()
        runName=self.load.getRunName()
        self.thread.setInputs(inputs,runName)
        self.thread.start()

    def handleFinished(self):
        self.thread.deleteLater
        self.activate()

    def activate(self):
        self.view.activateButton()

    def deactivate(self):
        self.view.deactivateButton()

    def getMaxEntInput(self):
        inputs=self.view.initMaxEntInput()
        if self.view.isRaw():
            self.view.addRaw(inputs,"InputWorkspace")
            self.view.addRaw(inputs,"EvolChi")
            self.view.addRaw(inputs,"EvolAngle")
            self.view.addRaw(inputs,"ReconstructedImage")
            self.view.addRaw(inputs,"ReconstructedData")
        return inputs
