from PyQt4 import QtGui, uic, QtCore
import reduction_gui.widgets.util as util
import math
import os
from reduction_gui.reduction.sans.hfir_options_script import ReductionOptions
from reduction_gui.settings.application_settings import GeneralSettings
from reduction_gui.reduction.mantid_util import DataFileProxy
from reduction_gui.widgets.base_widget import BaseWidget
import ui.sans.ui_hfir_instrument

class SANSInstrumentWidget(BaseWidget):    
    """
        Widget that present instrument details to the user
    """
    ## Widget name
    name = "Reduction Options"      
    
    def __init__(self, parent=None, state=None, settings=None, name="BIOSANS"):      
        super(SANSInstrumentWidget, self).__init__(parent, state, settings) 

        class SummaryFrame(QtGui.QFrame, ui.sans.ui_hfir_instrument.Ui_Frame): 
            def __init__(self, parent=None):
                QtGui.QFrame.__init__(self, parent)
                self.setupUi(self)
                
        self._summary = SummaryFrame(self)
        self.initialize_content()
        self._layout.addWidget(self._summary)
        
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

    def content(self):
        return self._summary

    def initialize_content(self):
        # Validators
        self._summary.scale_edit.setValidator(QtGui.QDoubleValidator(self._summary.scale_edit))
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

        # Q range
        self._summary.n_q_bins_edit.setText(QtCore.QString("100"))
        self._summary.n_sub_pix_edit.setText(QtCore.QString("1"))
            
        self._summary.scale_edit.setText(QtCore.QString("1"))
            
        self._summary.instr_name_label.hide()    
        self._dark_clicked(self._summary.dark_current_check.isChecked())    
            
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

    def set_state(self, state):
        """
            Populate the UI elements with the data from the given state.
            @param state: InstrumentDescription object
        """
        self._summary.instr_name_label.setText(QtCore.QString(state.instrument_name))
        #npixels = "%d x %d" % (state.nx_pixels, state.ny_pixels)
        #self._summary.n_pixel_label.setText(QtCore.QString(npixels))
        #self._summary.pixel_size_label.setText(QtCore.QString(str(state.pixel_size)))

        self._summary.scale_edit.setText(QtCore.QString(str(state.scaling_factor)))

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
        
        m.scaling_factor = util._check_and_get_float_line_edit(self._summary.scale_edit)
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
        
        return m
    
    def get_data_info(self):
        """
            Retrieve information from the data file and update the display
        """
        if not self._summary.sample_dist_chk.isChecked() or not self._summary.wavelength_chk.isChecked():
            flist_str = unicode(self._summary.data_file_edit.text())
            flist_str = flist_str.replace(',', ';')
            data_files = flist_str.split(';')
            if len(data_files)<1:
                return
            fname = data_files[0]
            if len(str(fname).strip())>0:
                dataproxy = DataFileProxy(fname)
                if len(dataproxy.errors)>0:
                    QtGui.QMessageBox.warning(self, "Error", dataproxy.errors[0])
                    return
                
                self._settings.last_data_ws = dataproxy.data_ws
                if dataproxy.sample_detector_distance is not None and not self._summary.sample_dist_chk.isChecked():
                    self._summary.sample_dist_edit.setText(QtCore.QString(str(dataproxy.sample_detector_distance)))
                    util._check_and_get_float_line_edit(self._summary.sample_dist_edit, min=0.0)
                if not self._summary.wavelength_chk.isChecked():
                    if dataproxy.wavelength is not None:
                        self._summary.wavelength_edit.setText(QtCore.QString(str(dataproxy.wavelength)))
                        util._check_and_get_float_line_edit(self._summary.wavelength_edit, min=0.0)
                    if dataproxy.wavelength_spread is not None:
                        self._summary.wavelength_spread_edit.setText(QtCore.QString(str(dataproxy.wavelength_spread)))
                    if len(dataproxy.errors)>0:
                        print dataproxy.errors
                if dataproxy.data is not None:
                    #self._mask_widget.set_background_data(dataproxy.data)
                    self._summary.repaint()  

            
        
