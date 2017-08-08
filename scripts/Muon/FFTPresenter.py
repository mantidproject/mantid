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
            self.view.table_click_signal.connect(self.Clicked) 
            self.view.button_signal.connect(self.handleButton)
            #layout
            #self.setLayout(self.view.grid)
     # only get ws that are groups or pairs
     # ignore raw 
     def getWorkspaceNames(self):
	    options=str(mantid.AnalysisDataService.getObjectNames()).replace(" ", "")
	    options=options[2:-2] 
            options=options.split("','")
            finalOptions=[]
            for pick in options:
		if ";" in pick and "Raw" not in pick: 
                    finalOptions.append(pick)
	    self.view.addItems(finalOptions)
 
     def Clicked(self,row,col):
         if row == self.view.getImBoxRow and col ==1:
		self.view.changedHideUnTick(self.view.ImBox,self.view.ImBoxRow+1)
         elif  row == self.view.getShiftBoxRow and col ==1:
		self.view.changed(self.view.shiftBox,self.view.shiftBoxRow+1)
     

     def handleButton(self):
        inputs = self.get_FFT_input()
   
        alg=mantid.AlgorithmManager.create("FFT")
        alg.initialize()
        #if doing multiple algs then will want child
        # will have to write the output to the ads manually
        alg.setChild(False)
        for name,value in iteritems(inputs):
               alg.setProperty(name,value)
               mantid.logger.warning(name+"  "+str(value))
        alg.execute() 
        #alg.getProperty("OutputWorkspace").value
       
     def get_FFT_input(self):
         inputs=self.view.init_FFT_input() 
         if self.view.isRaw():
             inputs["InputWorkspace"]+="_Raw"
         if  self.view.isAutoShift():
             inputs["AutoShift"]=True
         else:
             self.view.add_FFT_shift(inputs)
         if self.view.isComplex():
             self.view.add_FFT_complex(inputs)
             if self.view.isRaw():
                 inputs["InputImagWorkspace"]+="_Raw"
         return inputs 
