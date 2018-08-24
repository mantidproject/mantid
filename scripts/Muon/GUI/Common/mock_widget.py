from qtpy import QtWidgets

def mockQapp():
    qapp = QtWidgets.QApplication.instance()
    if qapp is None:
        return QtWidgets.QApplication([''])
    else:
        return qapp
