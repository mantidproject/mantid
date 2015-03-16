####################
# Tester 
####################

""" Test main """
import sys

import HfirPDReductionGUI
from PyQt4 import QtGui

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
print "Set to exp 400, scan 1"
reducer.ui.lineEdit_expNo.setText('400')
reducer.ui.lineEdit_scanNo.setText('1')
reducer.ui.lineEdit_cache.setText('/Users/wzz/Temp/')
reducer.ui.lineEdit_binsize.setText('0.1')
try:
    reducer.doLoadData()
except Exception as e:
    print e

try: 
    reducer.doPlotDspacing()
except Exception as e:
    print e

try: 
    reducer.doPlotQ()
except Exception as e:
    print e

app.exec_()

