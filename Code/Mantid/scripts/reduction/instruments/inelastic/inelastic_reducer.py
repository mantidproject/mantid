"""
    Simple Reducer example
"""
from MantidFramework import *
from reduction import Reducer

# Validate_step is a decorator that allows both Python algorithms and ReductionStep objects to be passed to the Reducer.
# It also does minimal type checking to ensure that the object that is passed is valid
from reduction import validate_step
from mantidsimple import *
mtd.initialise()

class InelasticReducer(Reducer):
    
    _data_loader = None
    
    def __init__(self):
        super(InelasticReducer, self).__init__()
        
    def pre_process(self): 
        """
            Create the list of algorithms that will be run.
        """
        if self._data_loader is not None:
            self.append_step(self._data_loader) 

if __name__ == '__main__':  
    # Instantiate the Reducer object
    r = InelasticReducer()
    r.reduce()
