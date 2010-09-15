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
        self._last_export_directory = '.'
        
        # Current file name
        self._filename = None
        
    def _set_window_title(self):
        title = "%s Reduction" % self._instrument
        if self._filename is not None:
            title += ": %s" % self._filename
        self.setWindowTitle(title)
    
        
    def setup_layout(self):
        #TODO: add instrument selection to top menu
        self._update_file_menu()

        self._interface = instrument_factory(self._instrument)
        self._interface.ui_path = self.ui_path
        self.tabWidget.clear()
        
        tab_dict = self._interface.get_tabs()
        for tab in tab_dict:
            self.tabWidget.addTab(tab_dict[tab], tab)
            
        self._set_window_title()

    def _update_file_menu(self):
        """
            Update the menu with recent files
        """
        openAction = QtGui.QAction("&Open...", self)
        openAction.setShortcut("Ctrl+O")
        openAction.setStatusTip("Open an XML file containing reduction parameters")
        self.connect(openAction, QtCore.SIGNAL("triggered()"), self._file_open)
    
        saveAsAction = QtGui.QAction("Save as...", self)
        saveAsAction.setStatusTip("Save the reduction parameters to XML")
        self.connect(saveAsAction, QtCore.SIGNAL("triggered()"), self._save_as)
    
        saveAction = QtGui.QAction("&Save...", self)
        saveAction.setShortcut("Ctrl+S")
        saveAction.setStatusTip("Save the reduction parameters to XML")
        self.connect(saveAction, QtCore.SIGNAL("triggered()"), self._save)
    
        exportAction = QtGui.QAction("&Export...", self)
        exportAction.setShortcut("Ctrl+E")
        exportAction.setStatusTip("Export to python script for Mantid")
        self.connect(exportAction, QtCore.SIGNAL("triggered()"), self._export)
    
        quitAction = QtGui.QAction("&Quit", self)
        quitAction.setShortcut("Ctrl+Q")
        self.connect(quitAction, QtCore.SIGNAL("triggered()"), self._quit)
    
        self.file_menu.clear()
        self.file_menu.addAction(openAction)
        self.file_menu.addAction(saveAction)
        self.file_menu.addAction(saveAsAction)
        self.file_menu.addAction(exportAction)
        self.file_menu.addAction(quitAction)
        
        recent_files = []
        for fname in self._recent_files:
            if fname != self._filename and QtCore.QFile.exists(fname) and not fname in recent_files:
                recent_files.append(fname)
                
        if len(recent_files)>0:
            self.file_menu.addSeparator()
            for i, fname in enumerate(recent_files):
                action = QtGui.QAction("&%d %s" % (i+1, QtCore.QFileInfo(fname).fileName()), self)
                action.setData(QtCore.QVariant(fname))
                self.connect(action, QtCore.SIGNAL("triggered()"), self.open_file)
                self.file_menu.addAction(action)

    def _quit(self):
        sys.exit()

    def reduce_clicked(self):
        """
            Create an object capable of using the information in the
            interface and turn it into a reduction process.
        """
        self._interface.reduce()
        
    def open_file(self, file_path=None):
        if file_path is None:
            action = self.sender()
            if isinstance(action, QtGui.QAction):
                file_path = unicode(action.data().toString())
            
        self._interface.load_file(file_path)
        self._recent_files.append(file_path)
        self._filename = file_path
        self._update_file_menu()
        self._set_window_title()
        
    def _file_open(self, *argv):
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
            
    def _save(self):
        if self._filename is None:
            self._save_as()
        else:
            try:
                self._interface.save_file(self._filename)
                self._update_file_menu()
                self.statusBar().showMessage("Saved as %s" % self._filename)
                self._set_window_title()
            except:
                #TODO: put this in a log window, and in a file
                print sys.exc_value
                self.statusBar().showMessage("Failed to save %s" % self._filename)
            
            
    def _save_as(self):
        fname = self._filename if self._filename is not None else '.'
        
        fname = unicode(QtGui.QFileDialog.getSaveFileName(self, "Reduction settings - Save settings",
                                                          self._last_directory, 
                                                          "Settings files (*.xml)"))
        
        if fname is not None:
            if '.' not in fname:
                fname += ".xml"
            self._recent_files.append(fname)
            (folder, file_name) = os.path.split(fname)
            self._last_directory = folder
            self._filename = fname
            self._save()
        
    def _export(self):
        fname = '.'
        if self._filename is not None:
            (root, ext) = os.path.splitext(self._filename)
            fname = root
            
        fname = unicode(QtGui.QFileDialog.getSaveFileName(self, "Mantid Python script - Save script",
                                                          self._last_export_directory, 
                                                          "Python script (*.py)"))
        
        if fname is not None:
            if '.' not in fname:
                fname += ".py"
            (folder, file_name) = os.path.split(fname)
            self._last_export_directory = folder
            self._interface.export(fname)
            self.statusBar().showMessage("Saved as %s" % fname)

        
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

        
