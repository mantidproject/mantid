from __future__ import (absolute_import, division, print_function)
from six import iteritems
import mantid.simpleapi as mantid

from PyQt4 import QtCore, QtGui
from Muon import FFT_view
#from Muon import MaxEnt_view
from Muon import transform_selection_view


class transformView(QtGui.QWidget):

    def __init__(self,parent=None):
       super(transformView,self).__init__(parent)
       self.FFTView =FFT_view.FFTView(self)
       self.selection=transform_selection_view.TransformSelectionView()
       self.Layout=QtGui.QGridLayout()
       self.Layout.addWidget(self.selection,1,0)
       self.Layout.addWidget(self.FFTView,2,0)
       self.setLayout(self.Layout)
        # set data
        
 
