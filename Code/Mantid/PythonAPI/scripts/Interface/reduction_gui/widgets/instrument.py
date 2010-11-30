from PyQt4 import QtGui, uic, QtCore
import util
import math
import os
from reduction_gui.reduction.hfir_reduction_steps import InstrumentDescription
from reduction_gui.settings.application_settings import GeneralSettings
from reduction_gui.reduction.mantid_util import DataFileProxy
from base_widget import BaseWidget
from mask import MaskWidget
import ui.ui_hfir_data_summary

class SANSInstrumentWidget(BaseWidget):    
    """
        Widget that present instrument details to the user
    """
    # Mask widget    
    _mask_widget = None   
    
    ## Widget name
    name = "Instrument"      
    
    def __init__(self, parent=None, state=None, settings=None):      
        QtGui.QWidget.__init__(self, parent)
        
        self._layout = QtGui.QHBoxLayout()

        self._summary = QtGui.QFrame(self)

        class SummaryFrame(QtGui.QFrame, ui.ui_hfir_data_summary.Ui_Frame): 
            def __init__(self, parent=None):
                QtGui.QFrame.__init__(self, parent)
                self.setupUi(self)
                
        self._summary = SummaryFrame(self)
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
        # Mask widget
        self._mask_widget = MaskWidget()
        self._summary.placeholder_layout.addWidget(self._mask_widget)
        self._mask_widget.setMinimumSize(400, 400)
        self._summary.repaint()     
        
        # Validators
        self._summary.detector_offset_edit.setValidator(QtGui.QDoubleValidator(self._summary.detector_offset_edit))
        self._summary.sample_dist_edit.setValidator(QtGui.QDoubleValidator(self._summary.sample_dist_edit))
        self._summary.wavelength_edit.setValidator(QtGui.QDoubleValidator(self._summary.wavelength_edit))
        self._summary.min_sensitivity_edit.setValidator(QtGui.QDoubleValidator(self._summary.min_sensitivity_edit))
        self._summary.max_sensitivity_edit.setValidator(QtGui.QDoubleValidator(self._summary.max_sensitivity_edit))
        self._summary.n_q_bins_edit.setValidator(QtGui.QIntValidator(self._summary.n_q_bins_edit))
        self._summary.n_sub_pix_edit.setValidator(QtGui.QIntValidator(self._summary.n_sub_pix_edit))
        
        # Event connections
        self.connect(self._summary.detector_offset_chk, QtCore.SIGNAL("clicked(bool)"), self._det_offset_clicked)
        self.connect(self._summary.sample_dist_chk, QtCore.SIGNAL("clicked(bool)"), self._sample_dist_clicked)
        self.connect(self._summary.wavelength_chk, QtCore.SIGNAL("clicked(bool)"), self._wavelength_clicked)
    
        self.connect(self._summary.sensitivity_chk, QtCore.SIGNAL("clicked(bool)"), self._sensitivity_clicked)
        self.connect(self._summary.sensitivity_browse_button, QtCore.SIGNAL("clicked()"), self._sensitivity_browse)

        # Data file
        self.connect(self._summary.data_file_browse_button, QtCore.SIGNAL("clicked()"), self._data_browse)
        
        # Q range
        self._summary.n_q_bins_edit.setText(QtCore.QString("100"))
        self._summary.n_sub_pix_edit.setText(QtCore.QString("1"))
            
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
        
    def _sensitivity_clicked(self, is_checked):
        self._summary.sensitivity_file_edit.setEnabled(is_checked)
        self._summary.sensitivity_browse_button.setEnabled(is_checked)
        self._summary.min_sensitivity_edit.setEnabled(is_checked)
        self._summary.max_sensitivity_edit.setEnabled(is_checked)
        self._summary.sensivity_file_label.setEnabled(is_checked)
        self._summary.sensitivity_range_label.setEnabled(is_checked)
        self._summary.sensitivity_min_label.setEnabled(is_checked)
        self._summary.sensitivity_max_label.setEnabled(is_checked)
        
    def _sensitivity_browse(self):
        fname = self.data_browse_dialog()
        if fname:
            self._summary.sensitivity_file_edit.setText(fname)      

    def _data_browse(self):
        fname = self.data_browse_dialog()
        if fname:
            self._summary.data_file_edit.setText(fname)   
            self.get_data_info()
            
            # Set the mask background
            self._find_background_image(fname)       
            
    def _find_background_image(self, filename):
        """
            Find a background image for a given data file.
            If one is found, set it as the background for the mask widget
            @param filename: data filename
        """
        # Assume the standard HFIR folder structure
        [data_dir, data_file_name] = os.path.split(filename)
        file_name = os.path.splitext(data_file_name)[0]
        file_name += '.png'
        image_dir = os.path.join(data_dir, "Images")
        # Check that we have an image folder
        if os.path.isdir(image_dir):
            file_name = os.path.join(image_dir, file_name)
            # Check that we have an image for the chosen data file
            if os.path.isfile(file_name):
                self._mask_widget.set_background(file_name)
                self._summary.repaint()  
            else:
                self._mask_widget.set_background(None)
                self._summary.repaint()  
            
    def set_state(self, state):
        """
            Populate the UI elements with the data from the given state.
            @param state: InstrumentDescription object
        """
        self._summary.instr_name_label.setText(QtCore.QString(state.instrument_name))
        #npixels = "%d x %d" % (state.nx_pixels, state.ny_pixels)
        #self._summary.n_pixel_label.setText(QtCore.QString(npixels))
        #self._summary.pixel_size_label.setText(QtCore.QString(str(state.pixel_size)))

        # Mask
        self._mask_widget.topSpinBox.setValue(state.mask_top)
        self._mask_widget.bottomSpinBox.setValue(state.mask_bottom)
        self._mask_widget.rightSpinBox.setValue(state.mask_right)
        self._mask_widget.leftSpinBox.setValue(state.mask_left)
        self._mask_widget.set_background_data(None)
        self._mask_widget.set_background(None)
        
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
                            self._summary.wavelength_edit,
                            state.wavelength_spread,
                            self._summary.wavelength_spread_edit)
        
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
        
        # Q range
        self._summary.n_q_bins_edit.setText(QtCore.QString(str(state.n_q_bins)))
        self._summary.n_sub_pix_edit.setText(QtCore.QString(str(state.n_sub_pix)))
        
        # Data file
        self._summary.data_file_edit.setText(QtCore.QString(state.data_file))
        self._find_background_image(state.data_file)
        # Store the location of the loaded file
        if len(state.data_file)>0:
            (folder, file_name) = os.path.split(state.data_file)
            self._settings.data_path = folder
            self.get_data_info()

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
        m = InstrumentDescription()
        
        # Data file
        m.data_file = unicode(self._summary.data_file_edit.text())

        # Mask
        m.mask_top = int(self._mask_widget.topSpinBox.value())
        m.mask_bottom = int(self._mask_widget.bottomSpinBox.value())
        m.mask_right = int(self._mask_widget.rightSpinBox.value())
        m.mask_left = int(self._mask_widget.leftSpinBox.value())
        
        # Detector offset input
        if self._summary.detector_offset_chk.isChecked():
            m.detector_offset = util._check_and_get_float_line_edit(self._summary.detector_offset_edit)
            
        # Sample-detector distance
        if self._summary.sample_dist_chk.isChecked():
            m.sample_detector_distance = util._check_and_get_float_line_edit(self._summary.sample_dist_edit)
            
        # Wavelength value
        if self._summary.wavelength_chk.isChecked():
            m.wavelength = util._check_and_get_float_line_edit(self._summary.wavelength_edit)
            m.wavelength_spread = util._check_and_get_float_line_edit(self._summary.wavelength_spread_edit)
            
        # Solid angle correction
        m.solid_angle_corr = self._summary.solid_angle_chk.isChecked()
        
        # Sensitivity correction
        m.sensitivity_corr = self._summary.sensitivity_chk.isChecked() 
        m.sensitivity_data = unicode(self._summary.sensitivity_file_edit.text())
        m.min_sensitivity = util._check_and_get_float_line_edit(self._summary.min_sensitivity_edit)
        m.max_sensitivity = util._check_and_get_float_line_edit(self._summary.max_sensitivity_edit)
        
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
        
        return m
    
    def get_data_info(self):
        """
            Retrieve information from the data file and update the display
        """
        if not self._summary.sample_dist_chk.isChecked() or not self._summary.wavelength_chk.isChecked():
            fname = self._summary.data_file_edit.text()
            if len(str(fname).strip())>0:
                dataproxy = DataFileProxy(str(self._summary.data_file_edit.text()))
                if dataproxy.sample_detector_distance is not None and not self._summary.sample_dist_chk.isChecked():
                    self._summary.sample_dist_edit.setText(QtCore.QString(str(dataproxy.sample_detector_distance)))
                if not self._summary.wavelength_chk.isChecked():
                    if dataproxy.wavelength is not None:
                        self._summary.wavelength_edit.setText(QtCore.QString(str(dataproxy.wavelength)))
                    if dataproxy.wavelength_spread is not None:
                        self._summary.wavelength_spread_edit.setText(QtCore.QString(str(dataproxy.wavelength_spread)))
                    if len(dataproxy.errors)>0:
                        print dataproxy.errors
                if dataproxy.data is not None:
                    self._mask_widget.set_background_data(dataproxy.data)
                    self._summary.repaint()  

            
        
