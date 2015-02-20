#pylint: disable=invalid-name
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

    def __init__(self, parent=None, state=None, settings=None, data_type=None):
        super(AbsoluteUnitsWidget, self).__init__(parent, state, settings, data_type=data_type)

        class AbsUnitsFrame(QtGui.QFrame, ui.inelastic.ui_dgs_absolute_units.Ui_AbsUnitsFrame):
            def __init__(self, parent=None):
                QtGui.QFrame.__init__(self, parent)
                self.setupUi(self)

        self._content = AbsUnitsFrame(self)
        self._layout.addWidget(self._content)
        self.initialize_content()
        self._instrument_name = settings.instrument_name

        if state is not None:
            self.set_state(state)
        else:
            self.set_state(AbsoluteUnitsScript(self._instrument_name))

    def initialize_content(self):
        # Constraints
        for widget in [
                       self._content.ei_edit,
                       self._content.van_mass_edit,
                       self._content.sample_mass_edit,
                       self._content.sample_rmm_edit,
                       self._content.median_test_high_edit,
                       self._content.median_test_low_edit,
                       self._content.median_test_out_high_edit,
                       self._content.median_test_out_low_edit,
                       self._content.errorbar_crit_edit,
                       ]:

            dvp = QtGui.QDoubleValidator(widget)
            dvp.setBottom(0.0)
            widget.setValidator(dvp)

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
        self._content.emin_edit.setText(str(state.emin))
        self._content.emax_edit.setText(str(state.emax))
        self._content.van_mass_edit.setText(str(state.vanadium_mass))
        self._content.sample_mass_edit.setText(str(state.sample_mass))
        self._content.sample_rmm_edit.setText(str(state.sample_rmm))
        self._content.median_test_high_edit.setText(str(state.absunits_median_test_high))
        self._content.median_test_low_edit.setText(str(state.absunits_median_test_low))
        self._content.median_test_out_high_edit.setText(str(state.absunits_median_test_out_high))
        self._content.median_test_out_low_edit.setText(str(state.absunits_median_test_out_low))
        self._content.errorbar_crit_edit.setText(str(state.absunits_errorbar_criterion))

    def get_state(self):
        """
            Returns an object with the state of the interface
        """
        a = AbsoluteUnitsScript(self._instrument_name)
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
        a.absunits_median_test_high = util._check_and_get_float_line_edit(self._content.median_test_high_edit)
        a.absunits_median_test_low = util._check_and_get_float_line_edit(self._content.median_test_low_edit)
        a.absunits_median_test_out_high = util._check_and_get_float_line_edit(self._content.median_test_out_high_edit)
        a.absunits_median_test_out_low = util._check_and_get_float_line_edit(self._content.median_test_out_low_edit)
        a.absunits_errorbar_criterion = util._check_and_get_float_line_edit(self._content.errorbar_crit_edit)
        return a

    def live_button_toggled_actions(self,checked):
        if checked:
            self._old_absunits = self._content.absunits_gb.isChecked()
            self._content.absunits_gb.setChecked(False)
        else:
            try:
                self._content.absunits_gb.setChecked(self._old_absunits)
            except:  # This is for if the live button started out checked
                pass
        self._content.absunits_gb.setEnabled(not checked)
