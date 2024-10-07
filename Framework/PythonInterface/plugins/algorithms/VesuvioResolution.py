# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init
from mantid.api import *
from mantid.kernel import *
from vesuvio.base import VesuvioBase


class VesuvioResolution(VesuvioBase):
    _workspace_index = None
    _mass = None

    def category(self):
        return "Inelastic\\Indirect\\Vesuvio"

    def summary(self):
        return "Calculates the resolution function for VESUVIO"

    def PyInit(self):
        self.declareProperty(
            MatrixWorkspaceProperty(name="Workspace", defaultValue="", direction=Direction.Input), doc="Sample matrix workspace"
        )

        self.declareProperty(name="WorkspaceIndex", defaultValue=0, doc="Workspace index to use for resolution")

        self.declareProperty(name="Mass", defaultValue=100.0, doc="The mass defining the recoil peak in AMU")

        self.declareProperty(
            WorkspaceProperty(name="OutputWorkspaceTOF", defaultValue="", direction=Direction.Output, optional=PropertyMode.Optional),
            doc="Output resolution workspace in TOF",
        )

        self.declareProperty(
            WorkspaceProperty(name="OutputWorkspaceYSpace", defaultValue="", direction=Direction.Output, optional=PropertyMode.Optional),
            doc="Output resolution workspace in ySpace",
        )

    def validateInputs(self):
        """
        Does basic validation for inputs.
        """

        issues = dict()

        sample_ws = self.getProperty("Workspace").value
        workspace_index = self.getProperty("WorkspaceIndex").value

        if not isinstance(sample_ws, MatrixWorkspace):
            issues["Workspace"] = "The Workspace must be a MatrixWorkspace"
        elif workspace_index > sample_ws.getNumberHistograms() - 1:
            issues["WorkspaceIndex"] = "Workspace index is out of range"

        out_ws_tof = self.getPropertyValue("OutputWorkspaceTOF")
        out_ws_ysp = self.getPropertyValue("OutputWorkspaceYSpace")

        output_tof = out_ws_tof != ""
        output_ysp = out_ws_ysp != ""

        if not (output_tof or output_ysp):
            warning_message = "Must output in either time of flight or ySpace"
            issues["OutputWorkspaceTOF"] = warning_message
            issues["OutputWorkspaceYSpace"] = warning_message

        return issues

    def PyExec(self):
        sample_ws = self.getProperty("Workspace").value
        out_ws_tof = self.getPropertyValue("OutputWorkspaceTOF")
        out_ws_ysp = self.getPropertyValue("OutputWorkspaceYSpace")
        self._workspace_index = self.getProperty("WorkspaceIndex").value
        self._mass = self.getProperty("Mass").value

        output_tof = out_ws_tof != ""
        output_ysp = out_ws_ysp != ""

        if output_tof:
            res_tof = self._calculate_resolution(sample_ws)
            self.setProperty("OutputWorkspaceTOF", res_tof)

        if output_ysp:
            y_space_conv = self._execute_child_alg(
                "ConvertToYSpace", return_values="OutputWorkspace", InputWorkspace=sample_ws, Mass=self._mass
            )
            res_ysp = self._calculate_resolution(y_space_conv)
            self.setProperty("OutputWorkspaceYSpace", res_ysp)

    def _calculate_resolution(self, workspace):
        """
        Calculates the resolution function using the VesuvioResolution fit function.

        @param workspace The sample workspace
        """

        function = "name=VesuvioResolution, Mass=%f" % self._mass
        fit_naming_stem = "__vesuvio_res_fit"

        # Execute the resolution function using fit.
        # Functions can't currently be executed as stand alone objects,
        # so for now we will run fit with zero iterations to achieve the same result.
        fit_ws = self._execute_child_alg(
            "Fit",
            return_values="OutputWorkspace",
            Function=function,
            InputWorkspace=workspace,
            MaxIterations=0,
            CreateOutput=True,
            Output=fit_naming_stem,
            WorkspaceIndex=self._workspace_index,
            OutputCompositeMembers=False,
        )

        # Extract just the function values from the fit spectrum
        res_ws = self._execute_child_alg("ExtractSingleSpectrum", InputWorkspace=fit_ws, WorkspaceIndex=1)
        return res_ws


AlgorithmFactory.subscribe(VesuvioResolution)
