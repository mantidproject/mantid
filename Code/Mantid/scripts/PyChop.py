from mantid import config
from PyChop import fluxGUI
from PyChop import PyChopGUI
from PyQt4 import QtGui
import sys


def qapp():
    if QtGui.QApplication.instance():
    	app = QtGui.QApplication.instance()
    else:
    	app = QtGui.QApplication(sys.argv)
    return app

app = qapp()
instr_name = config['default.instrument']
if instr_name[0:3] == 'LET':
    Resolution = fluxGUI.MainWindow()#the main ui class for LET resolution
else:
    Resolution = PyChopGUI.MainWindow()
Resolution.show()
app.exec_()