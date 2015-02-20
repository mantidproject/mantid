#pylint: disable=invalid-name
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

    def __init__(self, parent=None, state=None, settings=None, data_type=None):
        super(DiagnoseDetectorsWidget, self).__init__(parent, state, settings, data_type=data_type)

        class DiagDetsFrame(QtGui.QFrame, ui.inelastic.ui_dgs_diagnose_detectors.Ui_DiagDetsFrame):
            def __init__(self, parent=None):
                QtGui.QFrame.__init__(self, parent)
                self.setupUi(self)

        self._content = DiagDetsFrame(self)
        self._layout.addWidget(self._content)
        self.initialize_content()
        self._instrument_name = settings.instrument_name

        if state is not None:
            self.set_state(state)
        else:
            self.set_state(DiagnoseDetectorsScript(self._instrument_name))

    def initialize_content(self):
        # Constraints
        for widget in [
                       self._content.median_test_high_edit,
                       self._content.median_test_low_edit,
                       self._content.median_test_out_high_edit,
                       self._content.median_test_out_low_edit,
                       self._content.errorbar_crit_edit,
                       self._content.ratio_var_crit_edit,
                       self._content.sambkg_median_test_high_edit,
                       self._content.sambkg_median_test_low_edit,
                       self._content.sambkg_errorbar_crit_edit
                       ]:

            dvp = QtGui.QDoubleValidator(widget)
            dvp.setBottom(0.0)
            widget.setValidator(dvp)

        for widget in [self._content.tof_start_edit,
                       self._content.tof_end_edit]:
            ivp = QtGui.QIntValidator(widget)
            ivp.setBottom(0)
            widget.setValidator(ivp)

        # Connections
        self.connect(self._content.det_van2_browse, QtCore.SIGNAL("clicked()"),
                     self._det_van2_browse)

    def _det_van2_browse(self):
        fname = self.data_browse_dialog()
        if fname:
            self._content.det_van2_edit.setText(fname)

    def set_state(self, state):
        """
            Populate the UI elements with the data from the given state.
            @param state: DiagnoseDetectorsScript object
        """
        self._content.high_counts_edit.setText(str("%1.e" % state.high_counts))
        self._content.low_counts_edit.setText(str(state.low_counts))
        self._content.median_test_high_edit.setText(str(state.median_test_high))
        self._content.median_test_low_edit.setText(str(state.median_test_low))
        self._content.median_test_out_high_edit.setText(str(state.median_test_out_high))
        self._content.median_test_out_low_edit.setText(str(state.median_test_out_low))
        self._content.errorbar_crit_edit.setText(str(state.errorbar_criterion))
        self._content.det_van2_edit.setText(state.det_van2)
        self._content.ratio_var_crit_edit.setText(str(state.detvan_ratio_var))
        self._content.background_check_gb.setChecked(state.background_check)
        self._content.sambkg_median_test_high_edit.setText(str(state.sambkg_median_test_high))
        self._content.sambkg_median_test_low_edit.setText(str(state.sambkg_median_test_low))
        self._content.sambkg_errorbar_crit_edit.setText(str(state.sambkg_errorbar_criterion))
        self._content.tof_start_edit.setText(str(state.tof_start))
        self._content.tof_end_edit.setText(str(state.tof_end))
        self._content.reject_zero_bg_cb.setChecked(state.reject_zero_bkg)
        self._content.psd_bleed_gb.setChecked(state.psd_bleed)
        self._content.max_framerate_edit.setText(str(state.max_framerate))
        self._content.ignored_pixels_edit.setText(str(state.ignored_pixels))

    def get_state(self):
        """
            Returns an object with the state of the interface
        """
        d = DiagnoseDetectorsScript(self._instrument_name)
        d.high_counts = util._check_and_get_float_line_edit(self._content.high_counts_edit)
        d.low_counts = util._check_and_get_float_line_edit(self._content.low_counts_edit)
        d.median_test_high = util._check_and_get_float_line_edit(self._content.median_test_high_edit)
        d.median_test_low = util._check_and_get_float_line_edit(self._content.median_test_low_edit)
        d.median_test_out_high = util._check_and_get_float_line_edit(self._content.median_test_out_high_edit)
        d.median_test_out_low = util._check_and_get_float_line_edit(self._content.median_test_out_low_edit)
        d.errorbar_criterion = util._check_and_get_float_line_edit(self._content.errorbar_crit_edit)
        d.det_van2 = self._content.det_van2_edit.text()
        d.detvan_ratio_var = util._check_and_get_float_line_edit(self._content.ratio_var_crit_edit)
        d.background_check = self._content.background_check_gb.isChecked()
        d.sambkg_median_test_high = util._check_and_get_float_line_edit(self._content.sambkg_median_test_high_edit)
        d.sambkg_median_test_low = util._check_and_get_float_line_edit(self._content.sambkg_median_test_low_edit)
        d.sambkg_errorbar_criterion = util._check_and_get_float_line_edit(self._content.sambkg_errorbar_crit_edit)
        d.tof_start = util._check_and_get_float_line_edit(self._content.tof_start_edit)
        d.tof_end = util._check_and_get_float_line_edit(self._content.tof_end_edit)
        d.reject_zero_bkg = self._content.reject_zero_bg_cb.isChecked()
        d.psd_bleed = self._content.psd_bleed_gb.isChecked()
        d.max_framerate = util._check_and_get_float_line_edit(self._content.max_framerate_edit)
        d.ignored_pixels = util._check_and_get_float_line_edit(self._content.ignored_pixels_edit)
        return d
