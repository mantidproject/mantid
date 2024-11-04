# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name
from mantid.api import mtd, AlgorithmFactory, PythonAlgorithm, WorkspaceGroup
import mantid.simpleapi as ms
import numpy as np


class SortByQVectors(PythonAlgorithm):
    """
    Sorts spectra from a workspace

    """

    def category(self):
        return "Transforms\\Merging;Utility\\Sorting"

    def seeAlso(self):
        return ["SortDetectors"]

    def name(self):
        return "SortByQVectors"

    def summary(self):
        return "This algorithm sorts a group workspace by the qvectors found in the qvectors file."

    def PyInit(self):
        self.declareProperty("InputWorkspace", "", "Group workspace that automatically includes all members.")

    # pylint: disable=too-many-branches
    def PyExec(self):
        # get parameter values
        wsString = self.getPropertyValue("InputWorkspace").strip()
        # internal values
        wsOutput = "__OutputWorkspace"
        wsTemp = "__Sort_temp"
        # get the workspace list
        wsNames = []
        for wsName in wsString.split(","):
            ws = mtd[wsName.strip()]
            if isinstance(ws, WorkspaceGroup):
                wsNames.extend(ws.getNames())
            else:
                wsNames.append(wsName)

        if wsOutput in mtd:
            ms.DeleteWorkspace(Workspace=wsOutput)
        sortStat = []
        for wsName in wsNames:
            if "qvectors" in wsName:
                # extract the spectrum
                ws = mtd[wsName.strip()]
                for s in range(0, ws.getNumberHistograms()):
                    y_s = ws.readY(s)
                    stuple = (self.GetXValue(y_s), s)
                    sortStat.append(stuple)
                sortStat.sort()
        if len(sortStat) == 0:
            raise RuntimeError("Cannot find file with qvectors, aborting")
        # sort spectra using norm of q
        for wsName in wsNames:
            ws = mtd[wsName.strip()]
            yUnit = ws.getAxis(1).getUnit().unitID()
            transposed = False
            if ws.getNumberHistograms() < len(sortStat):
                ms.Transpose(InputWorkspace=wsName, OutputWorkspace=wsName)
                transposed = True
            for norm, spec in sortStat:
                ms.ExtractSingleSpectrum(InputWorkspace=wsName, OutputWorkspace=wsTemp, WorkspaceIndex=spec)
                if wsOutput in mtd:
                    ms.ConjoinWorkspaces(InputWorkspace1=wsOutput, InputWorkspace2=wsTemp, CheckOverlapping=False)
                    if wsTemp in mtd:
                        ms.DeleteWorkspace(Workspace=wsTemp)
                else:
                    ms.RenameWorkspace(InputWorkspace=wsTemp, OutputWorkspace=wsOutput)

            # put norm as y value and copy units from input
            loopIndex = 0
            wsOut = mtd[wsOutput]
            for norm, spec in sortStat:
                wsOut.getSpectrum(loopIndex).setSpectrumNo(int(norm * 1000))
                loopIndex = loopIndex + 1
            if len(yUnit) > 0:
                wsOut.getAxis(1).setUnit(yUnit)
            if transposed:
                ms.Transpose(InputWorkspace=wsOutput, OutputWorkspace=wsOutput)
            ms.RenameWorkspace(InputWorkspace=wsOutput, OutputWorkspace=wsName)

    def GetXValue(self, xs):
        return np.linalg.norm(xs)


AlgorithmFactory.subscribe(SortByQVectors)
