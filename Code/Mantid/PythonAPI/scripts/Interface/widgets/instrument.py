from PyQt4 import QtGui, uic, QtCore
import util
import os
from reduction.hfir_reduction import InstrumentDescription

class SANSInstrumentWidget(QtGui.QWidget):    
    
    def __init__(self, parent=None, state=None, ui_path='ui'):
        QtGui.QWidget.__init__(self, parent)
        
        self._layout = QtGui.QHBoxLayout()

        self._summary = QtGui.QFrame(self)
        uic.loadUi(os.path.join(ui_path, "hfir_summary.ui"), self._summary)
        self._layout.addWidget(self._summary)

        self.setLayout(self._layout)
        
        if state is not None:
            self.set_state(state)
        else:
            self.set_state(InstrumentDescription())

    def content(self):
        return self._summary
    
    def set_state(self, state):
        self._summary.instr_name_edit.setText(QtCore.QString(state.instrument_name))
        npixels = "%d x %d" % (state.nx_pixels, state.ny_pixels)
        self._summary.n_pixel_edit.setText(QtCore.QString(npixels))
        self._summary.pixel_size_edit.setText(QtCore.QString(str(state.pixel_size)))
    
    def get_state(self):
        m = InstrumentDescription()
        m.pixel_size = util._check_and_get_float_line_edit(self._summary.pixel_size_edit)
        return m