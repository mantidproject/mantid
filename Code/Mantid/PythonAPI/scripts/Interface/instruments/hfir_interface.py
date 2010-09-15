"""
    This module defines the interface control for HFIR SANS.
    Each reduction method tab that needs to be presented is defined here.
    The actual view/layout is define in .ui files. The state of the reduction
    process is kept elsewhere (HFIRReduction object)
"""
from PyQt4 import QtGui, uic, QtCore
from widgets.beam_finder import BeamFinderWidget
from widgets.instrument import SANSInstrumentWidget
from reduction.hfir_reduction import ReductionScripter


class HFIRInterface(object):
    """
        Defines the widgets for HFIR reduction
    """
    ui_path = 'ui'
    
    def __init__(self, name=''):
        self.scripter = ReductionScripter(name=name)
        # Main panel with instrument description common to all data files
        self._instrument_widget = None
        # Beam finder panel
        self._beam_finder_widget = None
        
    def load_file(self, file_name):
        self.scripter.from_xml(file_name)
        self._beam_finder_widget.set_state(self.scripter.beam_finder)
        self._instrument_widget.set_state(self.scripter.instrument)
        
    def save_file(self, file_name):
        self._push_to_scripter()
        self.scripter.to_xml(file_name)
        
    def export(self, file_name):
        self._push_to_scripter()
        self.scripter.to_script(file_name)
        
    def _push_to_scripter(self):
        """
            Pass the interface data to the scripter
        """
        self.scripter.instrument = self._instrument_widget.get_state()
        self.scripter.beam_finder = self._beam_finder_widget.get_state()
        
    def reduce(self):
        """
            Pass the interface data to the scripter and reduce
        """
        self._push_to_scripter()
        
        # Print the script for now
        print self.scripter.to_script()
        
    def get_tabs(self):
        
        # Instrument description
        self._instrument_widget = SANSInstrumentWidget(state=self.scripter.instrument,
                                                       ui_path = self.ui_path)
        
        # Beam finder
        self._beam_finder_widget = BeamFinderWidget(state=self.scripter.beam_finder,
                                                    ui_path = self.ui_path)
        
        return {"Instrument": self._instrument_widget,
                "Beam Finder": self._beam_finder_widget}
        