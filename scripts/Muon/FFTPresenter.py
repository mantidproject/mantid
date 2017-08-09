from __future__ import (absolute_import, division, print_function)
from six import iteritems
from PyQt4 import QtCore, QtGui
from PyQt4.QtCore import *
from PyQt4.QtGui import *
import sys

import mantid.simpleapi as mantid
from Muon import FFTView
from mantid.api import Algorithm

class FFTPresenter(object):


    def __init__(self,view):
        self.view=view
        # set data
        self.getWorkspaceNames()
        #connect
        self.view.tableClickSignal.connect(self.tableClicked) 
        self.view.buttonSignal.connect(self.handleButton)
    
    # only get ws that are groups or pairs
    # ignore raw 
    def getWorkspaceNames(self):
        options=str(mantid.AnalysisDataService.getObjectNames()).replace(" ", "")
        options=options[2:-2] 
        options=options.split("','")
        final_options=[]
        for pick in options:
       	    if ";" in pick and "Raw" not in pick: 
                final_options.append(pick)
        self.view.addItems(final_options)
    
    #functions  
    def tableClicked(self,row,col):
         if row == self.view.getImBoxRow() and col ==1:
        	    self.view.changedHideUnTick(self.view.getImBox(),self.view.getImBoxRow()+1)
         elif  row == self.view.getShiftBoxRow() and col ==1:
        	    self.view.changed(self.view.getShiftBox(),self.view.getShiftBoxRow()+1)
    
    def handleButton(self):
        inputs = self.get_FFT_input()
        alg=mantid.AlgorithmManager.create("FFT")
        alg.initialize()
        alg.setChild(False)
        for name,value in iteritems(inputs):
             alg.setProperty(name,value)
             mantid.logger.warning(name+"  "+str(value))
        alg.execute() 
      
    def get_FFT_input(self):
        inputs=self.view.initFFTInput() 
        if self.view.isRaw():
            self.view.addRaw(inputs,"InputWorkspace")
    
        if  self.view.isAutoShift():
            inputs["AutoShift"]=True
        else:
            self.view.addFFTShift(inputs)
        if self.view.isComplex():
            self.view.addFFTComplex(inputs)
            if self.view.isRaw():
                self.view.addRaw(inputs,"InputImagWorkspace")
        return inputs 
