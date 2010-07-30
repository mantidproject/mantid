"""
    SANS-specific implementation of the Reducer. The SANSReducer class implements 
    a predefined set of reduction steps to be followed. The actual ReductionStep objects
    executed for each of those steps can be modified.
"""
from Reducer import Reducer
from Reducer import ReductionStep
import SANSReductionSteps
from mantidsimple import *

## Version number
__version__ = '0.0'

class SANSReducer(Reducer):
    """
        SANS-specific implementation of the Reducer
    """
    ## Normalization options
    #TODO: those also correspond to the timer and monitor spectra -> store this in instr conf instead
    NORMALIZATION_NONE = None
    NORMALIZATION_TIME = 1
    NORMALIZATION_MONITOR = 0    
    
    ## Beam center finder ReductionStep object
    _beam_finder = None 
    
    ## Normalization option
    _normalizer = None
    
    ## Dark current data file
    _dark_current_subtracter = None
    
    ## Sensitivity correction ReductionStep object
    _sensitivity_correcter = None
    
    ## Solid angle correcter
    _solid_angle_correcter = None
    
    ## Azimuthal averaging
    _azimuthal_averager = None
    
    def __init__(self):
        super(SANSReducer, self).__init__()
        
        # Default beam finder
        self._beam_finder = SANSReductionSteps.BaseBeamFinder(0,0)
        
        # Default normalization
        self._normalizer = SANSReductionSteps.Normalize(SANSReducer.NORMALIZATION_TIME)
        
        # By default, we want the solid angle correction
        self._solid_angle_correcter = SANSReductionSteps.SolidAngle()
        
        # By default, use the weighted azimuthal averaging
        self._azimuthal_averager = SANSReductionSteps.WeightedAzimuthalAverage() 
    
    def set_normalizer(self, option):
        """
            Set normalization option (time or monitor)
            @param option: normalization option
        """
        if option not in (None, SANSReducer.NORMALIZATION_TIME, 
                          SANSReducer.NORMALIZATION_MONITOR):
            raise RuntimeError, "SANSReducer.set_normalization: unrecognized normalization option."
        
        if option is None:
            self._normalizer = None
        else:
            self._normalizer = SANSReductionSteps.Normalize(option)
        
    def set_beam_center(self, x, y):
        """
             Set the beam center
             @param x: x-position of the beam
             @param y: y-position of the beam
        """
        # Check that we have floats
        try: 
            x = float(x)
            y = float(y)
        except:
            raise RuntimeError, "Reducer.set_beam_center: input coordinates are expected to be floats (%s %s)" % (str(x), str(y))
        self._beam_finder = SANSReductionSteps.BaseBeamFinder(x,y)
        
    def get_beam_center(self): 
        return self._beam_finder.get_beam_center()
    
    def set_beam_finder(self, finder):
        if issubclass(finder.__class__, SANSReductionSteps.BaseBeamFinder) or None:
            self._beam_finder = finder
        else:
            raise RuntimeError, "Reducer.set_beam_finder expects an object of class ReductionStep"
    
    def set_sensitivity_correcter(self, correcter):
        if issubclass(correcter.__class__, ReductionStep) or None:
            self._sensitivity_correcter = correcter
        else:
            raise RuntimeError, "Reducer.set_sensitivity_correcter expects an object of class ReductionStep"
    
    def set_dark_current_subtracter(self, subtracter):
        if issubclass(subtracter.__class__, ReductionStep) or None:
            self._dark_current_subtracter = subtracter
        else:
            raise RuntimeError, "Reducer.set_dark_current expects an object of class ReductionStep"
    
    def set_solid_angle_correcter(self, correcter):
        if issubclass(correcter.__class__, ReductionStep) or None:
            self._solid_angle_correcter = correcter
        else:
            raise RuntimeError, "Reducer.set_solid_angle_correcter expects an object of class ReductionStep"
    
    def set_azimuthal_averager(self, averager):
        if issubclass(averager.__class__, ReductionStep) or None:
            self._azimuthal_averager = averager
        else:
            raise RuntimeError, "Reducer.set_azimuthal_averager expects an object of class ReductionStep"
    
    def pre_process(self): 
        """
            Reduction steps that are meant to be executed only once per set
            of data files. After this is executed, all files will go through
            the list of reduction steps.
        """
        if self._beam_finder is not None:
            self._beam_finder.execute(self)

    
    def post_process(self): raise NotImplemented
    
    def _to_steps(self, file_ws):
        
        # Load file
        self.append_step(SANSReductionSteps.LoadRun(datafile=self._data_files[file_ws]))
        
        # Dark current subtraction
        if self._dark_current_subtracter is not None:
            self.append_step(self._dark_current_subtracter)
        
        # Normalize
        if self._normalizer is not None:
            self.append_step(self._normalizer)
        
        # Mask
        
        # Sensitivity correction
        if self._sensitivity_correcter is not None:
            self.append_step(self._sensitivity_correcter)
            
        # Solid angle correction
        if self._solid_angle_correcter is not None:
            self.append_step(self._solid_angle_correcter)
        
        # Perform azimuthal averaging
        if self._azimuthal_averager is not None:
            self.append_step(self._azimuthal_averager)
    
    def reduce(self):
        """
            Go through the list of reduction steps
        """
        # Go through the list of steps that are common to all data files
        self.pre_process()
        
        for file_ws in self._data_files:
            # Create the list of reduction steps
            self._to_steps(file_ws)
            
            for item in self._reduction_steps:
                item.execute(self, file_ws)
        
        
        