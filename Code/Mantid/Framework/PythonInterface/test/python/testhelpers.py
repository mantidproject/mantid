"""A module of helper functions for the Python unit tests
"""

from mantid.api import algorithm_mgr

def run_algorithm(name, **kwargs):
    """Run a named algorithm and return the
    algorithm handle

    Parameters:
        name - The name of the algorithm
        kwargs - A dictionary of property name:value pairs
    """
    alg = algorithm_mgr.create_unmanaged(name)
    alg.initialize()
    # Avoid problem that Load needs to set Filename first if it exists
    if name == 'Load' and 'Filename' in kwargs:
        alg.set_property('Filename', kwargs['Filename'])
        del kwargs['Filename']
    if 'child'in kwargs:
        alg.set_child(True)
        del kwargs['child']
        alg.set_property_value("OutputWorkspace","UNUSED_NAME_FOR_CHILD")
    for key, value in kwargs.iteritems():
        alg.set_property(key, value)
    alg.execute()
    return alg

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