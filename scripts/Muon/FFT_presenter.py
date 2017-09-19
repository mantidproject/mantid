from __future__ import (absolute_import, division, print_function)

from six import iteritems


import mantid.simpleapi as mantid


class FFTPresenter(object):

  
    def __init__(self,view,alg,load):
        self.view=view
        self.alg=alg
        self.load=load

        # set data
        self.getWorkspaceNames()
        #connect
        self.view.tableClickSignal.connect(self.tableClicked)
        self.view.buttonSignal.connect(self.handleButton)
        self.view.phaseCheckSignal.connect(self.phaseCheck)

    # only get ws that are groups or pairs
    # ignore raw
    def getWorkspaceNames(self):
        runName,options = self.load.getCurrentWS()
        final_options = []
        for pick in options:
            if ";" in pick and "Raw" not in pick and runName in pick:
                final_options.append(pick)
        self.view.addItems(final_options)

    #functions
    def phaseCheck(self):
        self.view.phaseQuadChanged()
        #check if a phase table exists
        if mantid.AnalysisDataService.doesExist("PhaseTable"):
           self.view.setPhaseBox() 

    def tableClicked(self,row,col):
        if row == self.view.getImBoxRow() and col == 1:
            self.view.changedHideUnTick(self.view.getImBox(),self.view.getImBoxRow()+1)
        elif  row == self.view.getShiftBoxRow() and col == 1:
            self.view.changed(self.view.getShiftBox(),self.view.getShiftBoxRow()+1)

    def handleButton(self):
        self.alg.getAlg.setRun(self.load.getRunName())

        #do apodization and padding to real data

        preInputs=self.view.initAdvanced()

        if self.view.getWS() == "PhaseQuad":
            if self.view.isNewPhaseTable()==True:
                axis=self.view.getAxis()
                self.alg.getAlg.makePhaseQuadTable(axis,self.load.getInstrument())
            self.alg.getAlg.PhaseQuad()
            self.view.RePhaseAdvanced(preInputs)
        else:
            self.view.ReAdvanced(preInputs)
            if self.view.isRaw():
                self.view.addRaw(preInputs,"InputWorkspace")
            tmp=1
        self.alg.getAlg.preAlg(preInputs)

        #do apodization and padding to complex data
        if self.view.isComplex() and self.view.getWS()!="PhaseQuad":
            self.view.ImAdvanced(preInputs)
            if self.view.isRaw():
                self.view.addRaw(preInputs,"InputWorkspace")
            self.alg.getAlg.preAlg(preInputs)

        #do FFT to transformed data
        FFTInputs = self.get_FFT_input()
        if self.view.getWS()=="PhaseQuad":
            self.view.getFFTRePhase(FFTInputs)
            if self.view.isComplex():
                self.view.getFFTImPhase(FFTInputs)
        else:
            if self.view.isRaw():
                self.view.addRaw(FFTInputs,"OutputWorkspace")
        self.alg.getAlg.FFTAlg(FFTInputs)

        self.view.setPhaseBox()

    def get_FFT_input(self):
        FFTInputs=self.view.initFFTInput()
        if  self.view.isAutoShift():
            FFTInputs["AutoShift"]=True
        else:
            self.view.addFFTShift(FFTInputs)
        if self.view.isComplex():
            self.view.addFFTComplex(FFTInputs)
        return FFTInputs

