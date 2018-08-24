from qtpy import QtWidgets


def warning(error):
    ex = QtWidgets.QWidget()
    QtWidgets.QMessageBox.warning(ex, "Error", str(error))
