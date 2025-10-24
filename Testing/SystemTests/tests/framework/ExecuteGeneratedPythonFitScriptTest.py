# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init
from systemtesting import MantidSystemTest
from mantid.api import AnalysisDataService
from mantid.simpleapi import GeneratePythonFitScript, Load


class ExecuteGeneratedPythonFitScriptTest(MantidSystemTest):
    """
    This test will execute a python script for sequential fitting, and simultaneous fitting, to make sure that the
    generated scripts will run without an error.
    """

    def setUp(self):
        Load(Filename="MUSR62260_Group_fwd_Asymmetry_MA.nxs", OutputWorkspace="MUSR62260; Group; fwd; Asymmetry; MA")
        Load(Filename="MUSR62260_Group_bottom_Asymmetry_MA.nxs", OutputWorkspace="MUSR62260; Group; bottom; Asymmetry; MA")
        Load(Filename="MUSR62260_Group_top_Asymmetry_MA.nxs", OutputWorkspace="MUSR62260; Group; top; Asymmetry; MA")
        Load(Filename="MUSR62260_Group_bkwd_Asymmetry_MA.nxs", OutputWorkspace="MUSR62260; Group; bkwd; Asymmetry; MA")

    def cleanup(self):
        AnalysisDataService.clear()

    def requiredFiles(self):
        return [
            "MUSR62260_Group_fwd_Asymmetry_MA.nxs",
            "MUSR62260_Group_bottom_Asymmetry_MA.nxs",
            "MUSR62260_Group_top_Asymmetry_MA.nxs",
            "MUSR62260_Group_bkwd_Asymmetry_MA.nxs",
        ]

    def runTest(self):
        single_fit_script_text = self._generate_single_fit_script()
        self._run_fit_script(single_fit_script_text)

        sequential_script_text = self._generate_sequential_fit_script()
        self._run_fit_script(sequential_script_text)

        simultaneous_script_text = self._generate_simultaneous_fit_script()
        self._run_fit_script(simultaneous_script_text)

    @staticmethod
    def _generate_single_fit_script():
        function = "name=GausOsc,A=0.2,Sigma=0.2,Frequency=1.3,Phi=0"

        script_text = GeneratePythonFitScript(
            InputWorkspaces=["MUSR62260; Group; fwd; Asymmetry; MA"],
            WorkspaceIndices=[0],
            StartXs=[0.1],
            EndXs=[15.0],
            Function=function,
            MaxIterations=500,
            Minimizer="Levenberg-Marquardt",
        )
        return script_text

    @staticmethod
    def _generate_sequential_fit_script():
        function = "name=GausOsc,A=0.2,Sigma=0.2,Frequency=1.3,Phi=0"

        script_text = GeneratePythonFitScript(
            InputWorkspaces=[
                "MUSR62260; Group; fwd; Asymmetry; MA",
                "MUSR62260; Group; bottom; Asymmetry; MA",
                "MUSR62260; Group; top; Asymmetry; MA",
                "MUSR62260; Group; bkwd; Asymmetry; MA",
            ],
            WorkspaceIndices=[0, 0, 0, 0],
            StartXs=[0.1, 0.1, 0.1, 0.1],
            EndXs=[15.0, 15.0, 15.0, 15.0],
            Function=function,
            MaxIterations=500,
            Minimizer="Levenberg-Marquardt",
        )
        return script_text

    @staticmethod
    def _generate_simultaneous_fit_script():
        function = (
            "composite=MultiDomainFunction,NumDeriv=true;"
            "name=GausOsc,A=0.2,Sigma=0.2,Frequency=1.3,Phi=0,$domains=i;"
            "name=GausOsc,A=0.2,Sigma=0.2,Frequency=1.3,Phi=0,$domains=i;"
            "name=GausOsc,A=0.2,Sigma=0.2,Frequency=1.3,Phi=0,$domains=i;"
            "name=GausOsc,A=0.2,Sigma=0.2,Frequency=1.3,Phi=0,$domains=i;"
            "ties=(f2.Frequency=f3.Frequency,f1.Frequency=f3.Frequency,f0.Frequency=f3.Frequency)"
        )

        script_text = GeneratePythonFitScript(
            InputWorkspaces=[
                "MUSR62260; Group; fwd; Asymmetry; MA",
                "MUSR62260; Group; bottom; Asymmetry; MA",
                "MUSR62260; Group; top; Asymmetry; MA",
                "MUSR62260; Group; bkwd; Asymmetry; MA",
            ],
            WorkspaceIndices=[0, 0, 0, 0],
            StartXs=[0.1, 0.1, 0.1, 0.1],
            EndXs=[15.0, 15.0, 15.0, 15.0],
            FittingType="Simultaneous",
            Function=function,
            MaxIterations=500,
            Minimizer="Levenberg-Marquardt",
        )
        return script_text

    def _run_fit_script(self, script_text):
        try:
            exec(script_text)  # noqa: S102
        except Exception as ex:
            self.fail(f"Execution of python fit script failed: {ex}.")
