from __future__ import (absolute_import, division, print_function)
from six import iteritems
import mantid.simpleapi as mantid

from PyQt4 import QtCore, QtGui
from Muon import FFT_view
from Muon import MaxEnt_view
from Muon import transform_selection_view


class transformView(QtGui.QWidget):
    #methodSignal = QtCore.pyqtSignal()
    def __init__(self,parent=None):
       super(transformView,self).__init__(parent)
       self.methods={}
       self.methods["FFT"]=FFT_view.FFTView(self)
       self.selection=transform_selection_view.TransformSelectionView()
       self.methods["MaxEnt"]=MaxEntView=MaxEnt_view.MaxEntView()
       self.Layout=QtGui.QGridLayout()
       self.Layout.addWidget(self.selection,1,0)
       self.Layout.addWidget(self.methods["FFT"],2,0)
       self.Layout.addWidget(self.methods["MaxEnt"],3,0)
       self.setLayout(self.Layout)
       self.methods["MaxEnt"].hide()
    
    def getMethods(self):
       return [key for key in self.methods]
   
    def hideAll(self):
        for key in self.methods:
            self.methods[key].hide()

    def show(self,name):
        self.methods[name].show()
     #  connect
        # set data
        
 
