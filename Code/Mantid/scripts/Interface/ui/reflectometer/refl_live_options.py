from PyQt4 import QtCore, QtGui

try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    _fromUtf8 = lambda s: s
    
class ReflLiveOptions(QtGui.QDialog, refl_live_data.Ui_LiveDataDialog):

    frequency = 0
    method = 0

    def __init__(self):
        """
        Initialise the interface
        """
        super(QtGui.QDialog, self).__init__()
        self.setupUi(self)
        method = self.comboAccMethod # get index
        frequency = self.dspinFrequency #get value
        #connect updates to functions
        self.dspinFrequency.valueChanged.connect(self._update_frequency)
        self.comboAccMethod.activated.connect(self._update_method)

    def _update_frequency(self, freq):
        print type(freq)

    def _update_method(self, meth):
        print type(meth)

    def __del__(self):
        pass
