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
import copy


class data:

    """
    Holds the data and relevant information for plotting
    """
    def __init__(self,name=None,x=[],y=[],E=[]):
        """
        Creates a data object.
        @param x :: the x data
        @param y :: the y data
        @param E :: the (y) errors
        """
        if name is not None:
            self.name = name
            self.x = copy.copy(x)
            self.y = copy.copy(y)
        else:
            self.name ='none'
            self.x = [0.0,0.0,0.0]
            self.y = [0.0,0.0,0.0]
        if(len(E)==0):
            self.E=np.zeros(len(self.x))
        else:
            self.E=copy.copy(E)
        self.showError=False
        self.markers="x"
        self.colour="k"
        self.linestyle='--'

    def order_data(self):
        """
        Ensures that the data is in assending order in x
        Prevents line plots looping back on them selves
        """

        xData=self.x
        yData=self.y
        eData=self.E

        for j in range(0,len(yData)):
            for k in range(0,len(xData)-1):
                if xData[k]>xData[k+1]:
                    xData[k+1],xData[k]=xData[k],xData[k+1]
                    yData[k+1],yData[k]=yData[k],yData[k+1]
                    eData[k+1],eData[k]=eData[k],eData[k+1]


class insert(data):
    """
    Minimal information for defining an insert
    """
    def __init__(self):
        self.logs= {'x':False,'y':False}
        #make this a list later
        self.data = []
        self.labels={'x':"",'y':'',"title":""}
        self.xrange={'start':0.0,'end':0.0}
        self.yrange={'start':0.0,'end':0.0}
        self.position={'left':0.0, 'bottom':0.0, 'width':0.25, 'height':0.25}

    def add_data(self,inputData):
        """
        Creates a horizontal bar plot
        @param safe :: if to ensure that y labels only appear once
        @param save:: the name of the file to save to
        the default is not to save
        """
        self.data.append(inputData)


class plot(data,insert):
    """
    Minimal information for a plot
    """
    def __init__(self):
        self.logs= {'x':False,'y':False}
        #make this a list later
        self.data = []
        self.labels={'x':"xlabel",'y':'ylabel',"title":"title"}
        self.xrange={'start':0.0,'end':0.0}
        self.yrange={'start':0.0,'end':0.0}
        self.legend="upper left"
        self.insert=None
        self.title_size=20

    def add_data(self,inputData):
        """
        Adds the data to the main plot
        @param inputData :: the data to add to the plot
        """
        self.data.append(inputData)

    def add_insert(self,inputInsert):
        """
        Adds an insert to the plot
        @param inputInsert :: the insert to add to the plot
        """
        self.insert=inputInsert

    def make_scatter_plot(self,save=""):
        """
        Creates a scatter plot
        @param save:: the name of the file to save to
        the default is not to save
        """
        plt.figure()
        plt.xlabel(self.labels["x"])
        plt.ylabel(self.labels["y"])
        plt.title(self.labels["title"],fontsize=self.title_size)
        for data in self.data:
            if len(data.x)==len(data.y):
                if(data.showError):
                        #plot with errors
                        plt.errorbar(data.x,data.y,yerr=data.E,label=data.name ,marker=data.markers,color=data.colour,
                                     linestyle=data.linestyle,markersize=8)
                else:
                        plt.plot(data.x,data.y,label=data.name,marker=data.markers,color=data.colour,
                                 linestyle=data.linestyle,markersize=8)
            else:
                print ("Data "+data.name+" contains data of unequal lengths ",len(data.x),len(data.y))
        plt.legend(loc=self.legend)
        if  self.xrange["start"]!=0.0 or self.xrange["end"]!=0.0:
            plt.xlim([self.xrange["start"],self.xrange["end"]])
        if  self.yrange["start"]!=0.0 or self.yrange["end"]!=0.0:
            plt.xlim([self.yrange["start"],self.yrange["end"]])
        if self.logs['x']:
            plt.xscale("log")
        if self.logs['y']:
            plt.yscale("log")
        plt.tight_layout()
        if save=="":
            plt.show()
        else:
            output_file = save.replace(",","")
            print ("saving to "+output_file.replace(" ","_"))
            plt.savefig(output_file.replace(" ","_"))

    # safe is used if the y values (strings) all have unique names
    def make_y_bar_plot(self,safe=True,save=""):
        """
        Creates a horizontal bar plot
        @param save:: the name of the file to save to
        the default is not to save
        """
        if self.insert is None:
            self.make_y_bar_plot_no_insert(safe,save)
        else:
            self.make_y_bar_plot_insert(safe,save)

    def make_y_labels_safe_part_1(self):
        """
        Ensures that if multiple data sets share some labels (y values)
        that the plot includes each one once and only once.
        """
        labels=[]
        for data in self.data:
            for y in data.y:
                if y not in labels:
                    labels.append(y)
        return labels

    def get_safe_data(self,labels,data):
        """
        Ensures that if multiple data sets share some labels (y values)
        that the plot is correct. If its not present a value of 0 is used
        @param labels :: the safe y labels
        @param data :: the data object to be made safe
        """
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
        """
        Creates a horizontal bar plot
        @param safe :: if to ensure that y labels only appear once
        @param save:: the name of the file to save to
        the default is not to save
        """
        fig,ax1=plt.subplots()
        ax1.set_xlabel(self.labels["x"])
        ax1.set_ylabel(self.labels["y"])
        ax1.set_title(self.labels["title"])
        if self.insert is not None:
            ax2=fig.add_axes([self.insert.position['left'],self.insert.position['bottom'],
                              self.insert.position['width'], self.insert.position['height']])
        # get labels
        if safe is True:
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
            if safe is True:
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
        plt.tight_layout()
        if save=="":
            plt.show()
        else:
            print ("saving to "+save.replace(" ","_"))
            plt.savefig(save.replace(" ","_"))

    def make_y_bar_plot_no_insert(self,safe,save):
        """
        Creates a horizontal bar plot
        @param safe :: if to ensure that y labels only appear once
        @param save:: the name of the file to save to
        the default is not to save
        """
        plt.figure()
        plt.xlabel(self.labels["x"])
        plt.ylabel(self.labels["y"])
        plt.title(self.labels["title"])
        # get labels
        if safe is True:
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
            if safe is True:
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
