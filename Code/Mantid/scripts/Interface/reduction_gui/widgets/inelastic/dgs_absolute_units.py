from PyQt4 import QtGui, uic, QtCore
from functools import partial
from reduction_gui.widgets.base_widget import BaseWidget
from reduction_gui.reduction.inelastic.dgs_absolute_units_script import AbsoluteUnitsScript
import reduction_gui.widgets.util as util
import ui.inelastic.ui_dgs_absolute_units

class AbsoluteUnitsWidget(BaseWidget):
    """
        Widget that presents absolute units normalisation options to the user.
    """ 
    ## Widget name
    name = "Absolute Units"
    
    def __init__(self, parent=None, state=None, settings=None):
        super(AbsoluteUnitsWidget, self).__init__(parent, state, settings)

        class AbsUnitsFrame(QtGui.QFrame, ui.inelastic.ui_dgs_absolute_units.Ui_AbsUnitsFrame): 
            def __init__(self, parent=None):
                QtGui.QFrame.__init__(self, parent)
                self.setupUi(self)
                
        self._content = AbsUnitsFrame(self)
        self._layout.addWidget(self._content)
        self.initialize_content()
        
        if state is not None:
            self.set_state(state)
        else:
            self.set_state(AbsoluteUnitsScript())

    def initialize_content(self):
        # Constraints
        dvp = QtGui.QDoubleValidator(self._content.ei_edit)
        dvp.setBottom(0.0)
        self._content.ei_edit.setValidator(dvp)
        
        # Connections
        self.connect(self._content.absunits_van_browse, QtCore.SIGNAL("clicked()"), 
                     self._absunits_van_browse)
        self.connect(self._content.absunits_detvan_browse, QtCore.SIGNAL("clicked()"), 
                     self._absunits_detvan_browse)
        self.connect(self._content.grouping_file_browse, QtCore.SIGNAL("clicked()"), 
                     self._grouping_file_browse)

    def _absunits_van_browse(self):
        fname = self.data_browse_dialog()
        if fname:
            self._content.absunits_van_edit.setText(fname)   

    def _absunits_detvan_browse(self):
        fname = self.data_browse_dialog()
        if fname:
            self._content.absunits_detvan_edit.setText(fname)   

    def _grouping_file_browse(self):
        fname = self.data_browse_dialog()
        if fname:
            self._content.grouping_file_edit.setText(fname)   
    
    def set_state(self, state):
        """
            Populate the UI elements with the data from the given state.
            @param state: AbsoluteUnitsScript object
        """
        self._content.absunits_gb.setChecked(state.do_absolute_units)
        self._content.absunits_van_edit.setText(state.absunits_vanadium)
        self._content.grouping_file_edit.setText(state.grouping_file)
        self._content.absunits_detvan_edit.setText(state.absunits_detector_vanadium)
        self._content.ei_edit.setText(state.incident_energy)
        self._content.emin_edit.setText(QtCore.QString(str(state.emin)))
        self._content.emax_edit.setText(QtCore.QString(str(state.emax)))
        self._content.van_mass_edit.setText(QtCore.QString(str(state.vandium_mass)))
        self._content.sample_mass_edit.setText(QtCore.QString(str(state.sample_mass)))
        self._content.sample_rmm_edit.setText(QtCore.QString(str(state.sample_rmm)))
        
    def get_state(self):
        """
            Returns an object with the state of the interface
        """
        a = AbsoluteUnitsScript()
        a.do_absolute_units = self._content.absunits_gb.isChecked()
        a.absunits_vanadium = self._content.absunits_van_edit.text()
        a.grouping_file = self._content.grouping_file_edit.text()
        a.absunits_detector_vanadium = self._content.absunits_detvan_edit.text()
        a.incident_energy = self._content.ei_edit.text()
        a.emin = util._check_and_get_float_line_edit(self._content.emin_edit)
        a.emax = util._check_and_get_float_line_edit(self._content.emax_edit)
        a.vanadium_mass = util._check_and_get_float_line_edit(self._content.van_mass_edit)
        a.sample_mass = util._check_and_get_float_line_edit(self._content.sample_mass_edit)
        a.sample_rmm = util._check_and_get_float_line_edit(self._content.sample_rmm_edit)
        return a
        
