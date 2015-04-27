###############################################################################
# Tester: Test GUI's functionality: Merge runs
#
# Next:
# (-) Add an option/option groups such that the new reduced data can be plot \
#     on a clean canvas or over plot on the original one;
# (-) An inner sequence for line-color-marker-style of the plot should be made
# (-) Shall add button to load 'next' and 'previous' 
# (-) Make Ge 113 In Config and etc a comboBox for wavelength
# (-) Add tool bar to plot for save/zoom in and out and etc.
# (-) Label of the plots
#
#
# ( -) A dictionary should be used to manage the history data
# ( -) Merge runs
# ( -) Proper quit
# (10) Need a configuration file to load
# ( -) Auto reset the x-y-limit of the plot
# ( -) Design for vanadium peak strip
# (13) Implementation for vanadium peak strip
# (14) doPlotUnitDspace: auto fill minD, maxD, binsizeD
# (15) During loading, ui.label_ptNo and ui.label_detNo should give out the \
#      range of pt. and det number
#
# (16) Tab-Merge Scans: Button View 2D
# (17) Tab-Merge Scans: Button Save
# (18) Tab-Vanadium: Fix Button 'Strip Vanadium Peaks'
# (19) Tab-Vanadium: Button 'Save'
# (20) Tab-Advanced Setup: Auto fill Server Address
# (21) Test using Local Data Storage
#
# Use cases to test in this script 
# 1. Load a set of scans in an experiment;
#  > plot merged result as default.
# 2. PushButton 'View 2D': color fill plot on all individual experiments;
# 3. PushButton 'View Merged': 1D plot for previously merged;
# 4. Mode 'Show scan number' alog with 'View 2D';   
#  > Show to user which scan is at the mouse;
# 5. Right click mouse to get a submenu for 'include/exclude' (disable one)
# 6. Select or deselect a run from GUI and re-merge;
#
# Note: step 4, 5 and 6 should be referred to PlotData.MainAppNDim.py
#
###############################################################################
import sys

import HfirPDReductionGUI
from PyQt4 import QtGui

# Globals
LINUX = 1
OSX   = 2

def qapp():
    if QtGui.QApplication.instance():
        _app = QtGui.QApplication.instance()
    else:
        _app = QtGui.QApplication(sys.argv)
    return _app

app = qapp()
reducer = HfirPDReductionGUI.MainWindow() #the main ui class in this file is called MainWindow
reducer.show()

""" END OF COMMON CODES """


#-------------------------------------------------------------------------------
""" Determine OS """

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

#-------------------------------------------------------------------------------

# example: 'http://neutron.ornl.gov/user_data/hb2a/exp400/Datafiles/HB2A_exp0400_scan0001.dat'
#
# Test case: Experiment 231, Scan 1 to 10
#
print "Set to exp 380, scan 1-10"
reducer.ui.lineEdit_expNo.setText('380')
reducer.ui.lineEdit_scanStart.setText('1')
reducer.ui.lineEdit_scanEnd.setText('8')
reducer.ui.lineEdit_exclScans.setText('2,4-6')
#reducer.ui.lineEdit_wavelength.setText('2.41')

if MOS == LINUX:
    reducer.ui.lineEdit_cache.setText('/home/wzz/Temp/')
elif MOS == OSX: 
    reducer.ui.lineEdit_cache.setText('/Users/wzz/Temp/')

reducer.ui.lineEdit_mergeMinX.setText('5.0')
reducer.ui.lineEdit_mergeMaxX.setText('150.0')
reducer.ui.lineEdit_mergeBinSize.setText('0.1')


# Load multiple sets of data and plot in 2D
reducer.doLoadSetData()

# Merge!

# load and reduce data 
# reducer.doMergeScans()


#-------------------------------------------------------------------------------
""" Common Codes """
app.exec_()

