import refl_gui
from PyQt4 import QtCore, QtGui

if __name__ == "__main__":
    import sys
    app = QtGui.QApplication(sys.argv)
    MainWindow = QtGui.QMainWindow()
    ui = refl_gui.ReflGui()
    ui.setupUi(MainWindow)
    MainWindow.show()
    sys.exit(app.exec_())
