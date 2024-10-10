# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name
import numpy

from mantid.api import mtd, AlgorithmFactory, PythonAlgorithm, MatrixWorkspace, MatrixWorkspaceProperty
from mantid.kernel import Direction, StringMandatoryValidator
from mantid.simpleapi import CloneWorkspace


class Mean(PythonAlgorithm):
    def category(self):
        return "Arithmetic"

    def seeAlso(self):
        return ["MostLikelyMean", "WeightedMean", "WeightedMeanOfWorkspace"]

    def name(self):
        return "Mean"

    def summary(self):
        return "Calculates the arithemetic mean of the workspaces provided."

    def PyInit(self):
        mustHaveWorkspaceNames = StringMandatoryValidator()

        self.declareProperty(
            "Workspaces",
            "",
            validator=mustHaveWorkspaceNames,
            direction=Direction.Input,
            doc="Input workspaces. Comma separated workspace names",
        )

        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", "", Direction.Output), doc="Output mean workspace")

    def validateInputs(self):
        issues = dict()
        workspaces = self.getProperty("Workspaces").value.split(",")
        name = workspaces[0].strip()

        validation_message = self._validate_workspace_type(name)
        if validation_message != "":
            issues["Workspaces"] = validation_message
            return issues

        ws1 = mtd[name]
        nSpectra = ws1.getNumberHistograms()

        for index in range(1, len(workspaces)):
            name = workspaces[index].strip()

            validation_message = self._validate_workspace_type(name)
            if validation_message != "":
                issues["Workspaces"] = validation_message
                return issues

            ws2 = mtd[name]
            if not self._are_workspaces_compatible(ws1, ws2):
                issues["Workspaces"] = "Input Workspaces are not the same shape."
                # cannot run the next test if this fails
                return issues
            for spectra in range(0, nSpectra):
                if not numpy.allclose(ws1.readX(spectra), ws2.readX(spectra)):
                    issues["Workspaces"] = "The data should have the same order for x values. Sort your data first"
        return issues

    def _validate_workspace_type(self, workspace_name):
        if workspace_name not in mtd:
            return f"Workspace '{workspace_name}' does not exist"
        if not isinstance(mtd[workspace_name], MatrixWorkspace):
            return f"Workspace '{workspace_name}' is not a MatrixWorkspace"
        return ""

    def _are_workspaces_compatible(self, ws_a, ws_b):
        match_bins = ws_a.blocksize() == ws_b.blocksize()
        match_histograms = ws_a.getNumberHistograms() == ws_b.getNumberHistograms()
        return match_bins and match_histograms

    def PyExec(self):
        workspaces = self.getProperty("Workspaces").value.split(",")
        out_ws = CloneWorkspace(InputWorkspace=mtd[workspaces[0]], OutputWorkspace=self.getPropertyValue("OutputWorkspace"))
        for index in range(1, len(workspaces)):
            name = workspaces[index].strip()
            workspace = mtd[name]
            out_ws += workspace
        out_ws /= len(workspaces)
        self.setProperty("OutputWorkspace", out_ws)


AlgorithmFactory.subscribe(Mean())
