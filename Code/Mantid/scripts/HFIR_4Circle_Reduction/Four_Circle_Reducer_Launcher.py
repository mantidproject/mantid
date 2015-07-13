import sys
from PyQt4 import QtGui

import reduce4circleGUI 

if __name__=="__main__":
    app = QtGui.QApplication(sys.argv)
    myapp = reduce4circleGUI.MainWindow()
    myapp.show()

    exit_code=app.exec_()
    sys.exit(exit_code)
