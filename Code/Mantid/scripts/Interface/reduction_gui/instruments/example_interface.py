
from interface import InstrumentInterface
from reduction_gui.widgets.example_widget import ExampleWidget
#from reduction_gui.reduction.example_reduction import ExampleScripter
from reduction_gui.reduction.scripter import BaseReductionScripter
class ExampleInterface(InstrumentInterface):
    """
        Defines the widgets for HFIR reduction
    """
    # Allowed extensions for loading data files
    data_type = "Data files *.nxs *.dat (*.nxs *.dat)"
    
    def __init__(self, name, settings):
        super(ExampleInterface, self).__init__(name, settings)
        
        # Scripter object to interface with Mantid 
        self.scripter = BaseReductionScripter(name=name)        

        # Instrument description
        self.attach(ExampleWidget(settings = self._settings, data_type = self.data_type))
        