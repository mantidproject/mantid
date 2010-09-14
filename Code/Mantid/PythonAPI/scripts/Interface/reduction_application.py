import sys, os
from PyQt4 import QtGui, uic

from instruments.instrument_factory import instrument_factory
from reduction.hfir_reduction import ReductionScripter
     

class ReductionGUI(QtGui.QMainWindow):
    def __init__(self, instrument=None, ui_path="ui"):
        QtGui.QMainWindow.__init__(self)
        
        # Directory where to find the .ui files
        self.ui_path = ui_path
        
        # Name handle for the instrument
        self._instrument = instrument
        
        # Reduction interface
        self._interface = None
        
    def setup_layout(self):
        #TODO: add instrument selection to top menu

        self._interface = instrument_factory(self._instrument)
        self._interface.ui_path = self.ui_path
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
        
def start(ui_path="ui"):
    app = QtGui.QApplication([])
    
    # Instrument selection
    #TODO: pick up list of instrument from settings file
    dialog = uic.loadUi(os.path.join(ui_path,"instrument_dialog.ui"))
    dialog.exec_()
    if dialog.result()==1:
        reducer = ReductionGUI(dialog.instr_combo.currentText(), ui_path=ui_path)
        uic.loadUi(os.path.join(ui_path,"reduction_main.ui"), reducer)
        reducer.setup_layout()
        reducer.show()
        sys.exit(app.exec_())   
        
if __name__ == '__main__':
    start()

        
