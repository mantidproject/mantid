"""
    Simple Reducer example
"""
from MantidFramework import *
from reduction import Reducer
# Validate_step is a decorator that allows both Python algorithms and ReductionStep objects to be passed to the Reducer.
# It also does minimal type checking to ensure that the object that is passed is valid
from reduction import validate_step, validate_loader
from mantidsimple import *
mtd.initialise()

class ReflReducer(Reducer):
    
    # Place holders for the reduction steps
    _loader = None
        
    def __init__(self):
        super(ReflReducer, self).__init__()
        
    @validate_loader
    def set_loader(self, step):
        """
            Set loader algorithm
        """
        self._loader = step
            
    def pre_process(self): 
        """
            Create the list of algorithms that will be run.
            - This is the place to set algorithm properties if you need to
        """
        self._reduction_steps = []
        if self._loader is not None:
            self.append_step(self._loader) 
            


if __name__ == '__main__':  
    # Instantiate the Reducer object
    r = ReflReducer()
    
    r.append_data_file("REF_L_51440_event.nxs")
    r.set_loader(RefLoad, Filename=None, OutputWorkspace=None, PixelSizeX=0.0007, PixelSizeY=0.00075)
    
    r.reduce()
    
    
    


