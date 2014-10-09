import numpy as np
from mantid.api import MatrixWorkspace as MatrixWorkspace
from mantid.simpleapi import CreateWorkspace as CreateWorkspace
import mantidplot  

def __is_array(arg):
    return isinstance(arg, list) or isinstance(arg, np.ndarray) 

def __is_workspace(arg):
    return isinstance(arg, MatrixWorkspace)

def __is_array_of_workspaces(arg):
    return __is_array(arg) and len(arg) > 0 and __is_workspace(arg[0])


def __plot_as_workspace(arg):
    mantidplot.plotSpectrum(arg, 0)

def __plot_as_workspaces(arg):
    # plotSpectrum can already handle 1 or more input workspaces.
    return __plot_as_workspace(arg)
        

def __plot_as_array(y, *args, **kwargs):
    if len(args) != 0:
        if __is_array(args):
            ws = CreateWorkspace(DataX=args, DataY=y)     
        else:
            raise ValueError("Inputs are of type: " + str(type(args)) + ". Not plottable." )
    else:
        x = range(0, len(y), 1) # 0 to n, incremented by 1.
        ws = CreateWorkspace(DataX=x, DataY=y)   
    return __plot_as_workspace(ws)



def plot (y, x=None, marker=None):
    print type(y)
    if __is_array(y):
        if __is_array_of_workspaces(y):
            __plot_as_workspaces(y)
        else:
            __plot_as_array(y)
    elif __is_workspace(y):
        __plot_as_workspace(y)
    else:
        raise ValueError("Cannot plot argument of type " + str(type(y)) )
         

