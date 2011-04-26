from PyQt4 import QtGui, uic, QtCore
import reduction_gui.widgets.util as util
import math
import os
import functools
from reduction_gui.reduction.sans.hfir_options_script import ReductionOptions
from reduction_gui.settings.application_settings import GeneralSettings
from reduction_gui.widgets.base_widget import BaseWidget
import ui.sans.ui_hfir_instrument

class SANSInstrumentWidget(BaseWidget):    
    """
        Widget that present instrument details to the user
    """
    ## Widget name
    name = "Reduction Options"      
    
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
        if key == "sample_detector_distance" and not self._summary.sample_dist_chk.isChecked():
            self._summary.sample_dist_edit.setText(QtCore.QString(str(value)))
            util._check_and_get_float_line_edit(self._summary.sample_dist_edit, min=0.0)
        elif key == "wavelength" and not self._summary.wavelength_chk.isChecked():
            self._summary.wavelength_edit.setText(QtCore.QString(str(value)))
            util._check_and_get_float_line_edit(self._summary.wavelength_edit, min=0.0)
        elif key == "wavelength_spread" and not self._summary.wavelength_chk.isChecked():
            self._summary.wavelength_spread_edit.setText(QtCore.QString(str(value)))
        elif key == "sample_thickness"  and not self._summary.thickness_chk.isChecked():
            self._summary.thickness_edit.setText(QtCore.QString(str(value)))
            util._check_and_get_float_line_edit(self._summary.thickness_edit, min=0.0)

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
        self._summary.thickness_edit.setValidator(QtGui.QDoubleValidator(self._summary.thickness_edit))
        
        # Event connections
        self.connect(self._summary.detector_offset_chk, QtCore.SIGNAL("clicked(bool)"), self._det_offset_clicked)
        self.connect(self._summary.sample_dist_chk, QtCore.SIGNAL("clicked(bool)"), self._sample_dist_clicked)
        self.connect(self._summary.wavelength_chk, QtCore.SIGNAL("clicked(bool)"), self._wavelength_clicked)
    
        self.connect(self._summary.dark_current_check, QtCore.SIGNAL("clicked(bool)"), self._dark_clicked)
        self.connect(self._summary.dark_browse_button, QtCore.SIGNAL("clicked()"), self._dark_browse)
        self.connect(self._summary.dark_plot_button, QtCore.SIGNAL("clicked()"),
                     functools.partial(self.show_instrument, file_name=self._summary.dark_file_edit.text))

        # Q range
        self._summary.n_q_bins_edit.setText(QtCore.QString("100"))
        self._summary.n_sub_pix_edit.setText(QtCore.QString("1"))
            
        self._summary.scale_edit.setText(QtCore.QString("1"))
            
        self._summary.instr_name_label.hide()    
        self._dark_clicked(self._summary.dark_current_check.isChecked())  
        
        # Mask Validators
        self._summary.x_min_edit.setValidator(QtGui.QIntValidator(self._summary.x_min_edit))
        self._summary.x_max_edit.setValidator(QtGui.QIntValidator(self._summary.x_max_edit))
        self._summary.y_min_edit.setValidator(QtGui.QIntValidator(self._summary.y_min_edit))
        self._summary.y_max_edit.setValidator(QtGui.QIntValidator(self._summary.y_max_edit))
        
        self._summary.top_edit.setValidator(QtGui.QIntValidator(self._summary.top_edit))
        self._summary.bottom_edit.setValidator(QtGui.QIntValidator(self._summary.bottom_edit))
        self._summary.left_edit.setValidator(QtGui.QIntValidator(self._summary.left_edit))
        self._summary.right_edit.setValidator(QtGui.QIntValidator(self._summary.right_edit))
        
        # Mask Connections
        self.connect(self._summary.add_rectangle_button, QtCore.SIGNAL("clicked()"), self._add_rectangle)
        self.connect(self._summary.remove_button, QtCore.SIGNAL("clicked()"), self._remove_rectangle)
  
        # Absolute scale connections and validators
        self._summary.scale_edit.setValidator(QtGui.QDoubleValidator(self._summary.scale_edit))
        self._summary.scale_beam_radius_edit.setValidator(QtGui.QDoubleValidator(self._summary.scale_beam_radius_edit))
        self._summary.scale_att_trans_edit.setValidator(QtGui.QDoubleValidator(self._summary.scale_att_trans_edit))
        self.connect(self._summary.scale_data_browse_button, QtCore.SIGNAL("clicked()"), self._scale_data_browse)
        self.connect(self._summary.scale_data_plot_button, QtCore.SIGNAL("clicked()"),
                     functools.partial(self.show_instrument, file_name=self._summary.scale_data_edit.text))
        self.connect(self._summary.scale_chk, QtCore.SIGNAL("clicked(bool)"), self._scale_clicked)
        self._scale_clicked(self._summary.scale_chk.isChecked())
        
        if not self._in_mantidplot:
            self._summary.dark_plot_button.hide()
            self._summary.scale_data_plot_button.hide()
            
    def _scale_clicked(self, is_checked):
        self._summary.direct_beam_label.setEnabled(is_checked)
        self._summary.att_trans_label.setEnabled(is_checked)
        self._summary.beamstop_label.setEnabled(is_checked)
        self._summary.scale_data_edit.setEnabled(is_checked)
        self._summary.scale_data_plot_button.setEnabled(is_checked)
        self._summary.scale_data_browse_button.setEnabled(is_checked)
        self._summary.scale_att_trans_edit.setEnabled(is_checked)
        self._summary.scale_beam_radius_edit.setEnabled(is_checked)
        self._summary.thickness_edit.setEnabled(is_checked)
        self._summary.thickness_chk.setEnabled(is_checked)
        
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

    def _sample_dist_clicked(self, is_checked):
        self._summary.sample_dist_edit.setEnabled(is_checked)
        
        if is_checked:
            self._summary.detector_offset_chk.setChecked(not is_checked)
            self._summary.detector_offset_edit.setEnabled(not is_checked)

    def _wavelength_clicked(self, is_checked):
        self._summary.wavelength_edit.setEnabled(is_checked)
        self._summary.wavelength_spread_edit.setEnabled(is_checked)

    def _dark_clicked(self, is_checked):
        self._summary.dark_file_edit.setEnabled(is_checked)
        self._summary.dark_browse_button.setEnabled(is_checked)
        self._summary.dark_plot_button.setEnabled(is_checked)
        
    def _dark_browse(self):
        fname = self.data_browse_dialog()
        if fname:
            self._summary.dark_file_edit.setText(fname)      

    def _add_rectangle(self):
        # Read in the parameters
        x_min = util._check_and_get_int_line_edit(self._summary.x_min_edit)
        x_max = util._check_and_get_int_line_edit(self._summary.x_max_edit)
        y_min = util._check_and_get_int_line_edit(self._summary.y_min_edit)
        y_max = util._check_and_get_int_line_edit(self._summary.y_max_edit)
        
        # Check that a rectangle was defined. We don't care whether 
        # the min/max values were inverted
        if (self._summary.x_min_edit.hasAcceptableInput() and
            self._summary.x_max_edit.hasAcceptableInput() and
            self._summary.y_min_edit.hasAcceptableInput() and
            self._summary.y_max_edit.hasAcceptableInput()):
            rect = ReductionOptions.RectangleMask(x_min, x_max, y_min, y_max)
            self._append_rectangle(rect)
    
    def _remove_rectangle(self):
        selected = self._summary.listWidget.selectedItems()
        for item in selected:
            self._summary.listWidget.takeItem( self._summary.listWidget.row(item) )
    
    def _append_rectangle(self, rect):
        class _ItemWrapper(QtGui.QListWidgetItem):
            def __init__(self, value):
                QtGui.QListWidgetItem.__init__(self, value)
                self.value = rect
        self._summary.listWidget.addItem(_ItemWrapper("Rect: %g < x < %g; %g < y < %g" % (rect.x_min, rect.x_max, rect.y_min, rect.y_max)))    

    def set_state(self, state):
        """
            Populate the UI elements with the data from the given state.
            @param state: InstrumentDescription object
        """
        self._summary.instr_name_label.setText(QtCore.QString(state.instrument_name))
        #npixels = "%d x %d" % (state.nx_pixels, state.ny_pixels)
        #self._summary.n_pixel_label.setText(QtCore.QString(npixels))
        #self._summary.pixel_size_label.setText(QtCore.QString(str(state.pixel_size)))
       
        # Absolute scaling
        self._summary.scale_chk.setChecked(state.calculate_scale)
        self._summary.scale_edit.setText(QtCore.QString(str(state.scaling_factor)))
        self._summary.scale_data_edit.setText(QtCore.QString(state.scaling_direct_file))
        self._summary.scale_att_trans_edit.setText(QtCore.QString(str(state.scaling_att_trans)))
        self._summary.scale_beam_radius_edit.setText(QtCore.QString(str(state.scaling_beam_diam)))
        self._summary.thickness_edit.setText(QtCore.QString(str(state.sample_thickness)))
        util._check_and_get_float_line_edit(self._summary.thickness_edit, min=0.0)
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
        # Sample-detector distance takes precedence over offset if both are non-zero
        self._sample_dist_clicked(self._summary.sample_dist_chk.isChecked())

        # Wavelength value
        self._prepare_field(state.wavelength != 0, 
                            state.wavelength, 
                            self._summary.wavelength_chk, 
                            self._summary.wavelength_edit,
                            state.wavelength_spread,
                            self._summary.wavelength_spread_edit)
        
        # Solid angle correction flag
        self._summary.solid_angle_chk.setChecked(state.solid_angle_corr)
        
        # Dark current
        self._summary.dark_current_check.setChecked(state.dark_current_corr)
        self._summary.dark_file_edit.setText(QtCore.QString(state.dark_current_data))
        self._dark_clicked(self._summary.dark_current_check.isChecked())  
        
        # Normalization
        if state.normalization == state.NORMALIZATION_NONE:
            self._summary.normalization_none_radio.setChecked(True)
        elif state.normalization == state.NORMALIZATION_TIME:
            self._summary.normalization_time_radio.setChecked(True)
        elif state.normalization == state.NORMALIZATION_MONITOR:
            self._summary.normalization_monitor_radio.setChecked(True)
        
        # Q range
        self._summary.n_q_bins_edit.setText(QtCore.QString(str(state.n_q_bins)))
        self._summary.n_sub_pix_edit.setText(QtCore.QString(str(state.n_sub_pix)))
        self._summary.log_binning_radio.setChecked(state.log_binning)
        
        self._summary.top_edit.setText(QtCore.QString(str(state.top)))
        self._summary.bottom_edit.setText(QtCore.QString(str(state.bottom)))
        self._summary.left_edit.setText(QtCore.QString(str(state.left)))
        self._summary.right_edit.setText(QtCore.QString(str(state.right)))
            
        self._summary.listWidget.clear()
        for item in state.shapes:
            self._append_rectangle(item)
            
        self._masked_detectors = state.detector_ids

    def _prepare_field(self, is_enabled, stored_value, chk_widget, edit_widget, suppl_value=None, suppl_edit=None):
        #to_display = str(stored_value) if is_enabled else ''
        edit_widget.setEnabled(is_enabled)
        chk_widget.setChecked(is_enabled)
        edit_widget.setText(QtCore.QString(str(stored_value)))
        if suppl_value is not None and suppl_edit is not None:
            suppl_edit.setEnabled(is_enabled)
            suppl_edit.setText(QtCore.QString(str(suppl_value)))

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
        m.scaling_beam_diam = util._check_and_get_float_line_edit(self._summary.scale_beam_radius_edit)
        m.sample_thickness = util._check_and_get_float_line_edit(self._summary.thickness_edit, min=0.0)
        
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
        
        # Mask Edges
        m.top = util._check_and_get_int_line_edit(self._summary.top_edit)
        m.bottom = util._check_and_get_int_line_edit(self._summary.bottom_edit)
        m.left = util._check_and_get_int_line_edit(self._summary.left_edit)
        m.right = util._check_and_get_int_line_edit(self._summary.right_edit)
        
        # Mask Rectangles
        for i in range(self._summary.listWidget.count()):
            m.shapes.append(self._summary.listWidget.item(i).value)
        
        # Mask detector IDs
        m.detector_ids = self._masked_detectors

        return m
