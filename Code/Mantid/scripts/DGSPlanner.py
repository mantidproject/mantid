from DGSPlanner import DGSPlannerGUI
from PyQt4 import QtGui
import sys

def qapp(): 
    if QtGui.QApplication.instance(): 
        app = QtGui.QApplication.instance() 
    else: 
        app = QtGui.QApplication(sys.argv) 
    return app
 
app = qapp()

planner = DGSPlannerGUI.MainWindow() #the main ui class in this file is called MainWindow
planner.show()
app.exec_()
