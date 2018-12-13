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

xBounds = "xBounds"
yBounds = "yBounds"

class PlottingContext(object):
    def __init__(self):
        self.context = {}
        self.subplots = {}
        self.context[xBounds] =[0.,0.]
        self.context[yBounds] =[0.,0.]

    def addSubplot(self, name,subplot):
         self.subplots[name] = subPlotContext(name,subplot)

    def addLine(self,subplotName, workspace,specNum):
	    try:
             if len(workspace) >1:
                  self.subplots[subplotName].addLine(workspace.OutputWorkspace,specNum) 
             else:
                  self.subplots[subplotName].addLine(workspace,specNum) 
	    except:
              print("cannot plot workspace")


    def get(self, key):
        return self.context[key]

    def set(self,key,value):
       self.context[key] = value

    def get_xBounds(self):
        return self.context[xBounds]

    def get_yBounds(self):
        return self.context[yBounds]
