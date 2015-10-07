#!/usr/bin/python
#pylint: disable=C0103,relative-import
import sys
from PyQt4 import QtGui

import reduce4circleGUI

if __name__=="__main__":
    q_app = QtGui.QApplication(sys.argv)
    my_app = reduce4circleGUI.MainWindow()
    my_app.show()

    exit_code=q_app.exec_()
    sys.exit(exit_code)
