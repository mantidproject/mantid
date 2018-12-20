
import PyQt4.QtGui as QtGui


def mockQapp():
    qapp = QtGui.QApplication.instance()
    if qapp is None:
        return QtGui.QApplication([''])
    else:
        return qapp
