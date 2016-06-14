from mantid.api import AlgorithmManager


# --------------------------------------------
# Free functions
# -------------------------------------------

def create_unmanaged_algorithm(name, **kwargs):
    alg = AlgorithmManager.createUnmanaged(name)
    alg.initialize()
    alg.setChild(True)
    for key, value in kwargs.iteritems():
        alg.setProperty(key, value)
    return alg
