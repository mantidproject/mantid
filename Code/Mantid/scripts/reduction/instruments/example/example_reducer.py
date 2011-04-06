"""
    Simple Reducer example
"""
from MantidFramework import *
from reduction import Reducer
from reduction.instruments.example.ExampleRedStep import ExampleRedStep

# Validate_step is a decorator that allows both Python algorithms and ReductionStep objects to be passed to the Reducer.
# It also does minimal type checking to ensure that the object that is passed is valid
from reduction import validate_step
from mantidsimple import *
mtd.initialise()

class ExampleReducer(Reducer):
    
    # Place holders for the reduction steps
    _first_step = None
    _second_step = None
    
    # Algorithm used as part of a step
    _normalizer = None
        
    def __init__(self):
        super(ExampleReducer, self).__init__()
        
    @validate_step
    def set_first_step(self, step):
        """
            Set a reduction step. The 'step' parameter is expected to be 
            a ReductionStep object at this point, but this should be
            transparent to the developer. Nothing but setting the private
            data member should be done here.
            
            @validate_step processes the original inputs. There are two
            ways to call set_first_step():
            
            Method 1 (old): reducer.set_first_step(reduction_step)
              where reduction_step is a ReductionStep object
              
            Method 2 (new): reducer.set_first_step(Algorithm, *args, **argv)
              where Algorithm is an anlgorithm function from mantidsimple,
              following by its arguments. 
              
              For example: 
                reducer.set_first_step(Scale, InputWorkspace=None, OutputWorkspace=None, Factor='2.5')
              
              The arguments follow the signature of the mantidsimple function.
              
              See original ticket for more details:
                http://trac.mantidproject.org/mantid/ticket/2129l
        """
        self._first_step = step
            
    @validate_step
    def set_second_step(self, step):
        self._second_step = step
        
    @validate_step
    def set_normalizer(self, normalize_algorithm):
        """
            This is an example of a plug-and-play algorithm to be
            used as part of a reduction step 
        """
        self._normalizer = normalize_algorithm
            
    def pre_process(self): 
        """
            Create the list of algorithms that will be run.
            - This is the place to set algorithm properties if you need to
        """
        if self._first_step is not None:
            self.append_step(self._first_step) 
            
        if self._second_step is not None:
            # When we are able to set an algorithm object as a property, 
            # it should be done here:
            self._second_step.get_algorithm().setProperty("Algorithm", self._normalizer.get_algorithm())
            self.append_step(self._second_step) 


if __name__ == '__main__':  
    
    # Instantiate the Reducer object
    r = ExampleReducer()
    
    # Append a fake data file
    r.append_data_file("~/sample_data.txt")
    
    # Example of a standard algorithm used as a reduction step. Note that InputWorkspace and OutputWorkspace
    # are overwritten by the Reducer. They can be set to Non at this point.
    r.set_first_step(CreateWorkspace, OutputWorkspace=None, DataX='1', DataY='1', DataE='1')
    
    # Set up an algorithm to be used as part of a reduction step
    alg = mtd._createAlgProxy("Scale")
    alg.setPropertyValues(Factor="2.5")
    r.set_normalizer(alg)
    
    # Set up the actual reduction step that will use the algorithm
    step = ExampleRedStep()
    step.initialize()
    r.set_second_step(step)
    
    r.reduce()
    
    
    


