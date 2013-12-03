"""
    Script used to start the ISIS Reflectomery GUI from MantidPlot
"""
from PyQt4 import QtGui
#, QtCore
from ui.reflectometer import refl_gui

class ConfirmQMainWindow(QtGui.QMainWindow):
    def __init__(self, ui):
        super(ConfirmQMainWindow, self).__init__()
        self.modFlag = False
        self.gui = ui
    def closeEvent(self, event):
        if self.modFlag:
            event.ignore()
            msgBox = QtGui.QMessageBox()
            msgBox.setText("The table has been modified. Do you want to save your changes?")
            msgBox.setStandardButtons(QtGui.QMessageBox.Save | QtGui.QMessageBox.Discard | QtGui.QMessageBox.Cancel)
            msgBox.setIcon(QtGui.QMessageBox.Question)
            msgBox.setDefaultButton(QtGui.QMessageBox.Save)
            msgBox.setEscapeButton(QtGui.QMessageBox.Cancel)
            ret = msgBox.exec_()
            if ret == QtGui.QMessageBox.Save:
                if self.gui.save():
                    event.accept()
            elif ret == QtGui.QMessageBox.Discard:
                event.accept()
ui = refl_gui.ReflGui()
MainWindow = ConfirmQMainWindow(ui)
ui.setupUi(MainWindow)
MainWindow.show()
