from PyQt4 import QtGui, uic, QtCore
from functools import partial
from reduction_gui.widgets.base_widget import BaseWidget
from reduction_gui.reduction.inelastic.dgs_diagnose_detectors_script import DiagnoseDetectorsScript
import reduction_gui.widgets.util as util
import ui.inelastic.ui_dgs_diagnose_detectors

class DiagnoseDetectorsWidget(BaseWidget):
    """
        Widget that presents data correction options to the user.
    """ 
    ## Widget name
    name = "Diagnose Detectors"
    
    def __init__(self, parent=None, state=None, settings=None):
        super(DiagnoseDetectorsWidget, self).__init__(parent, state, settings)

        class DiagDetsFrame(QtGui.QFrame, ui.inelastic.ui_dgs_diagnose_detectors.Ui_DiagDetsFrame): 
            def __init__(self, parent=None):
                QtGui.QFrame.__init__(self, parent)
                self.setupUi(self)
                
        self._content = DiagDetsFrame(self)
        self._layout.addWidget(self._content)
        self.initialize_content()
        
        if state is not None:
            self.set_state(state)
        else:
            self.set_state(DiagnoseDetectorsScript())

    def initialize_content(self):
        pass
    
    def set_state(self, state):
        self._content.find_bad_det_gb.setChecked(state.find_bad_detectors)
    
    def get_state(self):
        d = DiagnoseDetectorsScript()
        d.find_bad_detectors = self._content.find_bad_det_gb.isChecked()
        return d