#pylint: disable=no-init,invalid-name
from __future__ import (absolute_import, division, print_function)

import numpy

from mantid.simpleapi import *
from mantid.api import *
from mantid.kernel import *


class Mean(PythonAlgorithm):

    def category(self):
        return "Arithmetic"

    def seeAlso(self):
        return [ "MostLikelyMean","WeightedMean","WeightedMeanOfWorkspace" ]

    def name(self):
        return "Mean"

    def summary(self):
        return "Calculates the arithemetic mean of the workspaces provided."

    def PyInit(self):
        mustHaveWorkspaceNames = StringMandatoryValidator()

        self.declareProperty("Workspaces", "", validator=mustHaveWorkspaceNames,
                             direction=Direction.Input,
                             doc="Input workspaces. Comma separated workspace names")

        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", "", Direction.Output),
                             doc="Output mean workspace")

    def validateInputs(self):
        issues = dict()
        workspaces = self.getProperty("Workspaces").value.split(',')
        name = workspaces[0].strip()
        ws1 = mtd[name]
        nSpectra = ws1.getNumberHistograms()

        for index in range(1, len(workspaces)):
            name = workspaces[index].strip()
            ws2 = mtd[name]
            if not self._are_workspaces_compatible(ws1, ws2):
                issues["workspaces"]="Input Workspaces are not the same shape."
                # cannot run the next test if this fails
                return issues
            for spectra in range(0,nSpectra):
                if not numpy.allclose(ws1.readX(spectra) ,ws2.readX(spectra)):
                    issues["Workspaces"] = "The data should have the same order for x values. Sort your data first"
        return issues

    def _are_workspaces_compatible(self, ws_a, ws_b):
        sizeA = ws_a.blocksize() * ws_a.getNumberHistograms()
        sizeB = ws_b.blocksize() * ws_b.getNumberHistograms()
        return sizeA == sizeB

    def PyExec(self):
        workspaces = self.getProperty("Workspaces").value.split(',')
        out_ws = CloneWorkspace(InputWorkspace=mtd[workspaces[0]],
                                OutputWorkspace=self.getPropertyValue("OutputWorkspace"))
        for index in range(1, len(workspaces)):
            name = workspaces[index].strip()
            workspace = mtd[name]
            out_ws += workspace
        out_ws /= len(workspaces)
        self.setProperty("OutputWorkspace", out_ws)


AlgorithmFactory.subscribe(Mean())
