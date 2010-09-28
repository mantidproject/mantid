"""
    Beam finder interface
"""
import util
import os
from PyQt4 import QtGui, uic, QtCore
from reduction_gui.reduction.hfir_reduction_steps import BeamFinder
from reduction_gui.settings.application_settings import GeneralSettings
from base_widget import BaseWidget

# Beam finder frame
class BeamFinderFrame(QtGui.QFrame):
    """
        Beam center interface
        TODO: move most of this class in BeamFinderWidget, along the
        lines of the SANSInstrumentWidget
    """
        
    def __init__(self, parent=None, flags=0):
        super(BeamFinderFrame, self).__init__(parent)        
    
    def get_state(self):
        """
            Returns an object with the state of the interface
        """
        m = BeamFinder()
        m.x_position = util._check_and_get_float_line_edit(self.x_pos_edit)
        m.y_position = util._check_and_get_float_line_edit(self.y_pos_edit)
        m.beam_radius = util._check_and_get_float_line_edit(self.beam_radius_edit)
        m.use_finder = self.use_beam_finder_checkbox.isChecked()
        m.beam_file = self.beam_data_file_edit.text()
        m.use_direct_beam = self.direct_beam.isChecked()    
        
        return m
        
    def set_state(self, state):
        """
            Populate the UI elements with the data from the given state.
            @param state: BeamFinder object
        """
        self.x_pos_edit.setText(QtCore.QString(str(state.x_position)))
        self.y_pos_edit.setText(QtCore.QString(str(state.y_position)))
        self.use_beam_finder_checkbox.setChecked(state.use_finder)
        self.groupBox.setEnabled(state.use_finder)
        self.beam_data_file_edit.setText(QtCore.QString(state.beam_file))
        self.beam_radius_edit.setText(QtCore.QString(str(state.beam_radius)))
        
        self.direct_beam.setChecked(state.use_direct_beam)
        self.beam_radius_edit.setEnabled(not state.use_direct_beam)
        
    def use_beam_finder_changed(self):
        """
            Call-back method for when the user toggles between using a 
            beam finder or setting the beam center by hand
        """
        if self.use_beam_finder_checkbox.isChecked():
            self.groupBox.setEnabled(True)
            self.x_pos_edit.setEnabled(False)
            self.y_pos_edit.setEnabled(False)            
        else:
            self.groupBox.setEnabled(False)            
            self.x_pos_edit.setEnabled(True)
            self.y_pos_edit.setEnabled(True)

    def fit_scattering_data(self):
        """
            Call-back method for when the user selects to fit the
            scattering data (as opposed to assume direct beam data and fit 
            the whole detector) 
        """
        self.beam_radius_edit.setEnabled(True)
        
    def fit_direct_beam(self):
        """
            Call-back method for when the user selects to fit the direct beam
            data by using the whole detector
        """
        self.beam_radius_edit.setEnabled(False)
            
class BeamFinderWidget(BaseWidget):    
    def __init__(self, parent=None, state=None, settings=None):
        QtGui.QWidget.__init__(self, parent)
        self._layout = QtGui.QHBoxLayout()
        self._content = BeamFinderFrame(self)
        f = QtCore.QFile(":/hfir_beam_finder.ui")
        f.open(QtCore.QIODevice.ReadOnly)
        uic.loadUi(f, self._content)
        self.initialize_content()
        self._layout.addWidget(self._content)
        self.setLayout(self._layout)
        
        if state is not None:
            self._content.set_state(state)
        else:
            self._content.set_state(BeamFinder())
            
        # General GUI settings
        if settings is None:
            settings = GeneralSettings()
        self._settings = settings
            
    def get_state(self):
        """
            Relay a get_state() method call to the beam finder widget
        """
        return self._content.get_state()
    
    def set_state(self, state):
        """
            Relay a set_state() method call to the beam finder widget
        """
        return self._content.set_state(state)

    def initialize_content(self):
        """
            Initialize the content that was added by loading the .ui file
        """
        # Validators
        self._content.x_pos_edit.setValidator(QtGui.QDoubleValidator(self._content.x_pos_edit))
        self._content.y_pos_edit.setValidator(QtGui.QDoubleValidator(self._content.y_pos_edit))
        self._content.beam_radius_edit.setValidator(QtGui.QDoubleValidator(self._content.beam_radius_edit))
        self.connect(self._content.data_file_browse_button, QtCore.SIGNAL("clicked()"), self._data_file_browse)
    
    def _data_file_browse(self):
        fname = self.data_browse_dialog()
        if fname:
            self._content.beam_data_file_edit.setText(fname)          
    
