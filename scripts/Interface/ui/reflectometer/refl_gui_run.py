#pylint: disable=invalid-name
from __future__ import (absolute_import, division, print_function)
import refl_gui
from PyQt4 import QtGui

if __name__ == "__main__":
    import sys
    app = QtGui.QApplication(sys.argv)
    MainWindow = QtGui.QMainWindow()
    ui = refl_gui.ReflGui()
    ui.setupUi(MainWindow)
    MainWindow.show()
    sys.exit(app.exec_())
