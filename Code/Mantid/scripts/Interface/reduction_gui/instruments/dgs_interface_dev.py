from interface import InstrumentInterface
from reduction_gui.widgets.inelastic.dgs_sample_setup import SampleSetupWidget
from reduction_gui.reduction.inelastic.dgs_reduction_script import DgsReductionScripter
class DgsInterface(InstrumentInterface):
    """
        Defines the widgets for direct geometry spectrometer reduction
    """
    # Allowed extensions for loading data files
    data_type = "Data files *.nxs *.dat (*.nxs *.dat)"
    
    def __init__(self, name, settings):
        super(DgsInterface, self).__init__(name, settings)
        
        # Scripter object to interface with Mantid 
        self.scripter = DgsReductionScripter()        

        # Sample run setup
        self.attach(SampleSetupWidget(settings = self._settings))#, data_type = self.data_type))
        