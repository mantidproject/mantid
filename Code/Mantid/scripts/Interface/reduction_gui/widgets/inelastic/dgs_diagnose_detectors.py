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
        # Constraints
        dvp = QtGui.QDoubleValidator()
        dvp.setBottom(0.0)
        self._content.errorbar_crit_edit.setValidator(dvp)
        self._content.median_test_high_edit.setValidator(dvp)
        self._content.median_test_low_edit.setValidator(dvp)
        self._content.prop_change_crit_edit.setValidator(dvp)
        self._content.accept_factor_edit.setValidator(dvp)
        ivp = QtGui.QIntValidator()
        ivp.setBottom(0)
        self._content.tof_start_edit.setValidator(ivp)
        self._content.tof_end_edit.setValidator(ivp)

        # Connections
        self.connect(self._content.output_mask_browse, QtCore.SIGNAL("clicked()"), 
                     self._output_mask_browse)
        self.connect(self._content.det_van1_browse, QtCore.SIGNAL("clicked()"), 
                     self._det_van1_browse)
        self.connect(self._content.det_van2_browse, QtCore.SIGNAL("clicked()"), 
                     self._det_van2_browse)

    def _output_mask_browse(self):
        fname = self.data_browse_dialog()
        if fname:
            self._content.output_mask_edit.setText(fname)   

    def _det_van1_browse(self):
        fname = self.data_browse_dialog()
        if fname:
            self._content.det_van1_edit.setText(fname)   

    def _det_van2_browse(self):
        fname = self.data_browse_dialog()
        if fname:
            self._content.det_van2_edit.setText(fname)   
    
    def set_state(self, state):
        self._content.find_bad_det_gb.setChecked(state.find_bad_detectors)
        self._content.output_mask_edit.setText(state.output_mask_file)
        self._content.errorbar_crit_edit.setText(QtCore.QString(str(state.errorbar_criterion)))
        self._content.det_van1_edit.setText(state.det_van1)
        self._content.high_counts_edit.setText(QtCore.QString("%1.e" % state.high_counts))
        self._content.low_counts_edit.setText(QtCore.QString(str(state.low_counts)))
        self._content.median_test_high_edit.setText(QtCore.QString(str(state.median_test_high)))
        self._content.median_test_low_edit.setText(QtCore.QString(str(state.median_test_low)))
        self._content.det_van2_edit.setText(state.det_van2)
        self._content.prop_change_crit_edit.setText(QtCore.QString(str(state.prop_change_criterion)))
        self._content.background_check_gb.setChecked(state.background_check)
        self._content.accept_factor_edit.setText(QtCore.QString(str(state.acceptance_factor)))
        self._content.tof_start_edit.setText(QtCore.QString(str(state.tof_start)))
        self._content.tof_end_edit.setText(QtCore.QString(str(state.tof_end)))
        self._content.reject_zero_bg_cb.setChecked(state.reject_zero_bkg)
        self._content.psd_bleed_gb.setChecked(state.psd_bleed)
        self._content.max_framerate_edit.setText(state.max_framerate)
        self._content.ignored_pixels_edit.setText(state.ignored_pixels)
    
    def get_state(self):
        d = DiagnoseDetectorsScript()
        d.find_bad_detectors = self._content.find_bad_det_gb.isChecked()
        d.output_mask_file = self._content.output_mask_edit.text()
        d.errorbar_criterion = util._check_and_get_float_line_edit(self._content.errorbar_crit_edit)
        d.det_van1 = self._content.det_van1_edit.text()
        d.high_counts = util._check_and_get_float_line_edit(self._content.high_counts_edit)
        d.low_counts = util._check_and_get_float_line_edit(self._content.low_counts_edit)
        d.median_test_high = util._check_and_get_float_line_edit(self._content.median_test_high_edit)
        d.median_test_low = util._check_and_get_float_line_edit(self._content.median_test_low_edit)
        d.det_van2 = self._content.det_van2_edit.text()
        d.prop_change_criterion = util._check_and_get_float_line_edit(self._content.prop_change_crit_edit)
        d.background_check = self._content.background_check_gb.isChecked()
        d.acceptance_factor = util._check_and_get_float_line_edit(self._content.accept_factor_edit)
        d.tof_start = util._check_and_get_int_line_edit(self._content.tof_start_edit)
        d.tof_end = util._check_and_get_int_line_edit(self._content.tof_end_edit)
        d.reject_zero_bkg = self._content.reject_zero_bg_cb.isChecked()
        d.psd_bleed = self._content.psd_bleed_gb.isChecked()
        d.max_framerate = self._content.max_framerate_edit.text()
        d.ignored_pixels = self._content.ignored_pixels_edit.text()
        return d
