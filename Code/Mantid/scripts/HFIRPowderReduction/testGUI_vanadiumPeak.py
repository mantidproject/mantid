###############################################################################
# Tester: Tab as vanadium peak strip
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
#
#
#
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

def qapp():
    if QtGui.QApplication.instance():
        _app = QtGui.QApplication.instance()
    else:
        _app = QtGui.QApplication(sys.argv)
    return _app

app = qapp()


import sys
osname = sys.platform
if osname.count('linux2') > 0:
    MOS = LINUX
    #sys.path.append("/home/wzz/Mantid/Code/debug/bin")
elif osname.count('darwin') > 0:
    MOS = OSX
    #sys.path.append("/Users/wzz/Mantid/Code/debug/bin")
else:
    raise NotImplementedError("OS %s is not supported." % (osname))

reducer = HfirPDReductionGUI.MainWindow() #the main ui class in this file is called MainWindow
if MOS == LINUX:
    reducer.ui.lineEdit_cache.setText('/home/wzz/Temp/')
elif MOS == OSX: 
    reducer.ui.lineEdit_cache.setText('/Users/wzz/Temp/')

# example: 'http://neutron.ornl.gov/user_data/hb2a/exp400/Datafiles/HB2A_exp0400_scan0001.dat'
print "Set to exp 457, scan 10"
reducer.ui.lineEdit_expNo.setText('379')
reducer.ui.lineEdit_scanNo.setText('10')
reducer.ui.lineEdit_wavelength.setText('2.41')

reducer.ui.lineEdit_min2Theta.setText('5.0')
reducer.ui.lineEdit_max2Theta.setText('150.0')
reducer.ui.lineEdit_binsize2Theta.setText('0.1')

# set to the right tab
reducer.ui.tabWidget.setCurrentIndex(4)

# load and reduce data 
reducer.doLoadData()

# rebin
reducer.ui.lineEdit_binsize2Theta.setText('0.1')
reducer.doRebin2Theta()
"""
reducer.doStripVandiumPeaks()
reducer.doSaveVanRun()
"""

# Skip if there is something wrong
reducer.show()
app.exec_()

