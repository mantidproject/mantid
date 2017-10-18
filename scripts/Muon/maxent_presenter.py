from __future__ import (absolute_import, division, print_function)

from Muon import thread_model


class MaxEntPresenter(object):
    """
    This is the presenter for the maximum entropy widget.
    It connects the view and model together and deals with
    logic.
    """
    def __init__(self,view,alg,load):
        self.view=view
        self.alg=alg
        self.load=load
        # set data
        self.getWorkspaceNames()
        #connect
        self.view.maxEntButtonSignal.connect(self.handleMaxEntButton)
        self.view.cancelSignal.connect(self.cancel)

    #functions
    def getWorkspaceNames(self):
        final_options=self.load.getWorkspaceNames()
        self.view.addItems(final_options)

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

    def cancel(self):
        import time
        if self.thread is not None:
            self.thread.cancel()
 
    def handleFinished(self):
        self.activate()
        self.thread.deleteLater()
        self.thread=None

    def activate(self):
        self.view.activateCalculateButton()

    def deactivate(self):
        self.view.deactivateCalculateButton()

    def getMaxEntInput(self):
        inputs=self.view.initMaxEntInput()
        if self.view.isRaw():
            self.view.addRaw(inputs,"InputWorkspace")
            self.view.addRaw(inputs,"EvolChi")
            self.view.addRaw(inputs,"EvolAngle")
            self.view.addRaw(inputs,"ReconstructedImage")
            self.view.addRaw(inputs,"ReconstructedData")
        return inputs
