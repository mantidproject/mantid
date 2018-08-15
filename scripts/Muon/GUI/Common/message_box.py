from qtpy import QtGui, QtWidgets

def warning(error):
    QtWidgets.QMessageBox.warning(QtWidgets.QWidget(), "Error", str(error))


class WarningBox():

    def __init__(self):
        pass

    def warning(self, error):
        QtWidgets.QMessageBox.warning(QtWidgets.QWidget(), "Error", str(error))