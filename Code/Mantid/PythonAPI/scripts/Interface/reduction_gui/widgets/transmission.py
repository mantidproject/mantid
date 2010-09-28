from PyQt4 import QtGui, uic, QtCore
import util
import os
import functools
from reduction_gui.reduction.hfir_reduction_steps import Transmission
from reduction_gui.settings.application_settings import GeneralSettings
from base_widget import BaseWidget

class DirectBeam(BaseWidget):
    """
        Widget for the direct beam transmission calculation options.
    """
    
    def __init__(self, parent=None, state=None, settings=None):
        QtGui.QWidget.__init__(self, parent)
        
        self._layout = QtGui.QHBoxLayout()
        self._content = QtGui.QGroupBox(self)
        self._layout.addWidget(self._content)

        self.setLayout(self._layout)
        
        # General GUI settings
        if settings is None:
            settings = GeneralSettings()
        self._settings = settings  
              
        f = QtCore.QFile(":/trans_direct_beam.ui")
        f.open(QtCore.QIODevice.ReadOnly)
        uic.loadUi(f, self._content)
        self.initialize_content()
        
        if state is not None:
            self.set_state(state)
        else:
            self.set_state(Transmission.DirectBeam())      
  
    def initialize_content(self):
        """
            Declare the validators and event connections for the 
            widgets loaded through the .ui file.
        """
        # Validators
        self._content.beam_radius_edit.setValidator(QtGui.QDoubleValidator(self._content.beam_radius_edit))
        
        # Connections
        self.connect(self._content.sample_browse, QtCore.SIGNAL("clicked()"), self._sample_browse)
        self.connect(self._content.direct_browse, QtCore.SIGNAL("clicked()"), self._direct_browse)

    def set_state(self, state):
        """
            Populate the UI elements with the data from the given state.
            @param state: Transmission object
        """
        self._content.beam_radius_edit.setText(QtCore.QString(str(state.beam_radius)))
        self._content.sample_edit.setText(QtCore.QString(state.sample_file))
        self._content.direct_edit.setText(QtCore.QString(state.direct_beam))

    def get_state(self):
        """
            Returns an object with the state of the interface
        """
        m = Transmission.DirectBeam()
        m.beam_radius = util._check_and_get_float_line_edit(self._content.beam_radius_edit)
        m.sample_file = self._content.sample_edit.text()
        m.direct_beam = self._content.direct_edit.text()
        return m
    
    def _sample_browse(self):
        fname = self.data_browse_dialog()
        if fname:
            self._content.sample_edit.setText(fname)      
        
    def _direct_browse(self):
        fname = self.data_browse_dialog()
        if fname:
            self._content.direct_edit.setText(fname)      
    
class BeamSpreader(BaseWidget):
    """
        Widget for the beam spreader transmission calculation options.
    """
    
    def __init__(self, parent=None, state=None, settings=None):
        QtGui.QWidget.__init__(self, parent)
        
        self._layout = QtGui.QHBoxLayout()
        self._content = QtGui.QGroupBox(self)
        self._layout.addWidget(self._content)

        self.setLayout(self._layout)
        
        # General GUI settings
        if settings is None:
            settings = GeneralSettings()
        self._settings = settings  
              
        f = QtCore.QFile(":/trans_spreader.ui")
        f.open(QtCore.QIODevice.ReadOnly)
        uic.loadUi(f, self._content)
        self.initialize_content()
        
        if state is not None:
            self.set_state(state)
        else:
            self.set_state(Transmission.BeamSpreader())      
  
    def initialize_content(self):
        """
            Declare the validators and event connections for the 
            widgets loaded through the .ui file.
        """
        # Validators
        self._content.spreader_trans_edit.setValidator(QtGui.QDoubleValidator(self._content.spreader_trans_edit))
        self._content.spreader_trans_spread_edit.setValidator(QtGui.QDoubleValidator(self._content.spreader_trans_spread_edit))
        
        # Connections
        self.connect(self._content.sample_scatt_browse, QtCore.SIGNAL("clicked()"), self._sample_scatt_browse)
        self.connect(self._content.sample_spread_browse, QtCore.SIGNAL("clicked()"), self._sample_spread_browse)
        self.connect(self._content.direct_scatt_browse, QtCore.SIGNAL("clicked()"), self._direct_scatt_browse)
        self.connect(self._content.direct_spread_browse, QtCore.SIGNAL("clicked()"), self._direct_spread_browse)

    def set_state(self, state):
        """
            Populate the UI elements with the data from the given state.
            @param state: Transmission object
        """
        self._content.spreader_trans_edit.setText(QtCore.QString(str(state.spreader_trans)))
        self._content.spreader_trans_spread_edit.setText(QtCore.QString(str(state.spreader_trans_spread)))
        self._content.sample_scatt_edit.setText(QtCore.QString(state.sample_scatt))
        self._content.sample_spread_edit.setText(QtCore.QString(state.sample_spreader))
        self._content.direct_scatt_edit.setText(QtCore.QString(state.direct_scatt))
        self._content.direct_spread_edit.setText(QtCore.QString(state.direct_spreader))
    
    def get_state(self):
        """
            Returns an object with the state of the interface
        """
        m = Transmission.BeamSpreader()
        m.spreader_trans = util._check_and_get_float_line_edit(self._content.spreader_trans_edit)
        m.spreader_trans_spread = util._check_and_get_float_line_edit(self._content.spreader_trans_spread_edit)
        m.sample_scatt = self._content.sample_scatt_edit.text()
        m.direct_scatt = self._content.direct_scatt_edit.text()
        m.sample_spreader = self._content.sample_spread_edit.text()
        m.direct_spreader = self._content.direct_spread_edit.text()
        return m    
    
    def _sample_scatt_browse(self):
        fname = self.data_browse_dialog()
        if fname:
            self._content.sample_scatt_edit.setText(fname)      
        
    def _direct_scatt_browse(self):
        fname = self.data_browse_dialog()
        if fname:
            self._content.direct_scatt_edit.setText(fname)      
            
    def _sample_spread_browse(self):
        fname = self.data_browse_dialog()
        if fname:
            self._content.sample_spread_edit.setText(fname)      
        
    def _direct_spread_browse(self):
        fname = self.data_browse_dialog()
        if fname:
            self._content.direct_spread_edit.setText(fname)      
    
