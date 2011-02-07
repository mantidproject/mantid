from PyQt4 import QtGui, uic, QtCore
import util
import math
import os
from reduction_gui.reduction.sns_script_elements import InstrumentDescription
from reduction_gui.reduction.sns_script_elements import DataFileProxy
from reduction_gui.settings.application_settings import GeneralSettings
from base_widget import BaseWidget
import ui.ui_sns_data_summary

class SNSInstrumentWidget(BaseWidget):    
    """
        Widget that present instrument details to the user
    """
    ## Widget name
    name = "Instrument"      
    
    def __init__(self, parent=None, state=None, settings=None, data_type=None):      
        super(SNSInstrumentWidget, self).__init__(parent, state, settings, data_type) 

        class SummaryFrame(QtGui.QFrame, ui.ui_sns_data_summary.Ui_Frame): 
            def __init__(self, parent=None):
                QtGui.QFrame.__init__(self, parent)
                self.setupUi(self)
                
        self._summary = SummaryFrame(self)
        self.initialize_content()
        self._layout.addWidget(self._summary)

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
        self._summary.min_sensitivity_edit.setValidator(QtGui.QDoubleValidator(self._summary.min_sensitivity_edit))
        self._summary.max_sensitivity_edit.setValidator(QtGui.QDoubleValidator(self._summary.max_sensitivity_edit))
        self._summary.n_q_bins_edit.setValidator(QtGui.QIntValidator(self._summary.n_q_bins_edit))
        self._summary.n_sub_pix_edit.setValidator(QtGui.QIntValidator(self._summary.n_sub_pix_edit))
        self._summary.transmission_edit.setValidator(QtGui.QIntValidator(self._summary.transmission_edit))
        self._summary.transmission_error_edit.setValidator(QtGui.QIntValidator(self._summary.transmission_error_edit))
        
        self.connect(self._summary.sensitivity_chk, QtCore.SIGNAL("clicked(bool)"), self._sensitivity_clicked)
        self.connect(self._summary.sensitivity_browse_button, QtCore.SIGNAL("clicked()"), self._sensitivity_browse)

        self.connect(self._summary.transmission_chk, QtCore.SIGNAL("clicked(bool)"), self._transmission_clicked)

        # Data file
        self.connect(self._summary.data_file_browse_button, QtCore.SIGNAL("clicked()"), self._data_browse)
        
        # Q range
        self._summary.n_q_bins_edit.setText(QtCore.QString("100"))
        self._summary.n_sub_pix_edit.setText(QtCore.QString("1"))
        
        self._summary.sample_dist_edit.setEnabled(False)
            
    def _sensitivity_clicked(self, is_checked):
        self._summary.sensitivity_file_edit.setEnabled(is_checked)
        self._summary.sensitivity_browse_button.setEnabled(is_checked)
        self._summary.min_sensitivity_edit.setEnabled(is_checked)
        self._summary.max_sensitivity_edit.setEnabled(is_checked)
        self._summary.sensivity_file_label.setEnabled(is_checked)
        self._summary.sensitivity_range_label.setEnabled(is_checked)
        self._summary.sensitivity_min_label.setEnabled(is_checked)
        self._summary.sensitivity_max_label.setEnabled(is_checked)
    
    def _transmission_clicked(self, is_checked):
        self._summary.transmission_edit.hide()
        self._summary.transmission_error_edit.hide()
        self._summary.plus_minus_label.hide()
           
    def _sensitivity_browse(self):
        fname = self.data_browse_dialog()
        if fname:
            self._summary.sensitivity_file_edit.setText(fname)      

    def _data_browse(self):
        fname = self.data_browse_dialog()
        if fname:
            self._summary.data_file_edit.setText(fname)   
            #self.get_data_info()
            
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

        # Solid angle correction flag
        self._summary.solid_angle_chk.setChecked(state.solid_angle_corr)
        
        # Frame skipping
        self._summary.frame_skipping_chk.setChecked(state.frame_skipping)
        
        # Sensitivity correction
        self._summary.sensitivity_file_edit.setText(QtCore.QString(state.sensitivity_data))
        self._summary.sensitivity_chk.setChecked(state.sensitivity_corr)
        self._sensitivity_clicked(state.sensitivity_corr)
        self._summary.min_sensitivity_edit.setText(QtCore.QString(str(state.min_sensitivity)))
        self._summary.max_sensitivity_edit.setText(QtCore.QString(str(state.max_sensitivity)))
        
        # Normalization
        if state.normalization == state.NORMALIZATION_NONE:
            self._summary.normalization_none_radio.setChecked(True)
        elif state.normalization == state.NORMALIZATION_MONITOR:
            self._summary.normalization_monitor_radio.setChecked(True)
        
        # Transmission
        self._summary.transmission_edit.setText(QtCore.QString(str(state.transmission)))
        self._summary.transmission_error_edit.setText(QtCore.QString(str(state.transmission_error)))
        self._summary.transmission_chk.setChecked(state.calculate_transmission)
        self._transmission_clicked(state.calculate_transmission)
        
        # Q range
        self._summary.n_q_bins_edit.setText(QtCore.QString(str(state.n_q_bins)))
        self._summary.n_sub_pix_edit.setText(QtCore.QString(str(state.n_sub_pix)))
        
        # Sample-detector distance
        self._summary.sample_dist_edit.setText(QtCore.QString("%-5.0g" % state.sample_distance))
        
        # Data file
        self._summary.data_file_edit.setText(QtCore.QString(state.data_file))
        self._find_background_image(state.data_file)
        # Store the location of the loaded file
        if len(state.data_file)>0:
            (folder, file_name) = os.path.split(state.data_file)
            self._settings.data_path = folder
            #self.get_data_info()

    def get_state(self):
        """
            Returns an object with the state of the interface
        """
        m = InstrumentDescription()
        
        # Data file
        m.data_file = unicode(self._summary.data_file_edit.text())

        # Solid angle correction
        m.solid_angle_corr = self._summary.solid_angle_chk.isChecked()
        
        # Frame skipping
        m.frame_skipping = self._summary.frame_skipping_chk.isChecked()
        
        # Sensitivity correction
        m.sensitivity_corr = self._summary.sensitivity_chk.isChecked() 
        m.sensitivity_data = unicode(self._summary.sensitivity_file_edit.text())
        m.min_sensitivity = util._check_and_get_float_line_edit(self._summary.min_sensitivity_edit)
        m.max_sensitivity = util._check_and_get_float_line_edit(self._summary.max_sensitivity_edit)
        
        # Normalization
        if self._summary.normalization_none_radio.isChecked():
            m.normalization = m.NORMALIZATION_NONE
        elif self._summary.normalization_monitor_radio.isChecked():
            m.normalization = m.NORMALIZATION_MONITOR
            
        # Transmission
        m.transmission = util._check_and_get_float_line_edit(self._summary.transmission_edit)
        m.transmission_error = util._check_and_get_float_line_edit(self._summary.transmission_error_edit)
        m.calculate_transmission = self._summary.transmission_chk.isChecked()
            
        # Q range
        m.n_q_bins = util._check_and_get_int_line_edit(self._summary.n_q_bins_edit)
        m.n_sub_pix = util._check_and_get_int_line_edit(self._summary.n_sub_pix_edit)
        
        return m
    
    def get_data_info(self):
        """
            Retrieve information from the data file and update the display
        """
        fname = self._summary.data_file_edit.text()
        if len(str(fname).strip())>0:
            dataproxy = DataFileProxy(str(self._summary.data_file_edit.text()))
            if dataproxy.sample_detector_distance is not None:
                self._summary.sample_dist_edit.setText(QtCore.QString(str(dataproxy.sample_detector_distance)))
            if dataproxy.data is not None:
                #self._mask_widget.set_background_data(dataproxy.data)
                self._summary.repaint()  

            
        
