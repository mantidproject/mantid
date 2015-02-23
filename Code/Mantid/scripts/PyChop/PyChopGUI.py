#pylint: disable=invalid-name

from PyChopUI import Ui_MainWindow
from PyQt4 import QtCore, QtGui
from mantidplot import *
from mantid import *
from mantid.simpleapi import *
import PyChop

class MainWindow(QtGui.QMainWindow):

    def __init__(self, parent=None):
        QtGui.QMainWindow.__init__(self,parent)
        self.ui = Ui_MainWindow()
        self.ui.setupUi(self)
        QtCore.QObject.connect(self.ui.calc, QtCore.SIGNAL("clicked()"), self.calc )
        QtCore.QObject.connect(self.ui.ei, QtCore.SIGNAL("returnPressed()"), self.ei)
        QtCore.QObject.connect(self.ui.actionMari, QtCore.SIGNAL("triggered()"), lambda : self.set_inst('MAR'))
        QtCore.QObject.connect(self.ui.actionMaps, QtCore.SIGNAL("triggered()"), lambda : self.set_inst('MAP'))
        QtCore.QObject.connect(self.ui.actionMerlin, QtCore.SIGNAL("triggered()"), lambda : self.set_inst('MER') )

        QtCore.QObject.connect(self.ui.actionGadolinium, QtCore.SIGNAL("triggered()"),lambda : self.set_chop('g') )
        QtCore.QObject.connect(self.ui.actionSloppy, QtCore.SIGNAL("triggered()"), lambda : self.set_chop('s') )
        QtCore.QObject.connect(self.ui.actionA, QtCore.SIGNAL("triggered()"),lambda : self.set_chop('a') )
        QtCore.QObject.connect(self.ui.actionRType, QtCore.SIGNAL("triggered()"), lambda : self.set_chop('r') )



        self.inst=config['default.instrument']
        self.chop=''
        self.ei=0.0

    def set_inst(self,name):
        self.inst=name
        config['default.instrument']=name
        if len(self.chop)>0:
            message,err = PyChop.setchoptype(self.inst,self.chop)
            if err>0:
                self.chop=''

    def set_chop(self,chop_type):
        message,err = PyChop.setchoptype(self.inst,chop_type)
        if err > 0:
            self.chop = ''
            QtGui.QMessageBox.warning(self, "PyChop",message)
            return
        else:
            self.chop=chop_type


    def ei(self):
        self.ei=float(self.ui.ei.text())

    def calc(self):
        self.ei=float(self.ui.ei.text())
        if len(self.chop) == 0:
            QtGui.QMessageBox.warning(self, "PyChop","Choper has not been selected. Select the chopper first.")
            return

        message,err = PyChop.setchoptype(self.inst,self.chop)
        if err > 0:
            QtGui.QMessageBox.warning(self, "PyChop",message)
            return


        string='Calculation for '+self.inst+' '+self.chop+' chopper at '+self.ui.ei.text()+'meV'
        self.ui.disp.addItem(string)
        string_title = 'Frequency(Hz)   Flux(n/s/cm^2)     Resolution[mEv]'
        self.ui.disp.addItem(string_title)
        for freq in range(50,650,50):
            van_el,van,flux=PyChop.calculate(self.ei,freq,all=True)

            #string1= 'Flux at sample position at   '+str(freq)+ ' Hz = '+" %.2f" % flux+'n/s/cm^2'
            # calc resolution as a function of energy trans
            #string2= 'Resolution of elastic line at '+str(freq)+ 'Hz = '+"%.2f" %van_el+ 'meV = '+"%.2f" % ((van_el/self.ei)*100)+ '%'
            #string1 = '\t {0:.2f} \t {1:.2f} \t {2:.2f}'.format(freq,flux,(van_el/self.ei)*100)
            #string1 = '\t {0:.2f} \t {1:.2f} \t {2:.2f}'.format(freq,flux,(van_el/self.ei)*100)
            string1 = ' {0}    \t {1:.2f}  \t  {2:.2f}'.format(freq,float(flux),((van_el/self.ei)*100))
            self.ui.disp.addItem(string1)
            #self.ui.disp.addItem(string2)

