"""
    Reducer class for EQSANS reduction
"""
from sans_reducer import SANSReducer
import sns_reduction_steps
import sans_reduction_steps

class EqSansReducer(SANSReducer):

    ## Transform TOF to wavelength
    tof_to_wavelength = None
    
    def __init__(self):
        super(EqSansReducer, self).__init__()
        # Default beam finder
        self._beam_finder = sans_reduction_steps.BaseBeamFinder(0,0)
        # Default data loader
        self._data_loader = sns_reduction_steps.LoadRun()
        # Skip normalization, which is currently part of the Loader 
        # since it's always a normalization to monitor
        self._normalizer = None

    def set_instrument(self, configuration):
        super(SANSReducer, self).set_instrument(configuration)
        #self._beam_finder = sans_reduction_steps.BaseBeamFinder(self.instrument.nx_pixels/2, self.instrument.ny_pixels/2)
        self._beam_finder = sans_reduction_steps.BaseBeamFinder(96.3, 125.95)

    

        
