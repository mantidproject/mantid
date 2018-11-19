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

def setUpSubplot():
    xData=np.linspace(start=0,stop=10,num=200)
    yData=np.sin(5.2*xData)
    result = (1-yData )*3.1
    ws= mantid.CreateWorkspace(DataX=xData, DataY=result,OutputWorkspace="ws")
    return ws   

class PlottingContext(object):
    def __init__(self):
        self.context = {}
        self.context[dummy] = "plot test here init"
        self.ws=setUpSubplot()
        self.subplots = {}

    def addSubplot(self, name):
         self.subplots[name] = subPlotContext(name)

    def addLine(self,subplot, label, lines,workspace):
        if subplot not in self.subplots.keys():
           self.addSubplot(subplot)
        self.subplots[subplot].addLine(label, lines, workspace) 

    def get(self, key):
        return self.context[key]

    def set(self,key,value):
       self.context[key] = value
