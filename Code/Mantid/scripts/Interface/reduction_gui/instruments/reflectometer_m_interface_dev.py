"""
    This module defines the interface control for SNS REF_M.
    Each reduction method tab that needs to be presented is defined here.
    The actual view/layout is define in .ui files. The state of the reduction
    process is kept elsewhere (REFMReduction object)
"""
from interface import InstrumentInterface
from reduction_gui.widgets.reflectometer.refm_data import DataRefmWidget
from reduction_gui.widgets.reflectometer.refm_parameters import ParametersRefmWidget
from reduction_gui.widgets.reflectometer.refm_norm import NormRefmWidget
from reduction_gui.widgets.reflectometer.advanced import AdvancedWidget

#from reduction_gui.reduction.hfir_reduction import HFIRReductionScripter

#from reduction_gui.reduction.sans.hfir_data_proxy import DataProxy

class REFMInterface(InstrumentInterface):
    """
        Defines the widgets for REF_M reduction
    """
    
    def __init__(self, name, settings):
        super(REFMInterface, self).__init__(name, settings)
        
        # Scripter object to interface with Mantid 
        self.scripter = HFIRReductionScripter(name=name)        





        # data REF_L tab
        self.attach(DataRefmWidget(settings = self._settings, name=name, data_proxy=DataProxy))
        
        # normalization REF_L tab
        self.attach(NormRefmWidget(settings = self._settings, data_proxy=DataProxy))

        # Parameters REF_L tab
        self.attach(ParametersRefmWidget(settings = self._settings, data_proxy=DataProxy))
        
        # merging
        self.attach(RefmMergingWidget(settings = self._settings, data_proxy=DataProxy))
        
        # Reduction output
        self.attach(OutputWidget(settings = self._settings))
