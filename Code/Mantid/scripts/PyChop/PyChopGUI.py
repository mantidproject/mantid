
import sys
from PyChopUI import Ui_MainWindow
from PyQt4 import QtCore, uic,QtGui
import MantidFramework 
MantidFramework.mtd.initialise()
#from DirectEnergyConversion import *
import time as time
from mantidplotpy import *
import dgreduce
import inspect
import numpy
from mantidplot import *
from mantid import *
from mantid.simpleapi import *
import math
from PyChop import *
#class MainWindow(QtGui.QMainWindow, Ui_MainWindow):

class MainWindow(QtGui.QMainWindow):

	def __init__(self, parent=None):
		QtGui.QMainWindow.__init__(self,parent)
		self.ui = Ui_MainWindow()
		self.ui.setupUi(self)
		QtCore.QObject.connect(self.ui.calc, QtCore.SIGNAL("clicked()"), self.calc )
		QtCore.QObject.connect(self.ui.ei, QtCore.SIGNAL("returnPressed()"), self.ei)
		QtCore.QObject.connect(self.ui.actionMari, QtCore.SIGNAL("triggered()"), self.mari )
		QtCore.QObject.connect(self.ui.actionMaps, QtCore.SIGNAL("triggered()"), self.maps )
		QtCore.QObject.connect(self.ui.actionMerlin, QtCore.SIGNAL("triggered()"), self.merlin )
		QtCore.QObject.connect(self.ui.actionGadolinium, QtCore.SIGNAL("triggered()"), self.gd )
		QtCore.QObject.connect(self.ui.actionSloppy, QtCore.SIGNAL("triggered()"), self.sloppy )
		QtCore.QObject.connect(self.ui.actionA, QtCore.SIGNAL("triggered()"), self.a )
		QtCore.QObject.connect(self.ui.actionRType, QtCore.SIGNAL("triggered()"), self.r )
		
		
		
		self.inst=''
		self.chop=''
		self.ei=0.0
		
	def maps(self):
		self.inst='maps'
	def merlin(self):
		self.inst='merlin'
	def mari(self):
		self.inst='mari'
	
	def gd(self):
		self.chop='g'
	def sloppy(self):
		self.chop='s'
	def a(self):
		self.chop='a'
	def r(self):
		self.chop='r'
		
	def ei(self):
		self.ei=float(self.ui.ei.text())
		
	def calc(self):
		self.ei=float(self.ui.ei.text())
		setchoptype(self.inst,self.chop)
		string='Calculation for '+self.inst+' '+self.chop+' chopper at '+self.ui.ei.text()+'meV'
		self.ui.disp.addItem(string)
		for freq in range(50,650,50):
			van_el,van,flux=calculate(self.ei,freq,all=True)
			
			string1= 'Flux at sample position at   '+str(freq)+ ' Hz = '+" %.2f" % flux+'n/s/cm^2'
			# calc resolution as a function of energy trans
			string2= 'Resolution of elastic line at '+str(freq)+ 'Hz = '+"%.2f" %van_el+ 'meV = '+"%.2f" % ((van_el/self.ei)*100)+ '%'
			self.ui.disp.addItem(string1)
			self.ui.disp.addItem(string2)

def qapp():
	if QtGui.QApplication.instance():
		app = QtGui.QApplication.instance()
	else:
		app = QtGui.QApplication(sys.argv)
	return app
 
app = qapp()
reducer = MainWindow()
reducer.show()
app.exec_()