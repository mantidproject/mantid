"""
    Beam finder interface
"""
import util
import os
from PyQt4 import QtGui, uic, QtCore
from reduction.hfir_reduction_steps import BeamFinder

# Beam finder frame
class BeamFinderFrame(QtGui.QFrame):
    """
        Beam center interface.
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
        self.x_pos_edit.setText(QtCore.QString(str(state.x_position)))
        self.y_pos_edit.setText(QtCore.QString(str(state.y_position)))
        self.use_beam_finder_checkbox.setChecked(state.use_finder)
        self.groupBox.setEnabled(state.use_finder)
        self.beam_data_file_edit.setText(QtCore.QString(state.beam_file))
        self.beam_radius_edit.setText(QtCore.QString(str(state.beam_radius)))
        
        self.direct_beam.setChecked(state.use_direct_beam)
        self.beam_radius_edit.setEnabled(not state.use_direct_beam)
        
    def use_beam_finder_changed(self):
        if self.use_beam_finder_checkbox.isChecked():
            self.groupBox.setEnabled(True)
            self.x_pos_edit.setEnabled(False)
            self.y_pos_edit.setEnabled(False)            
        else:
            self.groupBox.setEnabled(False)            
            self.x_pos_edit.setEnabled(True)
            self.y_pos_edit.setEnabled(True)

    def fit_scattering_data(self):
        self.beam_radius_edit.setEnabled(True)
        
    def fit_direct_beam(self):
        self.beam_radius_edit.setEnabled(False)
            
class BeamFinderWidget(QtGui.QWidget):    
    def __init__(self, parent=None, state=None, ui_path='ui'):
        QtGui.QWidget.__init__(self, parent)
        self._layout = QtGui.QHBoxLayout()
        self._content = BeamFinderFrame(self)
        uic.loadUi(os.path.join(ui_path, "hfir_beam_finder.ui"), self._content)
        self._layout.addWidget(self._content)
        self.setLayout(self._layout)
        
        if state is not None:
            self._content.set_state(state)
        else:
            self._content.set_state(BeamFinder())
            
    def get_state(self):
        return self._content.get_state()
