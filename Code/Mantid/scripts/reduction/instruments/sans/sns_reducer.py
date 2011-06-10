"""
    Reducer class for EQSANS reduction
    
    Implemented steps:
        _beam_finder  
        _normalizer
        _dark_current_subtracter
        _sensitivity_correcter
        _solid_angle_correcter 
        _azimuthal_averager
        _transmission_calculator
        _mask - needs more shapes, such as ellipse
        _data_loader
        _background_subtracter

"""
from sans_reducer import SANSReducer
from reduction import ReductionStep
import sns_reduction_steps
import sans_reduction_steps

class EqSansReducer(SANSReducer):

    def __init__(self):
        super(EqSansReducer, self).__init__()
        ## Default beam finder
        self._beam_finder = sans_reduction_steps.BaseBeamFinder(96.29, 126.15)
        ## Default data loader
        self._data_loader = sns_reduction_steps.LoadRun()
        ## Normalization
        self._normalizer = sns_reduction_steps.Normalize()
        ## Transmission calculator
        self._transmission_calculator = sns_reduction_steps.BeamStopTransmission(True)
        
        self._solid_angle_correcter = sans_reduction_steps.SolidAngle()
        
        # Default dark current subtracter class
        self._dark_current_subtracter_class = sns_reduction_steps.SubtractDarkCurrent
        
        # No resolution calculation for now
        self._resolution_calculator = None

    def set_instrument(self, configuration):
        super(SANSReducer, self).set_instrument(configuration)

    def run_to_data_file(self, run_number):
        """
            Transforms a run number into the best guess for a corresponding
            data file for this instrument
            @param run_number: run number (int)
        """
        run = None
        try:
            run = int(run_number)
        except:
            # Not a run number...
            return None
        return "EQSANS_%d_event.nxs" % run
        
    def set_normalizer(self, normalizer):
        """
            Set the ReductionStep object that takes care of normalization
            @param normalizer: ReductionStep object
        """
        if issubclass(normalizer.__class__, ReductionStep) or normalizer is None:
            self._normalizer = normalizer
        else:
            raise RuntimeError, "Reducer.set_normalizer expects an object of class ReductionStep"
