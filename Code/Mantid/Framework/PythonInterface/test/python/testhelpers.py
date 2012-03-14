"""A module of helper functions for the Python unit tests
"""

from mantid.api import AlgorithmManager

def run_algorithm(name, **kwargs):
    """Run a named algorithm and return the
    algorithm handle

    Parameters:
        name - The name of the algorithm
        kwargs - A dictionary of property name:value pairs
    """
    alg = AlgorithmManager.Instance().createUnmanaged(name)
    alg.initialize()
    # Avoid problem that Load needs to set Filename first if it exists
    if name == 'Load' and 'Filename' in kwargs:
        alg.setProperty('Filename', kwargs['Filename'])
        del kwargs['Filename']
    if 'child'in kwargs:
        alg.setChild(True)
        del kwargs['child']
        if 'OutputWorkspace' in alg:
            alg.setPropertyValue("OutputWorkspace","UNUSED_NAME_FOR_CHILD")
    for key, value in kwargs.iteritems():
        alg.setProperty(key, value)
    alg.execute()
    return alg

# Case difference is to be consistent with the unittest module
def assertRaisesNothing(testobj, callable, *args): 
    """
        unittest does not have an assertRaisesNothing. This
        provides that functionality
    
        Parameters:
            testobj  - A unittest object
            callable - A callable object
            *args    - Positional arguments passed to the callable as they are
    """
    try:
         return callable(*args)
    except Exception, exc:
        testobj.fail("Assertion error. An exception was caught where none was expected in %s. Message: %s" 
                     % (callable.__name__, str(exc)))

def can_be_instantiated(cls):
    """The Python unittest assertRaises does not
    seem to catch the assertion raised by being unable
    to instantiate a class (or maybe it's just the boost
    python stuff).
    In any case this little function tests for it and returns
    a boolean
    """
    try:
        cls()
        result = True
    except RuntimeError:
        result = False
    return result
