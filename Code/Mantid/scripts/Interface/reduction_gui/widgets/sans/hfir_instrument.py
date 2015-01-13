from PyQt4 import QtGui, uic, QtCore
import reduction_gui.widgets.util as util
import math
import os
from reduction_gui.reduction.sans.hfir_options_script import ReductionOptions
from reduction_gui.settings.application_settings import GeneralSettings
from reduction_gui.widgets.base_widget import BaseWidget
import ui.sans.ui_hfir_instrument

IS_IN_MANTIDPLOT = False
try:
    import mantidplot
    IS_IN_MANTIDPLOT = True
except:
    pass

class SANSInstrumentWidget(BaseWidget):
    """
        Widget that present instrument details to the user
    """
    ## Widget name
    name = "Reduction Options"

    # Place holder for data read from file
    _sample_detector_distance = None
    _sample_detector_distance_supplied = True
    _beam_diameter = None
    _beam_diameter_supplied = True
    _wavelength = None
    _wavelength_supplied = True
    _wavelength_spread = None

    # Internal data members for mask editor logic
    mask_file = ''
    mask_reload = False
    mask_ws = "__hfir_mask"

    def __init__(self, parent=None, state=None, settings=None, name="BIOSANS", data_proxy=None):
        super(SANSInstrumentWidget, self).__init__(parent, state, settings, data_proxy=data_proxy)

        class SummaryFrame(QtGui.QFrame, ui.sans.ui_hfir_instrument.Ui_Frame):
            def __init__(self, parent=None):
                QtGui.QFrame.__init__(self, parent)
                self.setupUi(self)

        self._summary = SummaryFrame(self)
        self.initialize_content()
        self._layout.addWidget(self._summary)

        self._masked_detectors = []

        if state is not None:
            self.set_state(state)
        else:
            instr = ReductionOptions()
            instr.instrument_name = name
            self.set_state(instr)

        # General GUI settings
        if settings is None:
            settings = GeneralSettings()
        self._settings = settings
        # Connect do UI data update
        self._settings.data_updated.connect(self._data_updated)

    def _data_updated(self, key, value):
        """
            Respond to application-level key/value pair updates.
            @param key: key string
            @param value: value string
        """
        if key == "sample_detector_distance":
            self._sample_detector_distance = value
            if not self._summary.sample_dist_chk.isChecked():
                self._summary.sample_dist_edit.setText(str(value))
                util._check_and_get_float_line_edit(self._summary.sample_dist_edit, min=0.0)
        elif key == "wavelength":
            self._wavelength = value
            if not self._summary.wavelength_chk.isChecked():
                self._summary.wavelength_edit.setText(str(value))
                util._check_and_get_float_line_edit(self._summary.wavelength_edit, min=0.0)
        elif key == "wavelength_spread":
            self._wavelength_spread = value
            if not self._summary.wavelength_chk.isChecked():
                self._summary.wavelength_spread_edit.setText(str(value))
        elif key == "beam_diameter":
            value_float = float(value)
            self._beam_diameter = "%-6.1f" % value_float
            if not self._summary.beamstop_chk.isChecked():
                self._summary.scale_beam_radius_edit.setText(self._beam_diameter)
                util._check_and_get_float_line_edit(self._summary.scale_beam_radius_edit, min=0.0)

    def content(self):
        return self._summary

    def initialize_content(self):
        # Validators
        self._summary.detector_offset_edit.setValidator(QtGui.QDoubleValidator(self._summary.detector_offset_edit))
        self._summary.sample_dist_edit.setValidator(QtGui.QDoubleValidator(self._summary.sample_dist_edit))
        self._summary.wavelength_edit.setValidator(QtGui.QDoubleValidator(self._summary.wavelength_edit))
        self._summary.wavelength_spread_edit.setValidator(QtGui.QDoubleValidator(self._summary.wavelength_spread_edit))
        self._summary.n_q_bins_edit.setValidator(QtGui.QIntValidator(self._summary.n_q_bins_edit))
        self._summary.n_sub_pix_edit.setValidator(QtGui.QIntValidator(self._summary.n_sub_pix_edit))

        # Event connections
        self.connect(self._summary.detector_offset_chk, QtCore.SIGNAL("clicked(bool)"), self._det_offset_clicked)
        self.connect(self._summary.sample_dist_chk, QtCore.SIGNAL("clicked(bool)"), self._sample_dist_clicked)
        self.connect(self._summary.wavelength_chk, QtCore.SIGNAL("clicked(bool)"), self._wavelength_clicked)

        self.connect(self._summary.dark_current_check, QtCore.SIGNAL("clicked(bool)"), self._dark_clicked)
        self.connect(self._summary.dark_browse_button, QtCore.SIGNAL("clicked()"), self._dark_browse)
        self.connect(self._summary.dark_plot_button, QtCore.SIGNAL("clicked()"), self._dark_plot_clicked)
        self.connect(self._summary.normalization_none_radio, QtCore.SIGNAL("clicked()"), self._normalization_clicked)
        self.connect(self._summary.normalization_time_radio, QtCore.SIGNAL("clicked()"), self._normalization_clicked)
        self.connect(self._summary.normalization_monitor_radio, QtCore.SIGNAL("clicked()"), self._normalization_clicked)

        # Q range
        self._summary.n_q_bins_edit.setText("100")
        self._summary.n_sub_pix_edit.setText("1")
        self.connect(self._summary.log_binning_radio, QtCore.SIGNAL("clicked(bool)"), self._summary.align_check.setEnabled)

        self._summary.scale_edit.setText("1")
        
        self._summary.n_wedges_edit.setText("2")
        self._summary.wedge_angle_edit.setText("30")
        self._summary.wedge_offset_edit.setText("0")

        self._summary.instr_name_label.hide()
        self._dark_clicked(self._summary.dark_current_check.isChecked())

        # Mask Connections
        self.connect(self._summary.mask_browse_button, QtCore.SIGNAL("clicked()"), self._mask_browse_clicked)
        self.connect(self._summary.mask_plot_button, QtCore.SIGNAL("clicked()"), self._mask_plot_clicked)
        self.connect(self._summary.mask_check, QtCore.SIGNAL("clicked(bool)"), self._mask_checked)

        # Absolute scale connections and validators
        self._summary.scale_edit.setValidator(QtGui.QDoubleValidator(self._summary.scale_edit))
        self._summary.scale_beam_radius_edit.setValidator(QtGui.QDoubleValidator(self._summary.scale_beam_radius_edit))
        self._summary.scale_att_trans_edit.setValidator(QtGui.QDoubleValidator(self._summary.scale_att_trans_edit))
        self.connect(self._summary.scale_data_browse_button, QtCore.SIGNAL("clicked()"), self._scale_data_browse)
        self.connect(self._summary.scale_data_plot_button, QtCore.SIGNAL("clicked()"), self._scale_data_plot_clicked)
        self.connect(self._summary.beamstop_chk, QtCore.SIGNAL("clicked(bool)"), self._beamstop_clicked)
        self.connect(self._summary.scale_chk, QtCore.SIGNAL("clicked(bool)"), self._scale_clicked)
        self._scale_clicked(self._summary.scale_chk.isChecked())

        # If we are not in debug/expert mode, hide some advanced options
        if not self._settings.debug:
            self._summary.mask_side_layout.deleteLater()
            self._summary.mask_side_label.hide()
            self._summary.mask_side_none_radio.hide()
            self._summary.mask_side_front_radio.hide()
            self._summary.mask_side_back_radio.hide()
            
        if not self._in_mantidplot:
            self._summary.dark_plot_button.hide()
            self._summary.scale_data_plot_button.hide()

    def _mask_plot_clicked(self):
        ws_name = os.path.basename(str(self._summary.mask_edit.text()))
        self.mask_ws = "__mask_%s" % ws_name
        self.show_instrument(self._summary.mask_edit.text, workspace=self.mask_ws, tab=2, reload=self.mask_reload, mask=self._masked_detectors)
        self._masked_detectors = []
        self.mask_reload = False

    def _mask_browse_clicked(self):
        fname = self.data_browse_dialog()
        if fname:
            self._summary.mask_edit.setText(fname)
            self.mask_reload = True

    def _mask_checked(self, is_checked):
        self._summary.mask_edit.setEnabled(is_checked)
        self._summary.mask_browse_button.setEnabled(is_checked)
        self._summary.mask_plot_button.setEnabled(is_checked)

    def _scale_data_plot_clicked(self):
        self.show_instrument(file_name=self._summary.scale_data_edit.text)

    def _dark_plot_clicked(self):
        self.show_instrument(file_name=self._summary.dark_file_edit.text)

    def _normalization_clicked(self):
        if self._summary.normalization_none_radio.isChecked():
            self._summary.scale_chk.setChecked(False)
            self._scale_clicked(False)
            self._summary.scale_chk.setEnabled(False)
        else:
            self._summary.scale_chk.setEnabled(True)

    def _beamstop_clicked(self, is_checked):
        self._summary.scale_beam_radius_edit.setEnabled(is_checked and self._summary.scale_chk.isChecked())

        # Keep track of current value so we can restore it if the check box is clicked again
        if self._beam_diameter_supplied != is_checked:
            current_value = util._check_and_get_float_line_edit(self._summary.scale_beam_radius_edit)
            self._summary.scale_beam_radius_edit.setText(str(self._beam_diameter))
            util._check_and_get_float_line_edit(self._summary.scale_beam_radius_edit, min=0.0)
            self._beam_diameter = current_value
            self._beam_diameter_supplied = is_checked

    def _scale_clicked(self, is_checked):
        self._summary.direct_beam_label.setEnabled(is_checked)
        self._summary.att_trans_label.setEnabled(is_checked)
        self._summary.beamstop_chk.setEnabled(is_checked)
        self._summary.scale_data_edit.setEnabled(is_checked)
        self._summary.scale_data_plot_button.setEnabled(is_checked)
        self._summary.scale_data_browse_button.setEnabled(is_checked)
        self._summary.scale_att_trans_edit.setEnabled(is_checked)
        self._summary.scale_beam_radius_edit.setEnabled(is_checked and self._summary.beamstop_chk.isChecked())

        self._summary.att_scale_factor_label.setEnabled(not is_checked)
        self._summary.scale_edit.setEnabled(not is_checked)

    def _scale_data_browse(self):
        fname = self.data_browse_dialog()
        if fname:
            self._summary.scale_data_edit.setText(fname)

    def _det_offset_clicked(self, is_checked):
        self._summary.detector_offset_edit.setEnabled(is_checked)

        if is_checked:
            self._summary.sample_dist_chk.setChecked(not is_checked)
            self._summary.sample_dist_edit.setEnabled(not is_checked)
            self._sample_dist_clicked(not is_checked)

    def _sample_dist_clicked(self, is_checked):
        self._summary.sample_dist_edit.setEnabled(is_checked)

        if is_checked:
            self._summary.detector_offset_chk.setChecked(not is_checked)
            self._summary.detector_offset_edit.setEnabled(not is_checked)

        # Keep track of current value so we can restore it if the check box is clicked again
        if self._sample_detector_distance_supplied != is_checked:
            current_value = util._check_and_get_float_line_edit(self._summary.sample_dist_edit)
            self._summary.sample_dist_edit.setText(str(self._sample_detector_distance))
            util._check_and_get_float_line_edit(self._summary.sample_dist_edit, min=0)
            self._sample_detector_distance = current_value

            self._sample_detector_distance_supplied = is_checked

    def _wavelength_clicked(self, is_checked):
        self._summary.wavelength_edit.setEnabled(is_checked)
        self._summary.wavelength_spread_edit.setEnabled(is_checked)

        # Keep track of current value so we can restore it if the check box is clicked again
        if self._wavelength_supplied != is_checked:
            current_value = util._check_and_get_float_line_edit(self._summary.wavelength_edit)
            self._summary.wavelength_edit.setText(str(self._wavelength))
            util._check_and_get_float_line_edit(self._summary.wavelength_edit, min=0)
            self._wavelength = current_value

            current_value = util._check_and_get_float_line_edit(self._summary.wavelength_spread_edit)
            self._summary.wavelength_spread_edit.setText(str(self._wavelength_spread))
            util._check_and_get_float_line_edit(self._summary.wavelength_spread_edit)
            self._wavelength_spread = current_value

            self._wavelength_supplied = is_checked


    def _dark_clicked(self, is_checked):
        self._summary.dark_file_edit.setEnabled(is_checked)
        self._summary.dark_browse_button.setEnabled(is_checked)
        self._summary.dark_plot_button.setEnabled(is_checked)

    def _dark_browse(self):
        fname = self.data_browse_dialog()
        if fname:
            self._summary.dark_file_edit.setText(fname)

    def set_state(self, state):
        """
            Populate the UI elements with the data from the given state.
            @param state: InstrumentDescription object
        """
        self._summary.instr_name_label.setText(state.instrument_name)
        #npixels = "%d x %d" % (state.nx_pixels, state.ny_pixels)
        #self._summary.n_pixel_label.setText(QtCore.QString(npixels))
        #self._summary.pixel_size_label.setText(QtCore.QString(str(state.pixel_size)))

        # Absolute scaling
        self._summary.scale_chk.setChecked(state.calculate_scale)
        self._summary.scale_edit.setText(str(state.scaling_factor))
        self._summary.scale_data_edit.setText(state.scaling_direct_file)
        self._summary.scale_att_trans_edit.setText(str(state.scaling_att_trans))

        self._summary.scale_beam_radius_edit.setText(str("%-6.1f" % state.scaling_beam_diam))
        if self._beam_diameter is None:
            self._beam_diameter = state.scaling_beam_diam
        self._beam_diameter_supplied = state.manual_beam_diam
        self._summary.beamstop_chk.setChecked(state.manual_beam_diam)
        util._check_and_get_float_line_edit(self._summary.scale_beam_radius_edit, min=0.0)

        self._scale_clicked(self._summary.scale_chk.isChecked())

        # Detector offset input
        self._prepare_field(state.detector_offset != 0,
                            state.detector_offset,
                            self._summary.detector_offset_chk,
                            self._summary.detector_offset_edit)

        # Sample-detector distance
        self._prepare_field(state.sample_detector_distance != 0,
                            state.sample_detector_distance,
                            self._summary.sample_dist_chk,
                            self._summary.sample_dist_edit)
        util._check_and_get_float_line_edit(self._summary.sample_dist_edit, min=0)
        if self._sample_detector_distance is None:
            self._sample_detector_distance = state.sample_detector_distance
        self._sample_detector_distance_supplied = self._summary.sample_dist_chk.isChecked()

        # Sample-detector distance takes precedence over offset if both are non-zero
        self._sample_dist_clicked(self._summary.sample_dist_chk.isChecked())

        # Wavelength value
        self._prepare_field(state.wavelength != 0,
                            state.wavelength,
                            self._summary.wavelength_chk,
                            self._summary.wavelength_edit,
                            state.wavelength_spread,
                            self._summary.wavelength_spread_edit)
        if self._wavelength is None:
            self._wavelength = state.wavelength
        if self._wavelength_spread is None:
            self._wavelength_spread = state.wavelength_spread
        self._wavelength_supplied = self._summary.wavelength_chk.isChecked()

        # Solid angle correction flag
        self._summary.solid_angle_chk.setChecked(state.solid_angle_corr)

        # Dark current
        self._summary.dark_current_check.setChecked(state.dark_current_corr)
        self._summary.dark_file_edit.setText(state.dark_current_data)
        self._dark_clicked(self._summary.dark_current_check.isChecked())

        # Normalization
        if state.normalization == state.NORMALIZATION_NONE:
            self._summary.normalization_none_radio.setChecked(True)
        elif state.normalization == state.NORMALIZATION_TIME:
            self._summary.normalization_time_radio.setChecked(True)
        elif state.normalization == state.NORMALIZATION_MONITOR:
            self._summary.normalization_monitor_radio.setChecked(True)

        # Q range
        self._summary.n_q_bins_edit.setText(str(state.n_q_bins))
        self._summary.n_sub_pix_edit.setText(str(state.n_sub_pix))
        self._summary.log_binning_radio.setChecked(state.log_binning)
        self._summary.align_check.setEnabled(state.log_binning)
        self._summary.align_check.setChecked(state.align_log_with_decades)
        
        self._summary.n_wedges_edit.setText(str(state.n_wedges))
        self._summary.wedge_angle_edit.setText(str(state.wedge_angle))
        self._summary.wedge_offset_edit.setText(str(state.wedge_offset))
        
        # Mask
        self._summary.mask_edit.setText(str(state.mask_file))
        self._summary.mask_check.setChecked(state.use_mask_file)
        self._mask_checked(state.use_mask_file)
        self._masked_detectors = state.detector_ids
        self.mask_reload = True
        
        if state.masked_side == 'Front':
            self._summary.mask_side_front_radio.setChecked(True)
        elif state.masked_side == 'Back':
            self._summary.mask_side_back_radio.setChecked(True)
        else:
            self._summary.mask_side_none_radio.setChecked(True)

    def _prepare_field(self, is_enabled, stored_value, chk_widget, edit_widget, suppl_value=None, suppl_edit=None):
        #to_display = str(stored_value) if is_enabled else ''
        edit_widget.setEnabled(is_enabled)
        chk_widget.setChecked(is_enabled)
        edit_widget.setText(str(stored_value))
        if suppl_value is not None and suppl_edit is not None:
            suppl_edit.setEnabled(is_enabled)
            suppl_edit.setText(str(suppl_value))

    def get_state(self):
        """
            Returns an object with the state of the interface
        """
        m = ReductionOptions()

        m.instrument_name = self._summary.instr_name_label.text()

        # Absolute scaling
        m.scaling_factor = util._check_and_get_float_line_edit(self._summary.scale_edit)
        m.calculate_scale = self._summary.scale_chk.isChecked()
        m.scaling_direct_file = unicode(self._summary.scale_data_edit.text())
        m.scaling_att_trans = util._check_and_get_float_line_edit(self._summary.scale_att_trans_edit)
        m.scaling_beam_diam = util._check_and_get_float_line_edit(self._summary.scale_beam_radius_edit, min=0.0)
        m.manual_beam_diam = self._summary.beamstop_chk.isChecked()

        # Detector offset input
        if self._summary.detector_offset_chk.isChecked():
            m.detector_offset = util._check_and_get_float_line_edit(self._summary.detector_offset_edit)

        # Sample-detector distance
        if self._summary.sample_dist_chk.isChecked():
            m.sample_detector_distance = util._check_and_get_float_line_edit(self._summary.sample_dist_edit)

        # Wavelength value
        wavelength = util._check_and_get_float_line_edit(self._summary.wavelength_edit, min=0.0)
        if self._summary.wavelength_chk.isChecked():
            m.wavelength = wavelength
            m.wavelength_spread = util._check_and_get_float_line_edit(self._summary.wavelength_spread_edit)

        # Solid angle correction
        m.solid_angle_corr = self._summary.solid_angle_chk.isChecked()

        # Dark current
        m.dark_current_corr = self._summary.dark_current_check.isChecked()
        m.dark_current_data = unicode(self._summary.dark_file_edit.text())

        # Normalization
        if self._summary.normalization_none_radio.isChecked():
            m.normalization = m.NORMALIZATION_NONE
        elif self._summary.normalization_time_radio.isChecked():
            m.normalization = m.NORMALIZATION_TIME
        elif self._summary.normalization_monitor_radio.isChecked():
            m.normalization = m.NORMALIZATION_MONITOR

        # Q range
        m.n_q_bins = util._check_and_get_int_line_edit(self._summary.n_q_bins_edit)
        m.n_sub_pix = util._check_and_get_int_line_edit(self._summary.n_sub_pix_edit)
        m.log_binning = self._summary.log_binning_radio.isChecked()
        m.align_log_with_decades = self._summary.align_check.isChecked()

        m.n_wedges = util._check_and_get_int_line_edit(self._summary.n_wedges_edit)
        m.wedge_angle = util._check_and_get_float_line_edit(self._summary.wedge_angle_edit)
        m.wedge_offset = util._check_and_get_float_line_edit(self._summary.wedge_offset_edit)

        # Detector side masking
        if self._summary.mask_side_front_radio.isChecked():
            m.masked_side = 'Front'
        elif self._summary.mask_side_back_radio.isChecked():
            m.masked_side = 'Back'
        else:
            m.masked_side = None
            
        # Mask detector IDs
        m.use_mask_file = self._summary.mask_check.isChecked()
        m.mask_file = unicode(self._summary.mask_edit.text())
        m.detector_ids = self._masked_detectors
        if self._in_mantidplot:
            from mantid.api import AnalysisDataService
            import mantid.simpleapi as api
            if AnalysisDataService.doesExist(self.mask_ws):
                ws, masked_detectors = api.ExtractMask(InputWorkspace=self.mask_ws, OutputWorkspace="__edited_mask")
                m.detector_ids = [int(i) for i in masked_detectors]

        self._settings.emit_key_value("DARK_CURRENT", str(self._summary.dark_file_edit.text()))
        return m
