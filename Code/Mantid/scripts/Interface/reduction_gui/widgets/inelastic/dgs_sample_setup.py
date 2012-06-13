from PyQt4 import QtGui, uic, QtCore
from functools import partial
from reduction_gui.widgets.base_widget import BaseWidget
from reduction_gui.reduction.inelastic.dgs_sample_data_setup_script import SampleSetupScript
import reduction_gui.widgets.util as util
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
        # Constraints
        dv = QtGui.QDoubleValidator()
        dv.setBottom(0.0)
        self._content.ei_edit.setValidator(dv)
        util.set_valid(self._content.ei_edit, False)
        dv1 = QtGui.QDoubleValidator()
        self._content.etr_low_edit.setValidator(dv1)
        util.set_valid(self._content.etr_low_edit, False)
        self._content.etr_width_edit.setValidator(dv1)
        util.set_valid(self._content.etr_width_edit, False)
        self._content.etr_high_edit.setValidator(dv1)
        util.set_valid(self._content.etr_high_edit, False)
        
        # Connections
        self.connect(self._content.sample_browse, QtCore.SIGNAL("clicked()"), 
                     self._sample_browse)
        self.connect(self._content.hardmask_browse, QtCore.SIGNAL("clicked()"), 
                     self._hardmask_browse)
        self.connect(self._content.grouping_browse, QtCore.SIGNAL("clicked()"), 
                     self._grouping_browse)
        # Validated widgets
        self._connect_validated_lineedit(self._content.ei_edit)
        self._connect_validated_lineedit(self._content.etr_low_edit)
        self._connect_validated_lineedit(self._content.etr_width_edit)
        self._connect_validated_lineedit(self._content.etr_high_edit)

    def _connect_validated_lineedit(self, ui_ctrl):
        call_back = partial(self._validate_edit, ctrl=ui_ctrl)
        self.connect(ui_ctrl, QtCore.SIGNAL("editingFinished()"), call_back)
        self.connect(ui_ctrl, QtCore.SIGNAL("textEdited(QString)"), call_back)

    def _validate_edit(self, ctrl=None):
        is_valid = True
        if ctrl.text().isEmpty():
            is_valid = False
        util.set_valid(ctrl, is_valid)

    def _sample_browse(self):
        fname = self.data_browse_dialog()
        if fname:
            self._content.sample_edit.setText(fname)   

    def _hardmask_browse(self):
        fname = self.data_browse_dialog()
        if fname:
            self._content.hardmask_edit.setText(fname)   

    def _grouping_browse(self):
        fname = self.data_browse_dialog()
        if fname:
            self._content.grouping_edit.setText(fname)   
            
    def set_state(self, state):
        """
            Populate the UI elements with the data from the given state.
            @param state: SampleSetupScript object
        """
        self._content.sample_edit.setText(state.sample_data)
        self._content.ei_edit.setText(state.incident_energy)
        self._content.fixed_ei_chkbox.setChecked(state.fixed_ei)
        self._content.etr_low_edit.setText(state.et_range_low)
        self._content.etr_width_edit.setText(state.et_range_width)
        self._content.etr_high_edit.setText(state.et_range_high)
        self._content.hardmask_edit.setText(state.hardmask_file)
        self._content.grouping_edit.setText(state.grouping_file)
    
    def get_state(self):
        """
            Returns an object with the state of the interface
        """
        s = SampleSetupScript()
        s.sample_data = self._content.sample_edit.text()
        s.incident_energy = self._content.ei_edit.text()
        s.fixed_ei = self._content.fixed_ei_chkbox.isChecked()
        s.et_range_low = self._content.etr_low_edit.text()
        s.et_range_width = self._content.etr_width_edit.text()
        s.et_range_high = self._content.etr_high_edit.text()
        s.hardmask_file = self._content.hardmask_edit.text()
        s.grouping_file = self._content.grouping_edit.text()    
        return s
