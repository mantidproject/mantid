from PyQt4 import QtGui, uic, QtCore
import os
from reduction_gui.settings.application_settings import GeneralSettings

IS_IN_MANTIDPLOT = False
try:
    import qti
    IS_IN_MANTIDPLOT = True
except:
    pass

class BaseWidget(QtGui.QWidget):    
    """
        Base widget for reduction UI
    """
    ## Widget name
    name = ""      
    
    def __init__(self, parent=None, state=None, settings=None, data_type=None, ui_class=None, data_proxy=None):
        QtGui.QWidget.__init__(self, parent)
        
        self._layout = QtGui.QHBoxLayout()
        self.setLayout(self._layout)
        if ui_class is not None:
            self._content = ui_class(self)
            self._layout.addWidget(self._content)

        # Data filter for file dialog
        self._data_type="Data files (*.xml)"
        if data_type is not None:
            self._data_type = data_type

        # General GUI settings
        if settings is None:
            settings = GeneralSettings()
        self._settings = settings

        if ui_class is not None:
            self.setLayout(self._layout)
            self.initialize_content()
            
        self._instrument_view = None
        
        self._data_proxy = data_proxy
        self._in_mantidplot = IS_IN_MANTIDPLOT and self._data_proxy is not None
        
    def initialize_content(self):
        """
            Declare the validators and event connections for the 
            widgets loaded through the .ui file.
        """
        return NotImplemented
    
    def set_state(self, state):
        """   
            Populate the UI elements with the data from the given state.
            @param state: InstrumentDescription object
        """
        return NotImplemented
    
    def get_state(self):
        """
            Returns an object with the state of the interface
        """
        return NotImplemented
    
    def data_browse_dialog(self, data_type=None, multi=False):
        """
            Pop up a file dialog box.
            @param data_type: string used to filter the files
            @param multi: multiselection is enabled if True
        """
        if data_type is None:
            data_type = self._data_type
        if multi:
            qflist = QtGui.QFileDialog.getOpenFileNames(self, "Data file - Choose a data file",
                                                              self._settings.data_path, 
                                                              data_type)
            if qflist.count()>0:
                flist = []
                for i in range(qflist.count()):
                    flist.append(unicode(qflist[i]))
                # Store the location of the loaded file
                (folder, file_name) = os.path.split(flist[0])
                self._settings.data_path = folder
                return flist
            else:
                return None
        else:       
            fname = unicode(QtGui.QFileDialog.getOpenFileName(self, "Data file - Choose a data file",
                                                              self._settings.data_path, 
                                                              data_type))
            if fname:
                # Store the location of the loaded file
                (folder, file_name) = os.path.split(fname)
                self._settings.data_path = folder
            return fname     
    
    def show_instrument(self, file_name=''):
        """
            Show instrument for the given data file.
            @param file_name: Data file path
        """
        if not IS_IN_MANTIDPLOT:
            return
        
        # Do nothing if the instrument view is already displayed
        if self._instrument_view is not None and self._instrument_view.isVisible():
            return

        if self._data_proxy is not None:
            proxy = self._data_proxy(file_name)
            
        self._instrument_view = qti.app.mantidUI.getInstrumentView(proxy.data_ws)
        self._instrument_view.show()
    
    