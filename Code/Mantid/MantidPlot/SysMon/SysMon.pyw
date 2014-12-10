#!/usr/bin/python
"""
SysMon.pyw
Initial application development 03Sep14 by S. Miller
The application utilizes the psutil and platform python modules to provide system information
for display via text fields, a table and Matplotlib plots.

The application utilizes a timer with user selectable timer intervals to update information 
provided to the application.
"""
import config
import sys

__version__="unknown"
try:
    from _version import __version__
except ImportError:
    #_version.py file not found - version not known in this case so use
    #the default previously given.
    pass

#parse args - doing it here as config structure needs to be filled prior to importing sysmon
if ['--nompl'] == [s for s in sys.argv if '--nompl' in s]:
    #case to not use matplotlib
    config.nompl=True
else:
    #case to use matplotlib (default case)
    config.nompl=False
    
if ['--help'] == [s for s in sys.argv if '--help' in s]:
    print "SysMon Help"
    print "--help    - this message"
    print "--nompl   - flag to disable using matplotlib, also disables History and Users tabs"
    print "--custom: - flag to use facility specific options"
    sys.exit()
    
if ['--custom'] == [s for s in sys.argv if '--custom' in s]:
    config.custom=True

from PyQt4 import QtGui

from ui_sysmonMainWindow import *
#from ui_sysmonTabs import *
from sysmon import *

class SysMonMainWindow(QtGui.QMainWindow):

    def __init__(self, parent=None):
        #setup main window
        QtGui.QMainWindow.__init__(self, parent)
        self.setWindowTitle("System Status")
        self.ui = Ui_MainWindow() #defined from ui_sysmon.py
        self.ui.setupUi(self)
        #setup tabs - class imported from sysmon.py
        self.sysmontabs=SysMon(self)
        self.setCentralWidget(self.sysmontabs)

        #setup menu bar actions
        self.connect(self.ui.actionExit, QtCore.SIGNAL('triggered()'), self.confirmExit) #define function to confirm and perform exit
        self.connect(self.ui.actionAbout, QtCore.SIGNAL('triggered()'), self.About)
        self.connect(self.ui.actionCheck_Matlab_Licenses, QtCore.SIGNAL('triggered()'), self.updateMatlab)

        #check if custom menu bar enabled via command line flag --custom and if not, remove it as it's built by default from Qt
        if not(config.custom):
            self.ui.menubar.removeAction(self.ui.menuCustom.menuAction())
        
        
    #define methods for menu bar options
    def confirmExit(self):
        reply = QtGui.QMessageBox.question(self, 'Message',
            "Are you sure to quit?", QtGui.QMessageBox.Yes | 
            QtGui.QMessageBox.No, QtGui.QMessageBox.No)

        if reply == QtGui.QMessageBox.Yes:
			#close application
            self.close()
        else:
			#do nothing and return
            pass   
            
    def About(self):
        dialog=QtGui.QMessageBox(self)
        dialog.setText("PyQt4 System Monitoring Application "+__version__)
        info='Application Info: \n\r * Changing the Update Rate Clears plots \n\r * It may take one full new update cycle for changes to take effect \n\r * Update rate shown in History plot xaxis label \n\r * Process tab CPU percentage can be greater than 100 when more than a single core is involved'
        dialog.setDetailedText(info) #give full info in detailed text
        dialog.exec_()
        
    def updateMatlab(self):
        #run license server command to extract license info
        info=commands.getstatusoutput(config.matlabchk)
        info=str(info[1]) #seem to need to make this a string for Linux to work properly
        #test if info string contains MATLAB info
        if info.find("MATLAB") < 0:
            #case where no license server found
            outstr="No Matlab License Server Found to Check"
        else:
            indx0=info.find("Users of MATLAB:")
            indx1=info.find("licenses in use")
            if indx0 > -1 and indx1 > -1:
                outstr=info[indx0:indx1+15+1]
            else:
                outstr="Unable to determine Matlab license information"
        dialog=QtGui.QMessageBox(self)
        #print "outstr: "+outstr
        dialog.setText(outstr)
        dialog.setDetailedText(info) #give full info in detailed text
        dialog.exec_()    
        
if __name__=="__main__":
    app = QtGui.QApplication(sys.argv)
    sysmon = SysMonMainWindow()
    sysmon.show()

    sys.exit(app.exec_())