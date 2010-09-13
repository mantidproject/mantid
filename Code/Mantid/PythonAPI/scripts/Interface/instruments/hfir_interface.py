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
    def __init__(self, name=''):
        self.scripter = ReductionScripter(name=name)
        # Main panel with instrument description common to all data files
        self._instrument_widget = None
        # Beam finder panel
        self._beam_finder_widget = None
        
    def reduce(self):
        """
            Pass the interface data to the scripter
        """
        self.scripter.instrument = self._instrument_widget.get_state()
        self.scripter.beam_finder = self._beam_finder_widget.get_state()
        
        # Print the script for now
        print self.scripter.to_script()
        
    def get_tabs(self):
        
        # Instrument description
        self._instrument_widget = SANSInstrumentWidget(state=self.scripter.instrument)
        
        # Beam finder
        self._beam_finder_widget = BeamFinderWidget(state=self.scripter.beam_finder)
        
        return {"Instrument": self._instrument_widget,
                "Beam Finder": self._beam_finder_widget}
        