# Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD
# Oak Ridge National Laboratory & European Spallation Source
#
# This file is part of Mantid.
# Mantid is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# Mantid is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# File change history is stored at: <https://github.com/mantidproject/mantid>.
# Code Documentation is available at: <http://doxygen.mantidproject.org>
from __future__ import (absolute_import, division, print_function)
import matplotlib.pyplot as plt
import numpy as np


class data:
    def __init__(self,name=None,x=[],y=[],E=[]):
        if(name != None):
                self.name = name
                self.x = x
                self.y = y
        else:
                self.name ='none'
                self.x = [0.0,0.0,0.0]
                self.y = [0.0,0.0,0.0]
        if(len(E)==0):
                self.E=np.zeros(len(self.x))
        else:
                self.E=E
        self.showError=False
        self.markers="x"
        self.colour="k"
    def setShowError(self, state):
        self.showError=state
    def setColour(self,colour):
        self.colour = colour

class plot(data):

    def __init__(self):
        self.logs= {'x':True,'y':False}
        #make this a list later
        self.data = []
        self.labels={'x':"xlabel",'y':'ylabel',"title":"title"}
        self.xrange={'start':0.0,'end':10.0}
        self.yrange={'start':0.0,'end':10.0}
        self.legend="upper left"
    def addData(self,inputData):
        self.data.append(inputData)

    def makePlot(self):
        fig=plt.figure()
        plt.xlabel(self.labels["x"])
        plt.ylabel(self.labels["y"])
        plt.title(self.labels["title"])
        for data in self.data:
                if(data.showError):
                        #plot with errors
                        plt.errorbar(data.x,data.y,yerr=data.E,label=data.name ,marker=data.markers,color=data.colour,linestyle='--',markersize=8)
                else:
                        plt.plot(data.x,data.y,label=data.name,marker=data.markers,color=data.colour,linestyle='--',markersize=8)
        plt.legend(loc=self.legend)
        plt.show()
    def makeYBarPlot(self):
        fig=plt.figure()
        plt.xlabel(self.labels["x"])
        plt.ylabel(self.labels["y"])
        plt.title(self.labels["title"])
		# get labels
        labels=[]
        for data in self.data:
             for y in data.y:
                 if y not in labels:
                    labels.append(y)
        if len(self.data)>1:
             width=0.8/len(self.data)
        else:
             width=0.8
        yNum = np.linspace(0,len(labels)+len(self.data),len(labels))
        

        for k,data in enumerate(self.data,0):
		     #get data in same form
             xData =[]
             for label in labels:
                 if label in data.y:
                    for j,y in  enumerate(data.y,0):
                       if label ==y:
                          xData.append(data.x[j])
                 else:
                    xData.append(0.0)
			       
             print (len(xData),xData)
             print (k)		      
             plt.yticks(yNum+width*(len(self.data)-1.)/2.,labels)
             if data.showError:
                  plt.barh(yNum+k*width,xData,xerr=data.E,label=data.name,color=data.colour,align='center')
             else:

                  plt.barh(yNum+k*width,xData,width,label=data.name,color=data.colour,align='center')
        plt.legend(loc=self.legend)
        plt.show()
