"""
    Runtime version of mantidsimple.py
    
    Note: to use the runtime version of mantidsimple, call mtd.runtime_mtdsimple() 
    before calling mtd.initialise()
    
    The only difference between the functions created by mantidsimple.py and
    this module is the signature. All runtime algorithm functions have a signature
    like Algorithm(*args, **kwargs). The help documentation is the same.
"""
from MantidFramework import *
from MantidFramework import _makeString

def version():
    return "mantidsimple - runtime version"

def numberRows(descr, fw):
  des_len = len(descr)
  if des_len == 0:
    return (1, [''])
  nrows = 0
  i = 0
  descr_split = []
  while i < des_len:
    nrows += 1
    descr_split.append(descr[i:i+fw])
    i += fw
  return (nrows, descr_split)
  
def create_algorithm(algorithm):
    """
        Create a function that will set up and execute an algorithm
        @param algorithm: name of the algorithm
    """
    
    def algorithm_wrapper(*args, **kwargs):
        version = -1
        if "Version" in kwargs:
            version = kwargs["Version"]
            kwargs.pop(kwargs.index("Version"))
        algm = mtd.createAlgorithm(algorithm, version)
        algm.setPropertyValues(*args, **kwargs)
        algm.execute()
        return algm
    
    algorithm_wrapper.__name__ = algorithm
    algorithm_wrapper.__doc__ = mtd.createAlgorithmDocs(algorithm)
    return algorithm_wrapper
    
def translate():
    """
        Loop through the algorithms and register a function call 
        for each of them
    """
    for algorithm in mtd._getRegisteredAlgorithms():
        globals()[algorithm[0]] = create_algorithm(algorithm[0])
            
translate()


