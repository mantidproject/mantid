from PyQt4 import QtGui, uic, QtCore
import util
import os
import functools
from reduction.hfir_reduction_steps import Transmission
from application_settings import GeneralSettings
from base_widget import BaseWidget

class TransmissionWidget(BaseWidget):    
    """
        Widget that presents the transmission options to the user
    """
    
    def __init__(self, parent=None, state=None, ui_path='ui', settings=None):
        BaseWidget.__init__(self, parent=parent, state=state, ui_path=ui_path, settings=settings)

        uic.loadUi(os.path.join(ui_path, "hfir_transmission.ui"), self._content)
        self.initialize_content()
        
        if state is not None:
            self.set_state(state)
        else:
            self.set_state(Transmission())
            
        self._ui_path = ui_path
        self._method_box = None

    
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
        self.connect(self._content.direct_beam_chk, QtCore.SIGNAL("clicked(bool)"), 
                     functools.partial(self._method_clicked, "trans_direct_beam.ui"))
        self.connect(self._content.beam_spreader_chk, QtCore.SIGNAL("clicked(bool)"), 
                     functools.partial(self._method_clicked, "trans_spreader.ui"))
        
        
    def _method_clicked(self, ui_file):
        if self._method_box is not None:
            for i in range(0, self._content.widget_placeholder.count()):
                item = self._content.widget_placeholder.itemAt(i)
                self._content.widget_placeholder.removeItem(self._content.widget_placeholder.itemAt(i))
                item.widget().deleteLater()
            
        self._method_box = QtGui.QGroupBox(self)
        uic.loadUi(os.path.join(self._ui_path, ui_file), self._method_box)
        self._content.widget_placeholder.addWidget(self._method_box)
                
        
    def _calculate_clicked(self, is_checked):
        self._content.direct_beam_chk.setEnabled(is_checked)
        self._content.beam_spreader_chk.setEnabled(is_checked)
        if self._method_box is not None:
            self._method_box.setEnabled(is_checked)
            
        self._content.transmission_edit.setEnabled(not is_checked)
        self._content.dtransmission_edit.setEnabled(not is_checked)
        