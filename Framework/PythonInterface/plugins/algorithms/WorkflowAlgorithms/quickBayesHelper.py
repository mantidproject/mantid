# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.api import PythonAlgorithm, MatrixWorkspaceProperty, WorkspaceGroupProperty
from mantid.kernel import StringListValidator, Direction

from typing import List


class QuickBayesTemplate(PythonAlgorithm):
    """
    This class is a template for algorithms that use quickBayes.
    It does assume QENS data at present.
    This allows the sharing of code between difference algorithms
    """

    def category(self):
        return "Workflow\\MIDAS"

    def summary(self):
        return "This is a template for algorithms that use the Python quickBayes package to fit data."

    def version(self):
        return 1

    def PyInit(self):
        # common properties
        self.declareProperty(
            MatrixWorkspaceProperty("SampleWorkspace", "", direction=Direction.Input), doc="Name of the Sample input Workspace"
        )

        self.declareProperty(
            MatrixWorkspaceProperty("ResolutionWorkspace", "", direction=Direction.Input), doc="Name of the resolution input Workspace"
        )

        self.declareProperty(name="EMin", defaultValue=-0.2, doc="The start of the fit range. Default=-0.2")

        self.declareProperty(name="EMax", defaultValue=0.2, doc="The end of the fit range. Default=0.2")

        self.declareProperty(name="Elastic", defaultValue=True, doc="Fit option for using the elastic peak")

        self.declareProperty(
            name="Background",
            defaultValue="Flat",
            validator=StringListValidator(["Linear", "Flat", "None"]),
            doc="Fit option for the type of background",
        )

        self.declareProperty(
            WorkspaceGroupProperty("OutputWorkspaceFit", "", direction=Direction.Output), doc="The name of the fit output workspaces"
        )

    def validateInputs(self):
        start_x = self.getProperty("EMin").value
        end_x = self.getProperty("EMax").value
        issues = dict()

        # Validate fitting range in energy
        if start_x > end_x:
            issues["EMax"] = "Must be less than EnergyMin"

        # Validate fitting range within data range
        ws = self.getProperty("SampleWorkspace").value
        data_min = ws.readX(0)[0]
        if start_x < data_min:
            issues["EMin"] = "EMin must be more than the minimum x range of the data."
        data_max = ws.readX(0)[-1]
        if end_x > data_max:
            issues["EMax"] = "EMax must be less than the maximum x range of the data"

        return issues

    def point_data(self, name):
        """
        Load data and convert to points
        :param name: the name of the workspace
        :return workspace, number of histograms
        """
        alg = self.createChildAlgorithm("ConvertToPointData", enableLogging=False)
        alg.setProperty("InputWorkspace", name)
        alg.setProperty("OutputWorkspace", name)
        alg.execute()
        ws = alg.getProperty("OutputWorkspace").value
        return ws, ws.getNumberHistograms()

    def group_ws(self, ws_list, name):
        """
        Method for creating a group workspace
        :param ws_list: a list of the workspaces that are to be in a group
        :param name: the name of the workspace group
        :returns the workspace group
        """
        alg = self.createChildAlgorithm("GroupWorkspaces", enableLogging=False)
        alg.setAlwaysStoreInADS(True)
        alg.setProperty("InputWorkspaces", ws_list)
        alg.setProperty("OutputWorkspace", name)
        alg.execute()
        return alg.getPropertyValue("OutputWorkspace")

    def add_sample_logs(self, workspace, sample_logs: List, data_ws):
        """
        Method for adding sample logs to results
        :param workspace: the workspace to add sample logs too
        :param sample_logs: new sample logs to add
        :param data_ws: the workspace to copy sample logs from
        """
        alg = self.createChildAlgorithm("CopyLogs", enableLogging=False)
        alg.setProperty("InputWorkspace", data_ws)
        alg.setProperty("OutputWorkspace", workspace)
        alg.execute()
        ws = alg.getPropertyValue("OutputWorkspace")

        alg2 = self.createChildAlgorithm("AddSampleLogMultiple", enableLogging=False)
        alg2.setProperty("Workspace", ws)
        alg2.setProperty("LogNames", [log[0] for log in sample_logs])
        alg2.setProperty("LogValues", [log[1] for log in sample_logs])
        alg2.execute()

    def create_ws(self, OutputWorkspace, DataX, DataY, NSpec, UnitX, YUnitLabel, VerticalAxisUnit, VerticalAxisValues, DataE=None):
        """
        A method to wrap the mantid CreateWorkspace algorithm
        :param OutputWorkspace: the name of the output Workspace
        :param DataX: the x data
        :param DataY: the y data
        :param NSpec: the number of spectra in x and y
        :param UnitX: the x UnitX
        :param YUnitLabel: the label for the y units
        :param VerticalAxisUnit: the unit for the vertical axis
        :param VerticalAxisValues: the values for the vertical axis
        :param DataE: the error data
        :returns the output workspace
        """
        alg = self.createChildAlgorithm("CreateWorkspace", enableLogging=False)
        alg.setAlwaysStoreInADS(True)
        alg.setProperty("OutputWorkspace", OutputWorkspace)
        alg.setProperty("DataX", DataX)
        alg.setProperty("DataY", DataY)
        alg.setProperty("NSpec", NSpec)
        alg.setProperty("UnitX", UnitX)
        alg.setProperty("YUnitLabel", YUnitLabel)
        alg.setProperty("VerticalAxisUnit", VerticalAxisUnit)
        alg.setProperty("VerticalAxisValues", VerticalAxisValues)
        if DataE is not None:
            alg.setProperty("DataE", DataE)

        alg.execute()
        return alg.getPropertyValue("OutputWorkspace")

    def duplicate_res(self, res_ws, N):
        """
        If a resolution ws has a single spectra, but we want use the same spectra repeatedly.
        So we duplicate it
        :param res_ws: the resolution workspace
        :param N: the number of histograms we need
        :return a list of repeated spectra
        """
        res_list = []
        for j in range(N):
            res_list.append({"x": res_ws.readX(0), "y": res_ws.readY(0)})
        return res_list

    def unique_res(self, res_ws, N):
        """
        If a resolution ws has multiple spectra, put them into a list for easy analysis
        :param res_ws: the resolution workspace
        :param N: the number of histograms we need
        :return a list of spectra
        """
        res_list = []
        for j in range(N):
            res_list.append({"x": res_ws.readX(j), "y": res_ws.readY(j)})
        return res_list

    def PyExec(self):
        raise NotImplementedError("This class is for shared functionality when using quickBayes. It does not have an execute")
