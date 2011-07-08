"""
    Simple Reducer example
"""
from MantidFramework import *
from reduction import Reducer

# Validate_step is a decorator that allows both Python algorithms and ReductionStep objects to be passed to the Reducer.
# It also does minimal type checking to ensure that the object that is passed is valid
from reduction import validate_step
from mantidsimple import *

class InelasticReducer(Reducer):
    """
        Inelastic-specific implementation of the Reducer
    """
    
    ## Data loader
    _data_loader = None
    
    ## Apply Ki/Kf correction
    _kikf_corrector = None
    
    ## Detector Efficiency
    _detector_efficiency_corrector = None
    
    ## Background subtraction
    _background_subtractor = None
    
    ## Masking
    _masker = None
    
    ## Save NXSPE file
    _nxspe_saver = None
    
    ## Save Mantid NeXus file
    _save_mantid_nexus = None
    
    ## Save SPE file 
    _save_spe = None
    
    
    
    def __init__(self):
        super(InelasticReducer, self).__init__()
    
    
    def set_data_loader(self, loader):
        """
            Set the algorithm to load the data files
            @param loader: Workflow algorithm object
        """ 
        self._data_loader = loader

    def set_kikf_correction(self, corrector):
        """
            Set the step that will apply the Ki/Kf scaling correction
        """
        self._kikf_corrector = corrector


    def set_detector_efficiency(self, corrector):
        """
            Set the step that will apply the detector efficiency correction
        """
        self._detector_efficiency_corrector = corrector


    def set_background_subtractor(self, subtractor):
        """
            Set the step that will apply the Ki/Kf scaling correction
        """
        self._background_subtractor = subtractor

    def set_masker(self, masker):
        """
            Set the step that will apply the mask
        """

        
    def pre_process(self): 
        """
            Create the list of algorithms that will be run.
        """
        if self._data_loader is not None:
            self.append_step(self._data_loader) 


#    def _absolute_norm_steps(self):
#        """
#            Creates a list of steps for each data set in the 
#            processing queue in order to calculate the absolute units.
#        """
        

if __name__ == '__main__':  
    # Instantiate the Reducer object
    r = InelasticReducer()
    r.reduce()
