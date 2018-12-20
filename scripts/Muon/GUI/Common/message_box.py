import PyQt4.QtGui as QtGui


def warning(error):

    ex = QtGui.QWidget()
    QtGui.QMessageBox.warning(ex, "Error", str(error))
