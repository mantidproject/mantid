"""
    This module defines the interface control for HFIR SANS.
    Each reduction method tab that needs to be presented is defined here.
    The actual view/layout is define in .ui files. The state of the reduction
    process is kept elsewhere (HFIRReduction object)
"""
from PyQt4 import QtGui, uic, QtCore
from widgets.beam_finder import BeamFinderWidget
from widgets.instrument import SANSInstrumentWidget
from widgets.transmission import TransmissionWidget
from widgets.background import BackgroundWidget
from widgets.data_sets import DataWidget
from reduction.hfir_reduction import ReductionScripter


class HFIRInterface(object):
    """
        Defines the widgets for HFIR reduction
    """
    ui_path = 'ui'
    
    def __init__(self, name, settings):
        self.scripter = ReductionScripter(name=name)
        # Main panel with instrument description common to all data files
        self._instrument_widget = None
        # Beam finder panel
        self._beam_finder_widget = None
        # Sample transmission
        self._transmission_widget = None
        # Background
        self._background_widget = None
        # Data sets
        self._data_widget = None
        
        # General settings
        self._settings = settings
        
        
    def load_file(self, file_name):
        """
            Load an XML file containing reduction parameters and
            populate the UI with them
            @param file_name: XML file to be loaded
        """
        self.scripter = ReductionScripter(name=self.scripter.instrument_name)
        self.scripter.from_xml(file_name)
        self._beam_finder_widget.set_state(self.scripter.beam_finder)
        self._instrument_widget.set_state(self.scripter.instrument)
        self._transmission_widget.set_state(self.scripter.transmission)
        self._background_widget.set_state(self.scripter.background)
        self._data_widget.set_state(self.scripter.data_sets)
        
    def save_file(self, file_name):
        """
            Save the content of the UI as a settings file that can 
            be reloaded
            @param file_name: XML file to be saved
        """
        self._push_to_scripter()
        self.scripter.to_xml(file_name)
        
    def export(self, file_name):
        """
            Export the content of the UI as a python script that can 
            be run within Mantid
            @param file_name: name of the python script to be saved
        """
        self._push_to_scripter()
        self.scripter.to_script(file_name)
        
    def _push_to_scripter(self):
        """
            Pass the interface data to the scripter
        """
        self.scripter.instrument = self._instrument_widget.get_state()
        self.scripter.beam_finder = self._beam_finder_widget.get_state()
        self.scripter.transmission = self._transmission_widget.get_state()
        self.scripter.background = self._background_widget.get_state()
        self.scripter.data_sets = self._data_widget.get_state()
        
    def reduce(self):
        """
            Pass the interface data to the scripter and reduce
        """
        self._push_to_scripter()
        
        # Print the script for now
        print self.scripter.to_script()
        
    def get_tabs(self):
        """
            Returns a list of widgets used to populate the central tab widget
            of the interface.
        """
        # Instrument description
        self._instrument_widget = SANSInstrumentWidget(settings = self._settings)
        
        # Beam finder
        self._beam_finder_widget = BeamFinderWidget(settings = self._settings)
        
        # Transmission
        self._transmission_widget = TransmissionWidget(settings = self._settings)
        
        # Background
        self._background_widget = BackgroundWidget(settings = self._settings)
        
        # Data sets
        self._data_widget = DataWidget(settings = self._settings)
        
        return [["Instrument", self._instrument_widget],
                ["Beam Center", self._beam_finder_widget],
                ["Transmission", self._transmission_widget],
                ["Background", self._background_widget],
                ["Data Sets", self._data_widget]]
        