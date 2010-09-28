from PyQt4 import QtGui, uic, QtCore
from reduction_gui.settings.application_settings import GeneralSettings
from base_widget import BaseWidget

class OutputWidget(BaseWidget):    
    """
        Widget that presents the transmission options to the user
    """
    
    def __init__(self, parent=None, state=None, settings=None):
        BaseWidget.__init__(self, parent=parent, state=state, settings=settings)

        f = QtCore.QFile(":hfir_output.ui")
        f.open(QtCore.QIODevice.ReadOnly)
        uic.loadUi(f, self._content)
        self.initialize_content()
    
    def set_log(self, log_text):
        self._content.output_text_edit.setText(QtCore.QString(log_text))
    
    def initialize_content(self):
        """
            Declare the validators and event connections for the 
            widgets loaded through the .ui file.
        """

        # Clear data list
        self._content.output_text_edit.clear()
        
    