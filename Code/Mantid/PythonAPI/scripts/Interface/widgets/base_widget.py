from PyQt4 import QtGui, uic, QtCore
import os
from application_settings import GeneralSettings

class BaseWidget(QtGui.QWidget):    
    """
        Base widget for reduction UI
    """
    
    def __init__(self, parent=None, state=None, ui_path='ui', settings=None):
        QtGui.QWidget.__init__(self, parent)
        
        self._layout = QtGui.QHBoxLayout()
        self._content = QtGui.QFrame(self)
        self._layout.addWidget(self._content)

        self.setLayout(self._layout)
        
        # General GUI settings
        if settings is None:
            settings = GeneralSettings()
        self._settings = settings
        
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