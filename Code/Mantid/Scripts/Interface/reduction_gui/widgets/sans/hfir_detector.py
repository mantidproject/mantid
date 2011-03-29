from PyQt4 import QtGui, uic, QtCore
import reduction_gui.widgets.util as util
import os
from reduction_gui.reduction.sans.hfir_detector_script import Detector
from reduction_gui.settings.application_settings import GeneralSettings
from reduction_gui.widgets.base_widget import BaseWidget
import ui.sans.ui_hfir_detector

class DetectorWidget(BaseWidget):    
    """
        Widget that presents the detector options to the user
    """
    _method_box = None

    ## Widget name
    name = "Detector"      
    
    def __init__(self, parent=None, state=None, settings=None, show_transmission=True, data_type=None):
        super(DetectorWidget, self).__init__(parent, state, settings, data_type) 

        class DetFrame(QtGui.QFrame, ui.sans.ui_hfir_detector.Ui_Frame): 
            def __init__(self, parent=None):
                QtGui.QFrame.__init__(self, parent)
                self.setupUi(self)
                
        self._content = DetFrame(self)
        self._layout.addWidget(self._content)
        
        self.initialize_content()
        
        if state is not None:
            self.set_state(state)
        else:
            self.set_state(Detector())
    
    def initialize_content(self):
        """
            Declare the validators and event connections for the 
            widgets loaded through the .ui file.
        """
        # Validators
        self._content.x_pos_edit.setValidator(QtGui.QDoubleValidator(self._content.x_pos_edit))
        self._content.y_pos_edit.setValidator(QtGui.QDoubleValidator(self._content.y_pos_edit))
        self._content.beam_radius_edit.setValidator(QtGui.QDoubleValidator(self._content.beam_radius_edit))
        self._content.min_sensitivity_edit.setValidator(QtGui.QDoubleValidator(self._content.min_sensitivity_edit))
        self._content.max_sensitivity_edit.setValidator(QtGui.QDoubleValidator(self._content.max_sensitivity_edit))


        self.connect(self._content.sensitivity_chk, QtCore.SIGNAL("clicked(bool)"), self._sensitivity_clicked)
        self.connect(self._content.sensitivity_browse_button, QtCore.SIGNAL("clicked()"), self._sensitivity_browse)
        self.connect(self._content.sensitivity_dark_browse_button, QtCore.SIGNAL("clicked()"), self._sensitivity_dark_browse)

        self.connect(self._content.use_beam_finder_checkbox, QtCore.SIGNAL("clicked(bool)"), self._use_beam_finder_changed)
        self.connect(self._content.scattering_data, QtCore.SIGNAL("clicked()"), self._center_method_changed)
        self.connect(self._content.direct_beam, QtCore.SIGNAL("clicked()"), self._center_method_changed)

        self.connect(self._content.use_sample_center_checkbox, QtCore.SIGNAL("clicked(bool)"), self._use_sample_center_changed)
        self.connect(self._content.scattering_data, QtCore.SIGNAL("clicked()"), self._flood_center_method_changed)
        self.connect(self._content.direct_beam, QtCore.SIGNAL("clicked()"), self._flood_center_method_changed)
        self.connect(self._content.use_beam_finder_checkbox_2, QtCore.SIGNAL("clicked(bool)"), self._flood_use_beam_finder_changed)

        self._use_beam_finder_changed(self._content.use_beam_finder_checkbox.isChecked())
        self._content.use_sample_center_checkbox.setChecked(True)
        self._use_sample_center_changed(self._content.use_sample_center_checkbox.isChecked())
        self._sensitivity_clicked(self._content.sensitivity_chk.isChecked())

    def set_state(self, state):
        """
            Populate the UI elements with the data from the given state.
            @param state: Transmission object
        """

        
        
    def get_state(self):
        """
            Returns an object with the state of the interface
        """
        m = Detector()
        

        return m
    
    def _use_sample_center_changed(self, is_checked):
        self._content.use_beam_finder_checkbox_2.setEnabled(not is_checked)
        self._flood_use_beam_finder_changed(self._content.use_beam_finder_checkbox_2.isChecked(), not is_checked)
        
    
    def _flood_use_beam_finder_changed(self, is_checked, is_flood_ctr=True):
        # Center by hand
        self._content.x_pos_edit_2.setEnabled(not is_checked and is_flood_ctr)
        self._content.y_pos_edit_2.setEnabled(not is_checked and is_flood_ctr)            
        self._content.x_pos_label_2.setEnabled(not is_checked and is_flood_ctr)
        self._content.y_pos_label_2.setEnabled(not is_checked and is_flood_ctr)
        
        # Center computed
        self._content.direct_beam_2.setEnabled(is_checked and is_flood_ctr)
        self._content.scattering_data_2.setEnabled(is_checked and is_flood_ctr)
        self._content.data_file_label_2.setEnabled(is_checked and is_flood_ctr)
        self._content.beam_radius_label_2.setEnabled(is_checked and is_flood_ctr)
        self._content.beam_data_file_edit_2.setEnabled(is_checked and is_flood_ctr)
        self._content.data_file_browse_button_2.setEnabled(is_checked and is_flood_ctr)
        self._flood_center_method_changed(is_checked and is_flood_ctr)
        
        
        
    def _center_method_changed(self, finder_checked=True):
        is_direct_beam = self._content.direct_beam.isChecked()
        self._content.beam_radius_edit.setEnabled(not is_direct_beam and finder_checked)
    
    def _flood_center_method_changed(self, finder_checked=True):
        is_direct_beam = self._content.direct_beam_2.isChecked()
        self._content.beam_radius_edit_2.setEnabled(not is_direct_beam and finder_checked)
    
    def _beam_finder_browse(self):
        pass
    
    def _use_beam_finder_changed(self, is_checked):
        """
            Call-back method for when the user toggles between using a 
            beam finder or setting the beam center by hand
        """
        # Center by hand
        self._content.x_pos_edit.setEnabled(not is_checked)
        self._content.y_pos_edit.setEnabled(not is_checked)            
        #self._content.x_pos_label.setEnabled(not is_checked)
        #self._content.y_pos_label.setEnabled(not is_checked)
        
        # Center computed
        self._content.direct_beam.setEnabled(is_checked)
        self._content.scattering_data.setEnabled(is_checked)
        self._content.data_file_label.setEnabled(is_checked)
        self._content.beam_radius_label.setEnabled(is_checked)
        self._content.beam_data_file_edit.setEnabled(is_checked)
        self._content.data_file_browse_button.setEnabled(is_checked)
        self._center_method_changed(is_checked)

    def _sensitivity_clicked(self, is_checked):
        self._content.sensitivity_file_edit.setEnabled(is_checked)
        self._content.sensitivity_browse_button.setEnabled(is_checked)
        self._content.sensitivity_dark_file_edit.setEnabled(is_checked)
        self._content.sensitivity_dark_browse_button.setEnabled(is_checked)
        self._content.min_sensitivity_edit.setEnabled(is_checked)
        self._content.max_sensitivity_edit.setEnabled(is_checked)
        self._content.sensitivity_file_label.setEnabled(is_checked)
        self._content.sensitivity_dark_file_label.setEnabled(is_checked)
        self._content.sensitivity_range_label.setEnabled(is_checked)
        self._content.sensitivity_min_label.setEnabled(is_checked)
        self._content.sensitivity_max_label.setEnabled(is_checked)
        self._content.flood_center_grpbox.setEnabled(is_checked)
        
    def _sensitivity_browse(self):
        fname = self.data_browse_dialog()
        if fname:
            self._content.sensitivity_file_edit.setText(fname)      

    def _sensitivity_dark_browse(self):
        fname = self.data_browse_dialog()
        if fname:
            self._content.sensitivity_dark_file_edit.setText(fname)      

