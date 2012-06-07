from PyQt4 import QtGui, uic, QtCore
from reduction_gui.widgets.base_widget import BaseWidget
import ui.inelastic.ui_dgs_sample_setup

class SampleSetupWidget(BaseWidget):
    """
        Widget that presents sample setup options to the user.
    """ 
    ## Widget name
    name = "SampleSetup"
    
    def __init__(self, parent=None, state=None, settings=None):
        super(SampleSetupWidget, self).__init__(parent, state, settings)
        
        class SamSetFrame(QtGui.QFrame, ui.inelastic.ui_dgs_sample_setup.Ui_Frame): 
            def __init__(self, parent=None):
                QtGui.QFrame.__init__(self, parent)
                self.setupUi(self)
                
        self._content = SamSetFrame(self)
        self._layout.addWidget(self._content)