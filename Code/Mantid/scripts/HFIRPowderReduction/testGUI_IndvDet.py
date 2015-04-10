###############################################################################
# Tester 
#
# ( 3) A dictionary should be used to manage the history data
# ( 8) Merge runs
# ( 9) Proper quit
# (10) Need a configuration file to load
# (11) Auto reset the x-y-limit of the plot
# (12) Design for vanadium peak strip
# (13) Implementation for vanadium peak strip
# (14) doPlotUnitDspace: auto fill minD, maxD, binsizeD
# (15) During loading, ui.label_ptNo and ui.label_detNo should give out the \
#      range of pt. and det number
#
###############################################################################

""" Test main """
import sys

import HfirPDReductionGUI
from PyQt4 import QtGui


# Globals
LINUX = 1
OSX   = 2
##########

osname = sys.platform
if osname.count('linux2') > 0:
    MOS = LINUX
elif osname.count('darwin') > 0:
    MOS = OSX
else:
    raise NotImplementedError("OS %s is not supported." % (osname))

def qapp():
    if QtGui.QApplication.instance():
        _app = QtGui.QApplication.instance()
    else:
        _app = QtGui.QApplication(sys.argv)
    return _app

app = qapp()
reducer = HfirPDReductionGUI.MainWindow() #the main ui class in this file is called MainWindow
reducer.show()

# example: 'http://neutron.ornl.gov/user_data/hb2a/exp400/Datafiles/HB2A_exp0400_scan0001.dat'
print "Set to exp 231, scan 1"
reducer.ui.lineEdit_expNo.setText('231')
reducer.ui.lineEdit_scanNo.setText('1')
reducer.ui.lineEdit_detID.setText('21')
reducer.ui.tabWidget.setCurrentIndex(1)

if MOS == LINUX:
    reducer.ui.lineEdit_cache.setText('/home/wzz/Temp/')
elif MOS == OSX: 
    reducer.ui.lineEdit_cache.setText('/Users/wzz/Temp/')

# plot raw
reducer.doLoadData()
reducer.doPlotIndvDet()

app.exec_()

