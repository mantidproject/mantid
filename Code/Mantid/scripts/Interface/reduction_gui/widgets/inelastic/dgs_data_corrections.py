from PyQt4 import QtGui, uic, QtCore
from functools import partial
from reduction_gui.widgets.base_widget import BaseWidget
from reduction_gui.reduction.inelastic.dgs_data_corrections_script import DataCorrectionsScript
import reduction_gui.widgets.util as util
import ui.inelastic.ui_dgs_data_corrections

class DataCorrectionsWidget(BaseWidget):
    """
        Widget that presents data correction options to the user.
    """ 
    ## Widget name
    name = "DataCorrections"
    
    def __init__(self, parent=None, state=None, settings=None):
        super(DataCorrectionsWidget, self).__init__(parent, state, settings)

        class DataCorrsFrame(QtGui.QFrame, ui.inelastic.ui_dgs_data_corrections.Ui_DataCorrsFrame): 
            def __init__(self, parent=None):
                QtGui.QFrame.__init__(self, parent)
                self.setupUi(self)
                
        self._content = DataCorrsFrame(self)
        self._layout.addWidget(self._content)
        self.initialize_content()
        
        if state is not None:
            self.set_state(state)
        else:
            self.set_state(DataCorrectionsScript())

    def initalize_content(self):
        pass
    
    def set_state(self, state):
        """
            Populate the UI elements with the data from the given state.
            @param state: SampleSetupScript object
        """
        self._content.filter_bad_pulses_chkbox.setChecked(state.filter_bad_pulses)

    def get_state(self):
        """
            Returns an object with the state of the interface
        """
        d = DataCorrectionsScript()
        d.filter_bad_pulses = self._content.filter_bad_pulses_chkbox.isChecked()
        return d