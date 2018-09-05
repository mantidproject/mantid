import PyQt4.QtGui as QtGui


def warning(error, parent=None):
    if not parent:
        parent = QtGui.QWidget()
    QtGui.QMessageBox.warning(parent, "Error", str(error))
