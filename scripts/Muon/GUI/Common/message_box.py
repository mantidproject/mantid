import PyQt4.QtGui as QtGui


def warning(error):

    ex = QtGui.QWidget()
    QtGui.QMessageBox.warning(ex, "Frequency Domain Analysis", str(error))
