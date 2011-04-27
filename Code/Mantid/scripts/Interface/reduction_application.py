import sys, os
from PyQt4 import QtGui, QtCore, uic

from reduction_gui.instruments.instrument_factory import instrument_factory, INSTRUMENT_DICT
from reduction_gui.settings.application_settings import GeneralSettings
import reduction_gui.settings.qrc_resources
import ui.ui_reduction_main
import ui.ui_instrument_dialog

IS_IN_MANTIDPLOT = False
try:
    import qti
    IS_IN_MANTIDPLOT = True
except:
    pass
    

# Check whether Mantid is available
try:
    from MantidFramework import *
    mtd.initialise(False)
    HAS_MANTID = True
except:
    HAS_MANTID = False    

class ReductionGUI(QtGui.QMainWindow, ui.ui_reduction_main.Ui_SANSReduction):
    def __init__(self, instrument=None, instrument_list=None):
        QtGui.QMainWindow.__init__(self)
        
        # Application settings
        settings = QtCore.QSettings()
        
        # Name handle for the instrument
        if instrument is None:
            instrument = unicode(settings.value("instrument_name", QtCore.QVariant('')).toString())

        self._instrument = instrument
        
        # List of allowed instrument
        self._instrument_list = instrument_list
        
        # Reduction interface
        self._interface = None
        
        # Recent files
        self._recent_files = settings.value("recent_files", QtCore.QVariant([])).toStringList()
        
        # Folder to open files in
        self._last_directory = unicode(settings.value("last_directory", QtCore.QVariant('.')).toString())
        self._last_export_directory = unicode(settings.value("last_export_directory", QtCore.QVariant('.')).toString())
        
        # Current file name
        self._filename = None   
        
        # Internal flag for clearing all settings and restarting the application
        self._clear_and_restart = False
        
        # General settings shared by all widgets
        self.general_settings = GeneralSettings(settings)
        
        self.setupUi(self)
        
        # Event connections
        if not HAS_MANTID:
            self.reduce_button.hide()
        self.connect(self.export_button, QtCore.SIGNAL("clicked()"), self._export)
        self.connect(self.reduce_button, QtCore.SIGNAL("clicked()"), self.reduce_clicked)  
        
        # Of the widgets that are part of the application, one is the ApplicationWindow.
        # The ApplicationWindow will send a shutting_down() signal when quitting,
        # after which we should close this window.
        # Note: there is no way to identify which Widget is the ApplicationWindow.
        for w in QtCore.QCoreApplication.instance().topLevelWidgets():
            self.connect(w, QtCore.SIGNAL("shutting_down()"), self.close)
            
        self.general_settings.progress.connect(self._progress_updated)        

    def _set_window_title(self):
        """
            Sets the window title using the instrument name and the 
            current settings file
        """
        title = "%s Reduction" % self._instrument
        if self._filename is not None:
            title += ": %s" % self._filename
        self.setWindowTitle(title)
    
    def _progress_updated(self, value):
        self.progress_bar.setValue(value)
        
    def setup_layout(self):
        """
            Sets up the instrument-specific part of the UI layout
        """
        if self._instrument == '':
            self._change_instrument()
            if self._instrument == '':
                self.close()
                return
        
        self._update_file_menu()

        # Clean up the widgets that have already been created
        if self._interface is not None:
            self._interface.destroy()
            
        self._interface = instrument_factory(self._instrument, settings=self.general_settings)
        
        if self._interface is not None:
            self.tabWidget.clear()
            
            tab_list = self._interface.get_tabs()
            for tab in tab_list:
                self.tabWidget.addTab(tab[1], tab[0])
                
            self._set_window_title()
            self.progress_bar.hide()
        else:
            self.close()
            
    def _update_file_menu(self):
        """
            Set up the File menu and update the menu with recent files
        """
        self.file_menu.clear()

        newAction = QtGui.QAction("&New...", self)
        newAction.setShortcut("Ctrl+N")
        newAction.setStatusTip("Start a new reduction")
        self.connect(newAction, QtCore.SIGNAL("triggered()"), self._new)
    
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
        self.connect(quitAction, QtCore.SIGNAL("triggered()"), self.close)
        
        self.file_menu.addAction(newAction)
        self.file_menu.addAction(openAction)
        self.file_menu.addAction(saveAction)
        self.file_menu.addAction(saveAsAction)
        self.file_menu.addAction(exportAction)
        self.file_menu.addSeparator()

        if self.general_settings.debug:
            clearAction = QtGui.QAction("&Clear settings and quit", self)
            clearAction.setStatusTip("Restore initial application settings and close the application")
            self.connect(clearAction, QtCore.SIGNAL("triggered()"), self._clear_and_close)
            self.file_menu.addAction(clearAction)
            
        self.file_menu.addAction(quitAction)
        
        # TOOLS menu
        instrAction = QtGui.QAction("Change &instrument...", self)
        instrAction.setShortcut("Ctrl+I")
        instrAction.setStatusTip("Select a new instrument")
        self.connect(instrAction, QtCore.SIGNAL("triggered()"), self._change_instrument)
    
        debug_menu_item_str = "Turn debug mode ON"
        if self.general_settings.debug:
            debug_menu_item_str = "Turn debug mode OFF"
        debugAction = QtGui.QAction(debug_menu_item_str, self)
        debugAction.setStatusTip(debug_menu_item_str)
        self.connect(debugAction, QtCore.SIGNAL("triggered()"), self._debug_mode)
    
        self.tools_menu.clear()
        self.tools_menu.addAction(instrAction)
        self.tools_menu.addAction(debugAction)
        
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

    def _debug_mode(self, mode=None):
        """
            Set debug mode
            @param mode: debug mode (True or False). If None, the debug mode will simply be flipped
        """
        if mode is None:
            mode = not self.general_settings.debug
        self.general_settings.debug = mode
        self._update_file_menu()

    def _change_instrument(self):
        """
            Invoke an instrument selection dialog
        """ 
        class InstrDialog(QtGui.QDialog, ui.ui_instrument_dialog.Ui_Dialog): 
            def __init__(self, instrument_list=None):
                QtGui.QDialog.__init__(self)
                self.instrument_list = instrument_list
                self.setupUi(self)
                self.instr_combo.clear()
                self.facility_combo.clear()
                instruments = INSTRUMENT_DICT.keys()
                instruments.sort()
                for facility in instruments:
                    self.facility_combo.addItem(QtGui.QApplication.translate("Dialog", facility, None, QtGui.QApplication.UnicodeUTF8))

                self._facility_changed(instruments[0])
                self.connect(self.facility_combo, QtCore.SIGNAL("activated(QString)"), self._facility_changed)    
                
            def _facility_changed(self, facility):
                self.instr_combo.clear()
                for item in INSTRUMENT_DICT[unicode(facility)]:
                    if self.instrument_list is None or item in self.instrument_list:
                        self.instr_combo.addItem(QtGui.QApplication.translate("Dialog", item, None, QtGui.QApplication.UnicodeUTF8))
                
        if self.general_settings.debug:
            dialog = InstrDialog()
        else:   
            dialog = InstrDialog(self._instrument_list)
        dialog.exec_()
        if dialog.result()==1:
            self._instrument = dialog.instr_combo.currentText()
            self.setup_layout()
            
    def _clear_and_close(self):
        """
            Clear all QSettings parameters
        """
        self._clear_and_restart = True
        self.close()
        # If we make it here, the user canceled the close, which 
        # means that we need to reset the clear&close flag so
        # that the state is properly saved on the next close.
        self._clear_and_restart = False

    def closeEvent(self, event):
        """
            Executed when the application closes
        """
        if False:
            reply = QtGui.QMessageBox.question(self, 'Message',
                "Are you sure you want to quit this application?", QtGui.QMessageBox.Yes, QtGui.QMessageBox.No)
    
            if reply == QtGui.QMessageBox.Yes:
                event.accept()
            else:
                event.ignore()
                
        # Save application settings
        if self._clear_and_restart:
            self._clear_and_restart = False
            QtCore.QSettings().clear()
        else:    
            settings = QtCore.QSettings()
            
            if self._instrument is not None:
                instrument = QtCore.QVariant(QtCore.QString(self._instrument))
            else: 
                instrument = QtCore.QVariant()    
            settings.setValue("instrument_name", instrument)
            
            if self._filename is not None:
                filename = QtCore.QVariant(QtCore.QString(self._filename))
            else:
                filename = QtCore.QVariant()    
            settings.setValue("last_file", filename)
            
            if self._recent_files is not []:
                recent_files = QtCore.QVariant(self._recent_files)
            else:
                recent_files = QtCore.QVariant()
            settings.setValue("recent_files", recent_files)
            
            last_dir = QtCore.QVariant(QtCore.QString(self._last_directory))
            settings.setValue("last_directory", last_dir)
    
            last_export_dir = QtCore.QVariant(QtCore.QString(self._last_export_directory))
            settings.setValue("last_export_directory", last_export_dir)
            
            # General settings
            self.general_settings.to_settings(settings)

    def reduce_clicked(self):
        """
            Create an object capable of using the information in the
            interface and turn it into a reduction process.
        """
        self.reduce_button.setEnabled(False)   
        self.export_button.setEnabled(False)
        if IS_IN_MANTIDPLOT:
            qti.app.mantidUI.setIsRunning(True)
        self._interface.reduce()
        if IS_IN_MANTIDPLOT:
            qti.app.mantidUI.setIsRunning(False)
        self.reduce_button.setEnabled(True)   
        self.export_button.setEnabled(True)
        
    def open_file(self, file_path=None):
        """
            Open an XML file and populate the UI
            @param file_path: path to the file to be loaded
        """
        if file_path is None:
            action = self.sender()
            if isinstance(action, QtGui.QAction):
                file_path = unicode(action.data().toString())
            
        # Check whether the file describes the current instrument
        found_instrument = self._interface.scripter.verify_instrument(file_path)
        if not found_instrument == self._instrument:
            self._instrument = found_instrument
            self.setup_layout()
            
        self.reduce_button.setEnabled(False)   
        self.export_button.setEnabled(False)
        self._interface.load_file(file_path)
        self.reduce_button.setEnabled(True)
        self.export_button.setEnabled(True)

        self._recent_files.append(file_path)
        self._filename = file_path
        self._update_file_menu()
        self._set_window_title()
        
    def _new(self, *argv):
        """
            Start new reduction
        """
        self._interface.reset()
        self._filename = None
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
        """
            Present a file dialog to the user and saves the content of the
            UI in XML format
        """
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
        """
            Present a file dialog to the user and saves the content of
            the UI in XML format.
        """
        if self._filename is not None:
            fname = self._filename
        else:
            fname = '.'
        
        fname = unicode(QtGui.QFileDialog.getSaveFileName(self, "Reduction settings - Save settings",
                                                          self._last_directory, 
                                                          "Settings files (*.xml)"))
        
        if len(fname)>0:
            if not fname.endswith('.xml'):
                fname += ".xml"
            self._recent_files.append(fname)
            (folder, file_name) = os.path.split(fname)
            self._last_directory = folder
            self._filename = fname
            self._save()
        
    def _export(self):
        """
            Exports the current content of the UI to a python script that can 
            be run within MantidPlot
        """
        fname = '.'
        if self._filename is not None:
            (root, ext) = os.path.splitext(self._filename)
            fname = root
            
        fname = unicode(QtGui.QFileDialog.getSaveFileName(self, "Mantid Python script - Save script",
                                                          self._last_export_directory, 
                                                          "Python script (*.py)"))
               
        if len(fname)>0:
            if not fname.endswith('.py'):
                fname += ".py"
            (folder, file_name) = os.path.split(fname)
            self._last_export_directory = folder
            script = self._interface.export(fname)
            if script is not None:
                self.statusBar().showMessage("Saved as %s" % fname)
            else:
                self.statusBar().showMessage("Could not save file")

        
def start(argv=[]):
    app = QtGui.QApplication(argv)
    app.setOrganizationName("Mantid")
    app.setOrganizationDomain("mantidproject.org")
    app.setApplicationName("Mantid Reduction")
    
    reducer = ReductionGUI()    
    reducer.setup_layout()
    reducer.show()
    app.exec_() 
        
if __name__ == '__main__':
    start(argv=sys.argv)

        
