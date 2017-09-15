from __future__ import (absolute_import, division, print_function)


class MaxEntPresenter(object):

    def __init__(self,view,alg,load):
        self.view=view
        self.alg=alg
        self.load=load
        # set data
        self.getWorkspaceNames()
        #connect
        self.alg.started.connect(self.view.deactivateButton)
        self.alg.finished.connect(self.view.activateButton)
        self.view.maxEntButtonSignal.connect(self.handleMaxEntButton)

    # only get ws that are groups or pairs
    # ignore raw
    # move the generating of the list to a helper?
    def getWorkspaceNames(self):
        runName,options = self.load.getCurrentWS()
        final_options=[]
        for pick in options:
            if ";" in pick and "Raw" not in pick and runName in pick:
                final_options.append(pick)
        self.view.addItems(final_options)

    #functions
    def handleMaxEntButton(self):
        inputs = self.get_MaxEnt_input()
        runName=self.load.getRunName()
        self.alg.setInputs(inputs,runName)
        self.alg.start()

    def get_MaxEnt_input(self):
        inputs=self.view.initMaxEntInput()
        if self.view.isRaw():
            self.view.addRaw(inputs,"InputWorkspace")
            self.view.addRaw(inputs,"EvolChi")
            self.view.addRaw(inputs,"EvolAngle")
            self.view.addRaw(inputs,"ReconstructedImage")
            self.view.addRaw(inputs,"ReconstructedData")
        return inputs
