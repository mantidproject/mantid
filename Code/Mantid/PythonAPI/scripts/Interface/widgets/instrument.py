from PyQt4 import QtGui, uic, QtCore
import util
import os
from reduction.hfir_reduction_steps import InstrumentDescription
from application_settings import GeneralSettings

class SANSInstrumentWidget(QtGui.QWidget):    
    """
        Widget that present instrument details to the user
    """
    
    def __init__(self, parent=None, state=None, ui_path='ui', settings=None):
        QtGui.QWidget.__init__(self, parent)
        
        self._layout = QtGui.QHBoxLayout()

        self._summary = QtGui.QFrame(self)
        uic.loadUi(os.path.join(ui_path, "hfir_summary.ui"), self._summary)
        self.initialize_content()
        self._layout.addWidget(self._summary)

        self.setLayout(self._layout)
        
        if state is not None:
            self.set_state(state)
        else:
            self.set_state(InstrumentDescription())
        
        # General GUI settings
        if settings is None:
            settings = GeneralSettings()
        self._settings = settings

    def content(self):
        return self._summary

    def initialize_content(self):
        # Validators
        self._summary.detector_offset_edit.setValidator(QtGui.QDoubleValidator(self._summary.detector_offset_edit))
        self._summary.sample_dist_edit.setValidator(QtGui.QDoubleValidator(self._summary.sample_dist_edit))
        self._summary.wavelength_edit.setValidator(QtGui.QDoubleValidator(self._summary.wavelength_edit))
        self._summary.min_sensitivity_edit.setValidator(QtGui.QDoubleValidator(self._summary.min_sensitivity_edit))
        self._summary.max_sensitivity_edit.setValidator(QtGui.QDoubleValidator(self._summary.max_sensitivity_edit))
        
        # Event connections
        self.connect(self._summary.detector_offset_chk, QtCore.SIGNAL("clicked(bool)"), self._det_offset_clicked)
        self.connect(self._summary.sample_dist_chk, QtCore.SIGNAL("clicked(bool)"), self._sample_dist_clicked)
        self.connect(self._summary.wavelength_chk, QtCore.SIGNAL("clicked(bool)"), self._wavelength_clicked)
    
        self.connect(self._summary.sensitivity_chk, QtCore.SIGNAL("clicked(bool)"), self._sensitivity_clicked)
        self.connect(self._summary.sensitivity_browse_button, QtCore.SIGNAL("clicked()"), self._sensitivity_browse)
            
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
        
    def _sensitivity_clicked(self, is_checked):
        self._summary.sensitivity_file_edit.setEnabled(is_checked)
        self._summary.sensitivity_browse_button.setEnabled(is_checked)
        self._summary.min_sensitivity_edit.setEnabled(is_checked)
        self._summary.max_sensitivity_edit.setEnabled(is_checked)
        
    def _sensitivity_browse(self):
        fname = unicode(QtGui.QFileDialog.getOpenFileName(self, "Data file - Choose a data file",
                                                          self._settings.data_path, 
                                                          "Data files (*.xml)"))
        
        if fname:
            # Store the location of the loaded file
            (folder, file_name) = os.path.split(fname)
            self._settings.data_path = folder
            self._summary.sensitivity_file_edit.setText(fname)      
            
    def set_state(self, state):
        """
            Populate the UI elements with the data from the given state.
            @param state: InstrumentDescription object
        """
        self._summary.instr_name_label.setText(QtCore.QString(state.instrument_name))
        npixels = "%d x %d" % (state.nx_pixels, state.ny_pixels)
        self._summary.n_pixel_label.setText(QtCore.QString(npixels))
        self._summary.pixel_size_label.setText(QtCore.QString(str(state.pixel_size)))

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

        # Wavelength value
        self._prepare_field(state.wavelength != 0, 
                            state.wavelength, 
                            self._summary.wavelength_chk, 
                            self._summary.wavelength_edit)
        
        # Solid angle correction flag
        self._summary.solid_angle_chk.setChecked(state.solid_angle_corr)
        
        # Sensitivity correction
        self._summary.sensitivity_file_edit.setText(QtCore.QString(state.sensitivity_data))
        self._summary.sensitivity_chk.setChecked(state.sensitivity_corr)
        self._sensitivity_clicked(state.sensitivity_corr)
        self._summary.min_sensitivity_edit.setText(QtCore.QString(str(state.min_sensitivity)))
        self._summary.max_sensitivity_edit.setText(QtCore.QString(str(state.max_sensitivity)))
        
        # Normalization
        if state.normalization == state.NORMALIZATION_NONE:
            self._summary.normalization_none_radio.setChecked(True)
        elif state.normalization == state.NORMALIZATION_TIME:
            self._summary.normalization_time_radio.setChecked(True)
        elif state.normalization == state.NORMALIZATION_MONITOR:
            self._summary.normalization_monitor_radio.setChecked(True)
        

    def _prepare_field(self, is_enabled, stored_value, chk_widget, edit_widget):
        #to_display = str(stored_value) if is_enabled else ''
        edit_widget.setEnabled(is_enabled)
        chk_widget.setChecked(is_enabled)
        edit_widget.setText(QtCore.QString(str(stored_value)))

    def get_state(self):
        """
            Returns an object with the state of the interface
        """
        m = InstrumentDescription()
        
        # Detector offset input
        if self._summary.detector_offset_chk.isChecked():
            m.detector_offset = util._check_and_get_float_line_edit(self._summary.detector_offset_edit)
        else:
            m.detector_offset = 0
            
        # Sample-detector distance
        if self._summary.sample_dist_chk.isChecked():
            m.sample_detector_distance = util._check_and_get_float_line_edit(self._summary.sample_dist_edit)
        else:
            m.sample_detector_distance = 0
            
        # Wavelength value
        if self._summary.wavelength_chk.isChecked():
            m.wavelength = util._check_and_get_float_line_edit(self._summary.wavelength_edit)
        else:
            m.wavelength = 0
            
        # Solid angle correction
        m.solid_angle_corr = self._summary.solid_angle_chk.isChecked()
        
        # Sensitivity correction
        m.sensitivity_corr = self._summary.sensitivity_chk.isChecked() 
        m.sensitivity_data = self._summary.sensitivity_file_edit.text()
        m.min_sensitivity = util._check_and_get_float_line_edit(self._summary.min_sensitivity_edit)
        m.max_sensitivity = util._check_and_get_float_line_edit(self._summary.max_sensitivity_edit)
        
        # Normalization
        if self._summary.normalization_none_radio.isChecked():
            m.normalization = m.NORMALIZATION_NONE
        elif self._summary.normalization_time_radio.isChecked():
            m.normalization = m.NORMALIZATION_TIME
        elif self._summary.normalization_monitor_radio.isChecked():
            m.normalization = m.NORMALIZATION_MONITOR
        
        return m