import sys, os
from PyQt4 import QtGui, QtCore, uic

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
        
        # Recent files
        self._recent_files = []
        
        # Folder to open files in
        self._last_directory = '.'
        
    def setup_layout(self):
        #TODO: add instrument selection to top menu
        self._update_file_menu()

        self._interface = instrument_factory(self._instrument)
        self._interface.ui_path = self.ui_path
        self.tabWidget.clear()
        
        tab_dict = self._interface.get_tabs()
        for tab in tab_dict:
            self.tabWidget.addTab(tab_dict[tab], tab)

    def _update_file_menu(self):
        """
            Update the menu with recent files
        """
        return NotImplemented  

    def reduce_clicked(self):
        """
            Create an object capable of using the information in the
            interface and turn it into a reduction process.
        """
        self._interface.reduce()
        
    def open_file(self, file_path):
        self._interface.load_file(file_path)
        
    def file_open(self, *argv):
        """
            File chooser for loading UI parameters
        """
        fname = unicode(QtGui.QFileDialog.getOpenFileName(self, "Reduction settings - Choose a settings file",
                                                          self._last_directory, 
                                                          "Settings files (*.xml)"))
        
        if fname:
            # Store the location of the loaded file
            (folder, file_name) = os.path.split(fname)
            self._last_directory = folder
            
            if fname in self._recent_files:
                self._recent_files.prepend(QtCore.QString(fname))
                while self._recent_files.count() > 9:
                    self._recent_files.takeLast()
            
            self.open_file(fname)
            
        
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

        
