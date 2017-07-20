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

import numpy as np
import matplotlib.pyplot as plt

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


class insert(data):

    def __init__(self):
        self.logs= {'x':False,'y':False}
        #make this a list later
        self.data = []
        self.labels={'x':"",'y':'',"title":""}
        self.xrange={'start':0.0,'end':0.0}
        self.yrange={'start':0.0,'end':0.0}
        self.position={'left':0.0, 'bottom':0.0, 'width':0.25, 'height':0.25}
    def add_data(self,inputData):
        self.data.append(inputData)


class plot(data,insert):

    def __init__(self):
        self.logs= {'x':False,'y':False}
        #make this a list later
        self.data = []
        self.labels={'x':"xlabel",'y':'ylabel',"title":"title"}
        self.xrange={'start':0.0,'end':0.0}
        self.yrange={'start':0.0,'end':0.0}
        self.legend="upper left"
        self.insert=None
    def add_data(self,inputData):
        self.data.append(inputData)
    def add_insert(self,inputInsert):
        self.insert=inputInsert	  
    def make_scatter_plot(self):
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
    # safe is used if the y values (strings) all have unique names
    def make_y_bar_plot(self,safe=True,save=""):
        if self.insert==None:
            self.make_y_bar_plot_no_insert(safe,save)
        else:
            self.make_y_bar_plot_insert(safe,save)

    def make_y_labels_safe_part_1(self):
        labels=[]
        for data in self.data:
             for y in data.y:
                 if y not in labels:
                    labels.append(y)
        return labels	
    def get_safe_data(self,labels,data):
		 #get data in same form
         xData =[]
         eData=[]
         for label in labels:
                 if label in data.y:
                    for j,y in  enumerate(data.y,0):
                       if label ==y:
                          xData.append(data.x[j])
                          eData.append(data.E[j])
                 else:
                    xData.append(0.0)
                    eData.append(0.0)
         return xData,eData						 
    def make_y_bar_plot_insert(self,safe,save):
        fig,ax1=plt.subplots()
        ax1.set_xlabel(self.labels["x"])
        ax1.set_ylabel(self.labels["y"])
        ax1.set_title(self.labels["title"])
        if self.insert!=None:
           ax2=fig.add_axes([self.insert.position['left'],self.insert.position['bottom'],self.insert.position['width'], self.insert.position['height']])
		# get labels
        if safe==True:
            labels=self.make_y_labels_safe_part_1()
        else: 
            labels=self.data[0].y

        if len(self.data)>1:
             width=0.8/len(self.data)
        else:
             width=0.8
        yNum = np.linspace(0,len(labels)+len(self.data),len(labels))
        

        for k,data in enumerate(self.data,0):
		#     #get data in same form
             if safe==True:
				xData,eData=self.get_safe_data(labels,data)
             else: 
                xData=data.x
                eData=data.E 				  
             ax1.set_yticks(yNum+width*(len(self.data)-1.)/2.)
             ax1.set_yticklabels(labels)
             ax2.set_yticks(yNum+width*(len(self.data)-1.)/2.)
             ax2.set_yticklabels(labels)
             if data.showError:
                  ax1.barh(yNum+k*width,xData,width,log= self.logs['x'],xerr=eData,label=data.name,color=data.colour,align='center')
             else:

                  ax1.barh(yNum+k*width,xData,width,log= self.logs['x'],label=data.name,color=data.colour,align='center')
             
             ax2.barh(yNum+k*width,xData,width,log= self.insert.logs['x'],color=data.colour,align='center')

        ax1.legend(loc=self.legend)
        if  self.xrange["start"]!=0.0 or self.xrange["end"]!=0.0:
             ax1.set_xlim([self.xrange["start"],self.xrange["end"]])
        print(self.logs['x'],self.insert.logs['x'])
        plt.tight_layout()
        if save=="":
           plt.show()
        else:
           print ("saving to "+save.replace(" ","_"))
           plt.savefig(save.replace(" ","_"))

    def make_y_bar_plot_no_insert(self,safe,save):
        fig=plt.figure()
        plt.xlabel(self.labels["x"])
        plt.ylabel(self.labels["y"])
        plt.title(self.labels["title"])
		# get labels
        if safe==True:
            labels=self.make_y_labels_safe_part_1()
        else: 
            labels=self.data[0].y

        if len(self.data)>1:
             width=0.8/len(self.data)
        else:
             width=0.8
        yNum = np.linspace(0,len(labels)+len(self.data),len(labels))
        

        for k,data in enumerate(self.data,0):
             #get data in same form
             if safe==True:
				xData,eData=self.get_safe_data(labels,data)
             else: 
                xData=data.x
                eData=data.E 				  
             plt.yticks(yNum+width*(len(self.data)-1.)/2.,labels)
             if data.showError:
                  plt.barh(yNum+k*width,xData,width,log= self.logs['x'],xerr=eData,label=data.name,color=data.colour,align='center')
             else:

                  plt.barh(yNum+k*width,xData,width,log= self.logs['x'],label=data.name,color=data.colour,align='center')           						 				   
        plt.legend(loc=self.legend)
        plt.tight_layout()
        if  self.xrange["start"]!=0.0 or self.xrange["end"]!=0.0:
             plt.xlim([self.xrange["start"],self.xrange["end"]])
        if save=="":
           plt.show()
        else:
           print ("saving to "+save.replace(" ","_"))
           plt.savefig(save.replace(" ","_"))