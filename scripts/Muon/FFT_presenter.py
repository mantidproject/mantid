from __future__ import (absolute_import, division, print_function)
from six import iteritems

import mantid.simpleapi as mantid


class FFTPresenter(object):

    def __init__(self,view,alg):
        self.view=view
        self.alg=alg
        # set data
        self.getWorkspaceNames()
        #connect
        self.view.tableClickSignal.connect(self.tableClicked)
        self.view.buttonSignal.connect(self.handleButton)

    # only get ws that are groups or pairs
    # ignore raw
    def getWorkspaceNames(self):
        options = mantid.AnalysisDataService.getObjectNames()
        options = [item.replace(" ","") for item in options]
        final_options = []
        for pick in options:
            if ";" in pick and "Raw" not in pick:
                final_options.append(pick)
        self.view.addItems(final_options)

    #functions
    def tableClicked(self,row,col):
        if row == self.view.getImBoxRow() and col == 1:
            self.view.changedHideUnTick(self.view.getImBox(),self.view.getImBoxRow()+1)
        elif  row == self.view.getShiftBoxRow() and col == 1:
            self.view.changed(self.view.getShiftBox(),self.view.getShiftBoxRow()+1)

    def handleButton(self):
        #do apodization and padding to real data
        preInputs=self.view.initAdvanced()
        self.view.ReAdvanced(preInputs)
        self.alg.preAlg(preInputs)
        if self.view.isRaw():
            self.view.addRaw(preInputs,"InputWorkspace")
        #do apodization and padding to complex data
        if self.view.isComplex():
            self.view.ImAdvanced(preInputs)
            self.alg.preAlg(preInputs)
            if self.view.isRaw():
                self.view.addRaw(preInputs,"InputWorkspace")
        #do FFT to transformed data
        FFTInputs = self.get_FFT_input()
        if self.view.isRaw():
            self.view.addRaw(FFTInputs,"OutputWorkspace")
        self.alg.FFTAlg(FFTInputs)
 
    def get_FFT_input(self):
        FFTInputs=self.view.initFFTInput()
        if  self.view.isAutoShift():
            FFTInputs["AutoShift"]=True
        else:
            self.view.addFFTShift(FFTInputs)
        if self.view.isComplex():
            self.view.addFFTComplex(FFTInputs)
        return FFTInputs