class TransmissionWidget(BaseWidget):    
    """
        Widget that presents the transmission options to the user
    """
    _method_box = None
    
    def __init__(self, parent=None, state=None, settings=None):
        BaseWidget.__init__(self, parent=parent, state=state, settings=settings)

        f = QtCore.QFile(":/hfir_transmission.ui")
        f.open(QtCore.QIODevice.ReadOnly)
        uic.loadUi(f, self._content)
        self.initialize_content()
        
        if state is not None:
            self.set_state(state)
        else:
            self.set_state(Transmission())
            
        self._last_direct_state = None
        self._last_spreader_state = None

    
    def initialize_content(self):
        """
            Declare the validators and event connections for the 
            widgets loaded through the .ui file.
        """
        # Validators
        self._content.transmission_edit.setValidator(QtGui.QDoubleValidator(self._content.transmission_edit))
        self._content.dtransmission_edit.setValidator(QtGui.QDoubleValidator(self._content.dtransmission_edit))
        
        # Connections
        self.connect(self._content.calculate_chk, QtCore.SIGNAL("clicked(bool)"), self._calculate_clicked)
        self.connect(self._content.direct_beam_chk, QtCore.SIGNAL("clicked()"), self._direct_beam)
        self.connect(self._content.beam_spreader_chk, QtCore.SIGNAL("clicked()"), self._beam_spreader)
        
    def set_state(self, state):
        """
            Populate the UI elements with the data from the given state.
            @param state: Transmission object
        """
        self._content.transmission_edit.setText(QtCore.QString(str(state.transmission)))
        self._content.dtransmission_edit.setText(QtCore.QString(str(state.transmission_spread)))
        
        if isinstance(state.calculation_method, state.DirectBeam):
            self._content.direct_beam_chk.setChecked(True)
            self._direct_beam(state=state.calculation_method)
        else:
            self._content.beam_spreader_chk.setChecked(True)
            self._beam_spreader(state=state.calculation_method)

        self._content.calculate_chk.setChecked(state.calculate_transmission)
        self._calculate_clicked(state.calculate_transmission)

    def get_state(self):
        """
            Returns an object with the state of the interface
        """
        m = Transmission()

        m.transmission = util._check_and_get_float_line_edit(self._content.transmission_edit)
        m.transmission_spread = util._check_and_get_float_line_edit(self._content.dtransmission_edit)
        
        m.calculate_transmission = self._content.calculate_chk.isChecked()
        
        m.calculation_method=self._method_box.get_state()           

        return m
            
    def _direct_beam(self, state=None):
        if state is None:
            state = self._last_direct_state
        if isinstance(self._method_box, BeamSpreader):
            self._last_spreader_state = self._method_box.get_state()
        self._replace_method(DirectBeam(self, state=state, settings=self._settings))
        
    def _beam_spreader(self, state=None):
        if state is None:
            state = self._last_spreader_state
        if isinstance(self._method_box, DirectBeam):
            self._last_direct_state = self._method_box.get_state()
        self._replace_method(BeamSpreader(self, state=state, settings=self._settings))
        
    def _replace_method(self, widget):
        if self._method_box is not None:
            for i in range(0, self._content.widget_placeholder.count()):
                item = self._content.widget_placeholder.itemAt(i)
                self._content.widget_placeholder.removeItem(self._content.widget_placeholder.itemAt(i))
                item.widget().deleteLater()
        self._method_box = widget
        self._content.widget_placeholder.addWidget(self._method_box)
        
    def _calculate_clicked(self, is_checked):
        self._content.direct_beam_chk.setEnabled(is_checked)
        self._content.beam_spreader_chk.setEnabled(is_checked)
        if self._method_box is not None:
            self._method_box.setEnabled(is_checked)
            
        self._content.transmission_edit.setEnabled(not is_checked)
        self._content.dtransmission_edit.setEnabled(not is_checked)
        