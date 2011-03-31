"""
    Simple Reducer example
"""
from reduction import Reducer
# Validate_step is a decorator that allows both Python algorithms and ReductionStep objects to be passed to the Reducer.
# It also does minimal type checking to ensure that the object that is passed is valid
from reduction import validate_step
from mantidsimple import *

class ExampleReducer(Reducer):
    
    # Place holders for the reduction steps
    _first_step = None
    _second_step = None
        
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
            
    def pre_process(self): 
        """
            Create the list of algorithms that will be run.
            - This is the place to set algorithm properties if you need to
        """
        if self._first_step is not None:
            self._reduction_steps.append(self._first_step) 
            
        if self._second_step is not None:
            # When we are able to set an algorithm object as a property, 
            # it should be done here:
            #self._second_step.algm.setAlgorithmProperty("Background", some_algorithm_object)
            self._reduction_steps.append(self._second_step) 


if __name__ == '__main__':  
    r = ExampleReducer()
    r.append_data_file("~/sample_data.txt")
    
    r.set_first_step(CreateWorkspace, OutputWorkspace="sample_data", DataX='1', DataY='1', DataE='1')
    r.set_second_step(Scale, InputWorkspace=None, OutputWorkspace=None, Factor='2.5')
    
    r.reduce()
    
    
    


