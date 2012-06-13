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

    def initialize_content(self):
        # Make group for incident beam normalisation radio buttons
        self.incident_beam_norm_grp = QtGui.QButtonGroup()
        self.incident_beam_norm_grp.addButton(self._content.none_rb, 0)
        self.incident_beam_norm_grp.addButton(self._content.current_rb, 1)
        self.incident_beam_norm_grp.addButton(self._content.monitor1_rb, 2) 
    
        self._monitor_intrange_widgets_state(self._content.monitor1_rb.isChecked())
        self.connect(self._content.monitor1_rb, QtCore.SIGNAL("toggled(bool)"), 
                     self._monitor_intrange_widgets_state)
        
    def _monitor_intrange_widgets_state(self, state=False):
        self._content.monint_label.setEnabled(state)
        self._content.monint_low_edit.setEnabled(state)
        self._content.monint_high_edit.setEnabled(state)        
    
    def set_state(self, state):
        """
            Populate the UI elements with the data from the given state.
            @param state: SampleSetupScript object
        """
        self._content.filter_bad_pulses_chkbox.setChecked(state.filter_bad_pulses)
        button_index = DataCorrectionsScript.INCIDENT_BEAM_NORM_TYPES.index(state.incident_beam_norm)
        cbutton = self.incident_beam_norm_grp.button(button_index)
        cbutton.setChecked(True)

    def get_state(self):
        """
            Returns an object with the state of the interface
        """
        d = DataCorrectionsScript()
        d.filter_bad_pulses = self._content.filter_bad_pulses_chkbox.isChecked()
        d.incident_beam_norm = DataCorrectionsScript.INCIDENT_BEAM_NORM_TYPES[self.incident_beam_norm_grp.checkedId()]
        return d