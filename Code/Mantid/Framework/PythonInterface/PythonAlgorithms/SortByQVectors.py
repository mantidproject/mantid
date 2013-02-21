"""*WIKI* 

This algorithm sorts a group workspace by the qvectors found in the qvectors file.  Workspaces will be tranformed if the qvectors dimension is in the bins.

*WIKI*"""


from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import (DeleteWorkspace, ExtractSingleSpectrum, RenameWorkspace, ConjoinWorkspaces, Transpose)
import numpy as np

import os

class SortByQVectors(PythonAlgorithm):
    """
    Sorts spectra from a workspace
           
    """
   
    def category(self):
        return "Transforms\\Merging;PythonAlgorithms"

    def name(self):
        return "SortByQVectors"

    def PyInit(self):
        self.declareProperty("InputWorkspace", "", "Group workspace that automatically includes all members.")

      
    def PyExec(self):
        # get parameter values
        wsOutput = "__OutputWorkspace"
        wsString = self.getPropertyValue("InputWorkspace").strip()
        #internal values
        wsTemp = "__ConjoinSpectra_temp"       
        #get the workspace list
        wsNames = []
        for wsName in wsString.split(","):
            print wsName
            ws = mtd[wsName.strip()]
            if type(ws) == WorkspaceGroup:
                wsNames.extend(ws.getNames())
            else:
                wsNames.append(wsName)

        if wsOutput in mtd:
            DeleteWorkspace(Workspace=wsOutput)
        sortStat = []
        for wsName in wsNames:
            if "qvectors" in wsName:     
                #extract the spectrum
                ws = mtd[wsName.strip()]
                for s in range(0,ws.getNumberHistograms()):
                    y_s = ws.readY(s)
                    tuple = (self.GetXValue(y_s), s)
                    sortStat.append(tuple)
                sortStat.sort()
        if len(sortStat) == 0:
            raise RuntimeError("Cannot find file with qvectors, aborting")
            
        for wsName in wsNames:
            ws = mtd[wsName.strip()]
            if ws.getNumberHistograms() < len(sortStat):
            	Transpose(InputWorkspace=wsName,OutputWorkspace=wsName)
            for norm, spec in sortStat:
                ExtractSingleSpectrum(InputWorkspace=wsName,OutputWorkspace=wsTemp,WorkspaceIndex=spec)
                if wsOutput in mtd:
                    ConjoinWorkspaces(InputWorkspace1=wsOutput,InputWorkspace2=wsTemp,CheckOverlapping=False)
                    if wsTemp in mtd:
                        DeleteWorkspace(Workspace=wsTemp)
                else:
                    RenameWorkspace(InputWorkspace=wsTemp,OutputWorkspace=wsOutput)
            wsOut = mtd[wsOutput]
            RenameWorkspace(InputWorkspace=wsOut,OutputWorkspace=wsName)
        
    def GetXValue(self,xs):
        return np.linalg.norm(xs)
        
registerAlgorithm(SortByQVectors)
