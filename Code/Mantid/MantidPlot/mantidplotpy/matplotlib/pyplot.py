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
            ws = __create_workspace(args, y)
        else:
            raise ValueError("Inputs are of type: " + str(type(args)) + ". Not plottable." )
    else:
        x = range(0, len(y), 1) # 0 to n, incremented by 1.
        ws = __create_workspace(x, y)
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
         

