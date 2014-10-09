import numpy as np
from mantid.api import MatrixWorkspace as MatrixWorkspace
from mantid.simpleapi import CreateWorkspace as CreateWorkspace
import mantidplot  

def __is_array(arg):
    """
        Is the argument a python or numpy list?
        @param arg :: argument
        
        Returns :: True if the argument a python or numpy list
    """
    return isinstance(arg, list) or isinstance(arg, np.ndarray) 

def __is_workspace(arg):
    """
        Is the argument a Mantid MatrixWorkspace?
        @param arg :: argument
        
        Returns :: True if the argument a MatrixWorkspace
    """
    return isinstance(arg, MatrixWorkspace)

def __is_array_of_workspaces(arg):
    """
        Is the argument a sequence of Mantid MatrixWorkspaces?
        @param arg :: argument
        
        Returns :: True if the argument is a sequence of  MatrixWorkspace
    """
    return __is_array(arg) and len(arg) > 0 and __is_workspace(arg[0])


def __plot_as_workspace(workspaces):
    """
        plotSpectrum via qti plotting framework to plot a workspace.
        @param workspaces :: workspace or list of workspaces
        
        Returns :: A plot handle
    """
    return mantidplot.plotSpectrum(workspaces, 0)

def __plot_as_workspaces(workspaces):
    """
        Plot a series of workspaces
        @param workspaces :: workspaces to plot
        
        Returns :: A plot handle
    """
    # plotSpectrum can already handle 1 or more input workspaces.
    return __plot_as_workspace(workspaces)
        

def __plot_as_array(y, *args, **kwargs):
    """
        Plot from an array
        @param y :: y array
        @param Args :: x array
        @param Kwargs :: Matplot lib style options
        
        Returns :: A plot handle
    """
    if len(args) != 0:
        if __is_array(args):
            ws = CreateWorkspace(DataX=args, DataY=y)     
        else:
            raise ValueError("Inputs are of type: " + str(type(args)) + ". Not plottable." )
    else:
        x = range(0, len(y), 1) # 0 to n, incremented by 1.
        ws = CreateWorkspace(DataX=x, DataY=y)   
    return __plot_as_workspace(ws)



def plot (y, *args, **kwargs):
    """
        Plot the data.
        @param y :: argument
        @param args :: curve inputs
        @param kwargs :: plot line options
        
        Returns :: A plot handle.
    """
    print type(y)
    if __is_array(y):
        if __is_array_of_workspaces(y):
            return __plot_as_workspaces(y)
        else:
            return __plot_as_array(y)
    elif __is_workspace(y):
        return __plot_as_workspace(y)
    else:
        raise ValueError("Cannot plot argument of type " + str(type(y)) )
         

