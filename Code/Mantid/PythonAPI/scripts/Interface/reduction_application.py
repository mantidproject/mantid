import sys
from PyQt4 import QtGui, uic
from instruments.instrument_factory import instrument_factory
from reduction.hfir_reduction import ReductionScripter
        
class ReductionGUI(QtGui.QMainWindow):
    def __init__(self, instrument=None):
        QtGui.QMainWindow.__init__(self)
        
        # Name handle for the instrument
        self._instrument = instrument
        
        # Reduction interface
        self._interface = None
        
    def setup_layout(self):
        #TODO: add instrument selection to top menu

        self._interface = instrument_factory(self._instrument)
        self.tabWidget.clear()
        
        tab_dict = self._interface.get_tabs()
        for tab in tab_dict:
            self.tabWidget.addTab(tab_dict[tab], tab)

    def reduce_clicked(self):
        """
            Create an object capable of using the information in the
            interface and turn it into a reduction process.
        """
        self._interface.reduce()
        
        
if __name__ == '__main__':
    app = QtGui.QApplication(sys.argv)
    
    # Instrument selection
    #TODO: pick up list of instrument from settings file
    dialog = uic.loadUi("ui/instrument_dialog.ui")
    dialog.exec_()
    if dialog.result()==1:
        reducer = ReductionGUI(dialog.instr_combo.currentText())
        uic.loadUi("ui/reduction_main.ui", reducer)
        reducer.setup_layout()
        reducer.show()
        sys.exit(app.exec_())
        