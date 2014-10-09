import numpy as np
from mantid.api import (MatrixWorkspace as MatrixWorkspace, AlgorithmManager as AlgorithmManager, AnalysisDataService as ADS)
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


def __plot_as_workspace(*args, **kwargs):
    """
        plotSpectrum via qti plotting framework to plot a workspace.
        @param Args :: curve data and options.
        @param Kwargs :: plot line options
        
        
        Returns :: A plot handle
    """
    return mantidplot.plotSpectrum(args[0], 0) # HACK. Hard-codes to workspace index zero.

def __plot_as_workspaces(*args, **kwargs):
    """
        Plot a series of workspaces
        @param Args :: curve data and options.
        @param Kwargs :: plot line options
        
        
        Returns :: A plot handle
    """
    # plotSpectrum can already handle 1 or more input workspaces.
    return __plot_as_workspace(*args, **kwargs)

def __create_workspace(x, y, name="dummy"):
    """
        Create a workspace. Also puts it in the ADS
        @param x :: x array
        @param y :: y array
        @param name :: workspace name
        
        Returns :: Workspace
    """    
    alg = AlgorithmManager.create("CreateWorkspace")
    alg.setChild(True) 
    alg.initialize()
    alg.setProperty("DataX", x)
    alg.setProperty("DataY", y)
    alg.setPropertyValue("OutputWorkspace", name) 
    alg.execute()
    ws = alg.getProperty("OutputWorkspace").value
    ADS.addOrReplace(name, ws) # Cannot plot a workspace that is not in the ADS
    return ws
        

def __plot_as_array(*args, **kwargs):
    """
        Plot from an array
        @param Args :: curve data and options.
        @param Kwargs :: plot line options
        
        Returns :: A plot handle
    """
    y = args[0]
    if len(args) > 1:
        if __is_array(args[1]):
            ws = __create_workspace(args[1], y)
        else:
            raise ValueError("Inputs are of type: " + str(type(args)) + ". Not plottable." )
    else:
        x = range(0, len(y), 1) # 0 to n, incremented by 1.
        ws = __create_workspace(x, y)
    return __plot_as_workspace(ws, **kwargs)




def plot (*args, **kwargs):
    """
        Plot the data.
        @param Args :: curve data and options
        @param Kwargs :: plot line options
        
        Returns :: A plot handle.
    """
    
    nargs = len(args)
    if nargs < 1:
        raise ValueError("Must provide data to plot")
    
    y = args[0]
    
    print type(y)
    if __is_array(y):
        if __is_array_of_workspaces(y):
            return __plot_as_workspaces(*args, **kwargs)
        else:
            return __plot_as_array(*args, **kwargs)
    elif __is_workspace(y):
        return __plot_as_workspace(*args, **kwargs)
    else:
        raise ValueError("Cannot plot argument of type " + str(type(y)) )
         

