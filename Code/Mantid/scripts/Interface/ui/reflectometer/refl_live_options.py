from PyQt4 import QtCore, QtGui

try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    _fromUtf8 = lambda s: s
    
class ReflLiveOptions(QtGui.QDialog, refl_live_data.Ui_LiveDataDialog):
    def __init__(self):
        """
        Initialise the interface
        """
        super(QtGui.QDialog, self).__init__()
        self.setupUi(self)
    
    def __del__(self):
        pass