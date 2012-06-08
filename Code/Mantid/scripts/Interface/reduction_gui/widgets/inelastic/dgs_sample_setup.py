from PyQt4 import QtGui, uic, QtCore
from reduction_gui.widgets.base_widget import BaseWidget
from reduction_gui.reduction.inelastic.dgs_sample_data_setup_script import SampleSetupScript
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
        self.initialize_content()
        
        if state is not None:
            self.set_state(state)
        else:
            self.set_state(SampleSetupScript())
        
    def initialize_content(self):
        # Connections
        self.connect(self._content.sample_browse, QtCore.SIGNAL("clicked()"), 
                     self._sample_browse)

    def _sample_browse(self):
        fname = self.data_browse_dialog()
        if fname:
            self._content.sample_edit.setText(fname)   
            
    def set_state(self, state):
        """
            Populate the UI elements with the data from the given state.
            @param state: SampleSetupScript object
        """
        self._content.sample_edit.setText(state.sample_data)
    
    def get_state(self):
        """
            Returns an object with the state of the interface
        """
        s = SampleSetupScript()
        s.sample_data = self._content.sample_edit.text()
        return s