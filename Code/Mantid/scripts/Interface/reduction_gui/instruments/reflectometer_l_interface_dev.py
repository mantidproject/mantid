"""
    This module defines the interface control for SNS REF_L
    Each reduction method tab that needs to be presented is defined here.
    The actual view/layout is define in .ui files. The state of the reduction
    process is kept elsewhere (REFLReduction object)
"""
from interface import InstrumentInterface
from reduction_gui.widgets.reflectometer.refl_data_simple import DataReflWidget
from reduction_gui.widgets.reflectometer.stitcher import StitcherWidget
#from reduction_gui.widgets.reflectometer.refl_parameters import ParametersReflWidget
#from reduction_gui.widgets.reflectometer.refl_norm import NormReflWidget
#from reduction_gui.widgets.reflectometer.advanced import AdvancedWidget
#from reduction_gui.widgets.output import OutputWidget
    
from reduction_gui.reduction.reflectometer.refl_reduction import REFLReductionScripter

from reduction_gui.reduction.reflectometer.refl_data_proxy import DataProxy

class REFLInterface(InstrumentInterface):
    """
        Defines the widgets for REF_L reduction
    """
    
    def __init__(self, name, settings):
        super(REFLInterface, self).__init__(name, settings)
        
        # Scripter object to interface with Mantid 
        self.scripter = REFLReductionScripter(name=name)        

        # data REF_L tab
        self.attach(DataReflWidget(settings = self._settings, name=name))
        self.attach(StitcherWidget(settings = self._settings))
        
        # normalization REF_L tab
        #self.attach(NormReflWidget(settings = self._settings, data_proxy=DataProxy))

        # Parameters REF_L tab
        #self.attach(ParametersReflWidget(settings = self._settings, data_proxy=DataProxy))

        # Parameters advanced tab
        #self.attach(AdvancedWidget(settings = self._settings, data_proxy=DataProxy))
        
        # Reduction output
        #self.attach(OutputWidget(settings = self._settings))