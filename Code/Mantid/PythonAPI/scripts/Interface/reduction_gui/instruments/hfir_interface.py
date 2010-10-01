"""
    This module defines the interface control for HFIR SANS.
    Each reduction method tab that needs to be presented is defined here.
    The actual view/layout is define in .ui files. The state of the reduction
    process is kept elsewhere (HFIRReduction object)
"""
from PyQt4 import QtGui
from interface import InstrumentInterface
from reduction_gui.widgets.beam_finder import BeamFinderWidget
from reduction_gui.widgets.instrument import SANSInstrumentWidget
from reduction_gui.widgets.transmission import TransmissionWidget
from reduction_gui.widgets.background import BackgroundWidget
from reduction_gui.widgets.output import OutputWidget
from reduction_gui.reduction.hfir_reduction import HFIRReductionScripter


class HFIRInterface(InstrumentInterface):
    """
        Defines the widgets for HFIR reduction
    """
    
    def __init__(self, name, settings):
        super(HFIRInterface, self).__init__(name, settings)
        
        # Scripter object to interface with Mantid 
        self.scripter = HFIRReductionScripter(name=name)
        
        # Main panel with instrument description common to all data files
        self._instrument_widget = None
        # Beam finder panel
        self._beam_finder_widget = None
        # Sample transmission
        self._transmission_widget = None
        # Background
        self._background_widget = None
        # Reduction output
        self._output_widget = None
              
    def _warning(self, title, message):
        """
            Pop up a dialog and warn the user
            @param title: dialog box title
            @param message: message to be displayed
            
            #TODO: change this to signals and slots mechanism
        """
        QtGui.QMessageBox.warning(self._instrument_widget, title, message)
              
    def _push_to_scripter(self):
        """
            Pass the interface data to the scripter
        """
        self.scripter.instrument = self._instrument_widget.get_state()
        self.scripter.beam_finder = self._beam_finder_widget.get_state()
        self.scripter.transmission = self._transmission_widget.get_state()
        self.scripter.background = self._background_widget.get_state()
        
    def _update_from_scripter(self):
        """
            Update the interface with the scripter data
        """
        self._beam_finder_widget.set_state(self.scripter.beam_finder)
        self._instrument_widget.set_state(self.scripter.instrument)
        self._transmission_widget.set_state(self.scripter.transmission)
        self._background_widget.set_state(self.scripter.background)
        
    def reduce(self):
        """
            Pass the interface data to the scripter and reduce
        """
        super(HFIRInterface, self).reduce()
        
        #TODO: change this to standard widget get/set state
        self._output_widget.set_log(self._output_log)
        self._output_widget.plot_data(self.scripter.get_data())
        
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
        
        # Reduction output
        self._output_widget = OutputWidget(settings = self._settings)
        
        return [["Instrument", self._instrument_widget],
                ["Beam Center", self._beam_finder_widget],
                ["Transmission", self._transmission_widget],
                ["Background", self._background_widget],
                ['Output', self._output_widget]]
        