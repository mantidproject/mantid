import numpy as np
from mantid.api import MatrixWorkspace as MatrixWorkspace

def __is_array(arg):
    return isinstance(arg, list) or isinstance(arg, np.ndarray) 

def __is_workspace(arg):
    return isinstance(arg, MatrixWorkspace)

def __is_array_of_workspaces(arg):
    return __is_array(arg) and len(arg) > 0 and __is_workspace(arg[0])


def __plot_as_workspace(arg):
    print "TODO plot_as_workspace"
    pass

def __plot_as_workspaces(arg):
    print "TODO plot as workspaces"
    pass

def __plot_as_array(arg):
    print "TODO plot as array"
    pass



def plot (y, x=None, marker=None):
    print type(y)
    if __is_array(y):
        if __is_array_of_workspaces(y):
            __plot_as_workspaces(y)
        elif __is_workspace(y):
            __plot_as_workspace(y)
        else:
            __plot_as_array(y)
    else:
        raise ValueError("Cannot plot argument of type " + str(type(y)) )
         

