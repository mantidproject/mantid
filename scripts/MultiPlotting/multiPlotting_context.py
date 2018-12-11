# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import absolute_import, print_function
from MultiPlotting.subplot.subPlot_context import subPlotContext
import mantid.simpleapi as mantid
import numpy as np

dummy = "dummy"
subplots = "subplots"
xBounds = "xBounds"
yBounds = "yBounds"

def setUpSubplot():
    xData=np.linspace(start=0,stop=10,num=200)
    yData=np.sin(5.2*xData)
    eData = 0.1*np.sin(1.*xData)
    result = (1-yData )*3.1
    #ws= mantid.CreateWorkspace(DataX=xData, DataY=result,DataE=eData, OutputWorkspace="ws")
    ws=mantid.Load("MUSR00015089",OutputWorkspace="ws") 
    #ws = mtd['ws_1']
    return ws 

class PlottingContext(object):
    def __init__(self):
        self.context = {}
        self.context[dummy] = "plot test here init"
        self.ws=setUpSubplot()
        self.subplots = {}
        self.context[xBounds] =[0.,0.]
        self.context[yBounds] =[0.,0.]

    def addSubplot(self, name,subplot):
         self.subplots[name] = subPlotContext(name,subplot)

    def addLine(self,subplot, workspace,specNum):
	    try:
             if len(workspace) >1:
                  self.subplots[subplot].addLine(workspace.OutputWorkspace,specNum) 
             else:
                  self.subplots[subplot].addLine(workspace,specNum) 
	    except:
              pass # add error message


    def get(self, key):
        return self.context[key]

    def set(self,key,value):
       self.context[key] = value

    def get_xBounds(self):
        return self.context[xBounds]

    def get_yBounds(self):
        return self.context[yBounds]
