import refl_window
from PyQt4 import QtCore, QtGui

if __name__ == "__main__":
    import sys
    app = QtGui.QApplication(sys.argv)
    MainWindow = QtGui.QMainWindow()
    ui = refl_window.ReflGuiWindow()
    ui.setupUi(MainWindow)
    MainWindow.show()
    sys.exit(app.exec_())