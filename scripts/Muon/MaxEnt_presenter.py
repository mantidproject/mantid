from __future__ import (absolute_import, division, print_function)
from six import iteritems
import mantid.simpleapi as mantid

from Muon import MaxEnt_model

class MaxEntPresenter(object):

    def __init__(self,view,alg):
        self.view=view
        self.alg=alg
        # set data
        self.getWorkspaceNames()
        #connect
        #self.view.tableClickSignal.connect(self.tableClicked)
        self.view.maxEntButtonSignal.connect(self.handleMaxEntButton)

    # only get ws that are groups or pairs
    # ignore raw
    def getWorkspaceNames(self):
        options=mantid.AnalysisDataService.getObjectNames()
        options=[item.replace(" ","") for item in options]
        final_options=[]
        for pick in options:
            if ";" in pick and "Raw" not in pick:
                final_options.append(pick)
        self.view.addItems(final_options)

    #functions
    def handleMaxEntButton(self):
        inputs = self.get_MaxEnt_input()
        self.alg.setInputs(inputs)
        self.alg.execute()
        self.alg.output()

    def get_MaxEnt_input(self):
        inputs=self.view.initMaxEntInput()
        if self.view.isRaw():
            self.view.addRaw(inputs,"InputWorkspace")
            self.view.addRaw(inputs,"EvolChi")
            self.view.addRaw(inputs,"EvolAngle")
            self.view.addRaw(inputs,"ReconstructedImage")
            self.view.addRaw(inputs,"ReconstructedData")
        return inputs
