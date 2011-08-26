"""Defines the api sub-package
"""
import sys

from kernel import dlopen
flags = dlopen.setup_dlopen() # Ensure the library is open with the correct flags
from _api import *
dlopen.restore_flags(flags)

# Alias the singleton objects
framework_mgr = get_framework_mgr() # The first import of this starts the framework
algorithm_mgr = get_algorithm_mgr()

# Control what an import * does
__all__ = dir(_api)
__all__.extend(['algorithm_mgr', 'framework_mgr'])


#########################################################################################
## Private methods (not in global import) 
#########################################################################################

def IAlgorithm_dynamic_getattr(self, name):
    """Dark magic so that there can be a single function call set_property that routes to
    the appropriate place.
    
    The issue is actually only with the basic numeric types. Boost Python selects
    the last method that was registered when the argument is int, float or bool because
    Boost Python determines that it is acceptable to convert to the type of that function
    but Mantid won't accept it because the type then doesn't match the property declaration.
    For example, assume an algorithm has a property declared as type int:
      
      Calling set_property('Name', 10) leads Boost.Python to call the first function
      it can find that will accept that type. If the C++ setProperty taking a double
      was exported last then it casts the int to a double and calls that function
      resulting in Mantid saying there is a type mismatch
      
      The problem is simple reversed if you export the int set_property last as then
      that fails for float arguments
    
    This function replaces the __getattribute__ function on IAlgorithm and returns
    a different method call depending on the type of arguments that have been 
    passed to the original function call. It is only the basic types that suffer so
    everything else is routed to the generic set_property    
    """
    if name != 'set_property':
        return object.__getattribute__(self, name)

    def set_property(self, propname, value):
        if type(value) is int:
            method = object.__getattribute__(self, name + '_int')
        elif type(value) is float:
            method = object.__getattribute__(self, name + '_float')
        elif type(value) is bool:
            method = object.__getattribute__(self, name + '_bool')
        else:
            method = object.__getattribute__(self, name)
        method(propname, value)
    return set_property.__get__(self) 

# Replace the standard __get__attribute method
IAlgorithm.__getattribute__ = IAlgorithm_dynamic_getattr