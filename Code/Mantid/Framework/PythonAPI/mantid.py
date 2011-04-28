"""
    Simple interface to Mantid algorithms.
"""
import MantidFramework
MantidFramework.mtd.initialise()
import mantidsimple
import functools

def simple_algorithm(algorithm, *args, **kwargs):
    """
        Algorithm wrapper to simplify the Mantid algorithm call signature.
        Allows for the following variations:
    
            test_ws = CreateWorkspace(DataX=1,DataY=1,DataE=1)                     --> The output workspace name will be "test_ws"
            CreateWorkspace(OutputWorkspace="test_ws2", DataX=1,DataY=1,DataE=1)   --> The output workspace name will be "test_ws2"
            CreateWorkspace("test_ws3", DataX=1,DataY=1,DataE=1)                   --> The output workspace name will be "test_ws3"
            test_ws4 = CreateWorkspace(OutputWorkspace="some_str", DataX=1,DataY=1,DataE=1) --> The output workspace will be "test_ws4"
    """
    proxy = MantidFramework.mtd.createAlgorithm(algorithm)
                
    propertyOrder = MantidFramework.mtd._getPropertyOrder(proxy._getHeldObject())
    
    # Get the output workspace name
    output_ws = None
    for (key, arg) in zip(propertyOrder[:len(args)], args):
        if key=="OutputWorkspace":
            output_ws = arg
    if "OutputWorkspace" in kwargs:
        output_ws = kwargs["OutputWorkspace"]

    # Check whether the user wants to hold on to the workspace object, and take that name
    lhs = MantidFramework.lhs_info()
    if "OutputWorkspace" in propertyOrder and lhs[0]==1:
        output_ws = lhs[1][0]
        alg = eval("functools.partial(mantidsimple.%s, OutputWorkspace=\"%s\")" % (algorithm, output_ws))
    else:
        alg = eval("mantidsimple.%s" % algorithm)
    alg(*args, **kwargs)
        
    # If we have an output workspace, return the proxy to that workspace    
    if output_ws is not None:
        return MantidFramework.mtd[output_ws]
    return None
    
def translate():
    """
        Loop through the algorithms and register the simple algorithm wrapper 
        for each of them.
    """
    for algorithm in MantidFramework.mtd._getRegisteredAlgorithms():
        algorithm = algorithm[0]
        f = functools.partial(simple_algorithm, algorithm)
        f.__doc__ = MantidFramework.mtd.createAlgorithmDocs(algorithm)
        globals()[algorithm] = f
            
translate()


