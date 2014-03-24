import refl_live_data
from PyQt4 import QtCore, QtGui

try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    _fromUtf8 = lambda s: s

class ReflLiveOptions(QtGui.QDialog, refl_live_data.Ui_LiveDataDialog):

    frequency = 0
    _method = 0
    _method_list = ["Add","Replace","Append"]

    def __init__(self, def_meth = "Add", def_freq = float(60)):
        """
        Initialise the interface
        """
        super(QtGui.QDialog, self).__init__()
        self.setupUi(self)
        self.comboAccMethod.addItems(self._method_list)

        if def_meth in self._method_list:
            self.comboAccMethod.setCurrentIndex(self._method_list.index(def_meth))
        else:
            self.comboAccMethod.setCurrentIndex(0)

        self.dspinFrequency.setValue(def_freq)

        _method = self.comboAccMethod.currentIndex
        frequency = self.dspinFrequency.value

        #connect update signals to functions
        self.dspinFrequency.valueChanged.connect(self._update_frequency)
        self.comboAccMethod.activated.connect(self._update_method)

    def _update_frequency(self, freq):
        self.frequency = freq

    def _update_method(self, meth):
        self._method = meth

    def get_method(self):
        return self._method_list[self._method]
