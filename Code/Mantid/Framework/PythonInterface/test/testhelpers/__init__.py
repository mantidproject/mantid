"""Defines a set of helpers wrapped around the C++ TestHelpers package that
are for use in unit tests only!
"""
# Define all mantid exported classes first
import mantid

# Add workspace creation namespace
import WorkspaceCreationHelper

# Define some pure-Python functions to add to the mix

def run_algorithm(name, **kwargs):
    """Run a named algorithm and return the
    algorithm handle

    Parameters:
        name - The name of the algorithm
        kwargs - A dictionary of property name:value pairs
    """
    alg = create_algorithm(name, **kwargs)
    alg.execute()
    return alg

def create_algorithm(name, **kwargs):
    """Create a named algorithm, set the properties given by the keywords and return the
    algorithm handle WITHOUT executing the algorithm

    Useful keywords:
      - child: Makes algorithm a child algorithm
      - rethrow: Causes exceptions to be rethrown on execution

    Parameters:
        name - The name of the algorithm
        kwargs - A dictionary of property name:value pairs
    @returns The algorithm handle
    """
    alg = mantid.api.AlgorithmManager.createUnmanaged(name)
    alg.initialize()
    # Avoid problem that Load needs to set Filename first if it exists
    if name == 'Load' and 'Filename' in kwargs:
        alg.setPropertyValue('Filename', kwargs['Filename'])
        del kwargs['Filename']
    if 'child'in kwargs:
        alg.setChild(True)
        del kwargs['child']
        if 'OutputWorkspace' in alg:
            alg.setPropertyValue("OutputWorkspace","UNUSED_NAME_FOR_CHILD")
    if 'rethrow' in kwargs:
        alg.setRethrows(True)
        del kwargs['rethrow']
    for key, value in kwargs.iteritems():
        alg.setProperty(key, value)
    return alg

# Case difference is to be consistent with the unittest module
def assertRaisesNothing(testobj, callable, *args, **kwargs):
    """
        unittest does not have an assertRaisesNothing. This
        provides that functionality

        Parameters:
            testobj  - A unittest object
            callable - A callable object
            *args    - Positional arguments passed to the callable as they are
            **kwargs - Keyword arguments, passed on as they are
    """
    try:
         return callable(*args, **kwargs)
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
