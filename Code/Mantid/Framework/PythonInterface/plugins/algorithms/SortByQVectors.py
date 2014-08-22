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

    def summary(self):
        return "This algorithm sorts a group workspace by the qvectors found in the qvectors file."

    def PyInit(self):
        self.declareProperty("InputWorkspace", "", "Group workspace that automatically includes all members.")

    def PyExec(self):
        # get parameter values
        wsString = self.getPropertyValue("InputWorkspace").strip()
        #internal values
        wsOutput = "__OutputWorkspace"
        wsTemp = "__Sort_temp"
        #get the workspace list
        wsNames = []
        for wsName in wsString.split(","):
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
                for s in range(0, ws.getNumberHistograms()):
                    y_s = ws.readY(s)
                    tuple = (self.GetXValue(y_s), s)
                    sortStat.append(tuple)
                sortStat.sort()
        if len(sortStat) == 0:
            raise RuntimeError("Cannot find file with qvectors, aborting")
        #sort spectra using norm of q
        for wsName in wsNames:
            ws = mtd[wsName.strip()]
            yUnit = ws.getAxis(1).getUnit().unitID()
            transposed = False
            if ws.getNumberHistograms() < len(sortStat):
                Transpose(InputWorkspace=wsName, OutputWorkspace=wsName)
                transposed = True
            for norm, spec in sortStat:
                ExtractSingleSpectrum(InputWorkspace=wsName, OutputWorkspace=wsTemp, WorkspaceIndex=spec)
                if wsOutput in mtd:
                    ConjoinWorkspaces(InputWorkspace1=wsOutput,InputWorkspace2=wsTemp,CheckOverlapping=False)
                    if wsTemp in mtd:
                        DeleteWorkspace(Workspace=wsTemp)
                else:
                    RenameWorkspace(InputWorkspace=wsTemp, OutputWorkspace=wsOutput)

            #put norm as y value and copy units from input
            loopIndex = 0
            wsOut = mtd[wsOutput]
            for norm, spec in sortStat:
                wsOut.getSpectrum(loopIndex).setSpectrumNo(int(norm*1000))
                loopIndex = loopIndex + 1
            if len(yUnit) > 0:
                wsOut.getAxis(1).setUnit(yUnit)
            if transposed:
                Transpose(InputWorkspace=wsOutput, OutputWorkspace=wsOutput)
            RenameWorkspace(InputWorkspace=wsOutput, OutputWorkspace=wsName)


    def GetXValue(self, xs):
        return np.linalg.norm(xs)

AlgorithmFactory.subscribe(SortByQVectors)
