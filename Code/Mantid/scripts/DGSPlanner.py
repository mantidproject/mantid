from DGSPlanner import DGSPlannerGUI
from PyQt4 import QtGui
import sys

def qapp(): 
    if QtGui.QApplication.instance(): 
        app = QtGui.QApplication.instance() 
    else: 
        app = QtGui.QApplication(sys.argv) 
    return app
    
if __name__ == '__main__': 
    app = qapp()
    planner = DGSPlannerGUI.DGSPlannerGUI()
    planner.show()
    try: #check if started from within mantidplot
        import mantidplot
    except:
        sys.exit(app.exec_()) 
