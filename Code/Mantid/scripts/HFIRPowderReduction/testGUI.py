###############################################################################
# Tester 
#
# Next:
# (-) Add an option/option groups such that the new reduced data can be plot 
#     on a clean canvas or over plot on the original one;
# (-) An inner sequence for line-color-marker-style of the plot should be made
# (3) A dictionary should be used to manage the history data
# (4) Shall add button to load 'next' and 'previous' 
# (5) Make Ge 113 In Config and etc a comboBox for wavelength
# (6) Add tool bar to plot for save/zoom in and out and etc.
# (7) Label of the plots
# (8) Merge runs
# (9) Proper quit
# (10) Need a configuration file to load
# (11) Auto reset the x-y-limit of the plot
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
    sys.path.append("/home/wzz/Mantid/Code/debug/bin")
elif osname.count('darwin') > 0:
    MOS = OSX
    sys.path.append("/Users/wzz/Mantid/Code/debug/bin")
else:
    raise NotImplementedError("OS %s is not supported." % (osname))


reducer = HfirPDReductionGUI.MainWindow() #the main ui class in this file is called MainWindow
reducer.show()

# example: 'http://neutron.ornl.gov/user_data/hb2a/exp400/Datafiles/HB2A_exp0400_scan0001.dat'
print "Set to exp 231, scan 1"
reducer.ui.lineEdit_expNo.setText('231')
reducer.ui.lineEdit_scanNo.setText('1')
reducer.ui.lineEdit_wavelength.setText('2.41')

if MOS == LINUX:
    reducer.ui.lineEdit_cache.setText('/home/wzz/Temp/')
elif MOS == OSX: 
    reducer.ui.lineEdit_cache.setText('/Users/wzz/Temp/')

reducer.ui.lineEdit_xmin.setText('5.0')
reducer.ui.lineEdit_xmax.setText('150.0')
reducer.ui.lineEdit_binsize.setText('0.1')


# load and reduce data 
reducer.doLoadData()

# try:
#     reducer.doLoadData()
# except Exception as e:
#     print e
#     raise e
# 
# try: 
#     reducer.doPlotDspacing()
# except Exception as e:
#     print e
# 
# try: 
#     reducer.doPlotQ()
# except Exception as e:
#     print e
# 
# Skip if there is something wrong
app.exec_()

