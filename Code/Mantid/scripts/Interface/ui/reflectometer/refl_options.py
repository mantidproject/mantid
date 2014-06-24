import refl_options_window
from PyQt4 import QtCore, QtGui

try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    _fromUtf8 = lambda s: s

class ReflOptions(QtGui.QDialog, refl_options_window.Ui_OptionsDialog):


    frequency = 0
    _method = 0
    _method_list = ["Add","Replace","Append"]
    ads_get = False

    def __init__(self, def_meth = "Add", def_freq = float(60), def_ads = False, def_alg = False):
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
        self.checkADS.setChecked(def_ads)
        self.checkAlg.setChecked(def_alg)

        self.ads_get = self.checkADS.isChecked()
        self.alg_use = self.checkAlg.isChecked()
        self._method = self.comboAccMethod.currentIndex()
        self.frequency = self.dspinFrequency.value()
        #connect update signals to functions
        self.dspinFrequency.valueChanged.connect(self._update_frequency)
        self.comboAccMethod.activated.connect(self._update_method)
        self.checkADS.clicked.connect(self._update_ADS_get)
        self.checkAlg.clicked.connect(self._update_Alg_use)

    def _update_Alg_use(self, checked):
        self.alg_use = checked

    def _update_ADS_get(self, checked):
        self.ads_get = checked

    def _update_frequency(self, freq):
        self.frequency = freq

    def _update_method(self, meth):
        self._method = meth

    def get_method(self):
        return self._method_list[self._method]
