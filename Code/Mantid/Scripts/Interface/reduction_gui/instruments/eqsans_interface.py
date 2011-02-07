"""
    This module defines the interface control for EQSANS.
    Each reduction method tab that needs to be presented is defined here.
    The actual view/layout is define in .ui files. The state of the reduction
    process is kept elsewhere (SNSReduction object)
"""
from interface import InstrumentInterface
from reduction_gui.widgets.sans_beam_finder import BeamFinderWidget
from reduction_gui.widgets.eqsans_instrument import SNSInstrumentWidget
from reduction_gui.widgets.sans_background import BackgroundWidget
from reduction_gui.widgets.output import OutputWidget
from reduction_gui.widgets.mask import MaskTabWidget
from reduction_gui.reduction.eqsans_reduction import EQSANSReductionScripter


class EQSANSInterface(InstrumentInterface):
    """
        Defines the widgets for EQSANS reduction
    """
    
    def __init__(self, name, settings):
        super(EQSANSInterface, self).__init__(name, settings)
        
        # Scripter object to interface with Mantid 
        self.scripter = EQSANSReductionScripter(name=name)        

        # Instrument description
        self.attach(SNSInstrumentWidget(settings = self._settings))
        
        # Beam finder
        self.attach(BeamFinderWidget(settings = self._settings))
        
        # Mask
        self.attach(MaskTabWidget(settings = self._settings))
        
        # Background
        self.attach(BackgroundWidget(settings = self._settings, show_transmission = False))
        
        # Reduction output
        self.attach(OutputWidget(settings = self._settings))
