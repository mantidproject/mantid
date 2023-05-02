# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name,too-many-instance-attributes,too-many-branches,no-init,redefined-builtin

from mantid.api import PythonAlgorithm, AlgorithmFactory, MatrixWorkspaceProperty, PropertyMode, WorkspaceGroupProperty, Progress
from mantid.kernel import StringListValidator, Direction
import mantid.simpleapi as s_api
from mantid import logger
from IndirectCommon import GetThetaQ, CheckHistZero, CheckHistSame

from typing import Dict, List
from numpy import ndarray

try:
    from quickBayes.fitting.fit_engine import FitEngine
except (Exception, Warning):
    import subprocess

    print(
        subprocess.Popen(
            "python -m pip install -U quickBayes==1.0.0b7",
            shell=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            stdin=subprocess.PIPE,
        ).communicate()
    )
    from quickBayes.fitting.fit_engine import FitEngine
from quickBayes.functions.qldata_function import QlDataFunction
from quickBayes.utils.general import get_background_function
from quickBayes.workflow.QlData import QLData
from quickBayes.functions.qse_function import QSEFunction
from quickBayes.workflow.QSE import QlStretchedExp
import numpy as np


class BayesQuasi2(PythonAlgorithm):
    def category(self):
        return "Workflow\\MIDAS"

    def summary(self):
        return "This algorithm uses the Python quickBayes package to fit the quasielastic data (Lorentzians or stretched exponential)."

    def version(self):
        return 1

    def PyInit(self):
        self.declareProperty(
            name="Program",
            defaultValue="QL",
            validator=StringListValidator(["QL", "QSe"]),
            doc="The type of program to run (either QL or QSe)",
        )

        self.declareProperty(
            MatrixWorkspaceProperty("SampleWorkspace", "", direction=Direction.Input), doc="Name of the Sample input Workspace"
        )

        self.declareProperty(
            MatrixWorkspaceProperty("ResolutionWorkspace", "", direction=Direction.Input), doc="Name of the resolution input Workspace"
        )

        self.declareProperty(name="MinRange", defaultValue=-0.2, doc="The start of the fit range. Default=-0.2")

        self.declareProperty(name="MaxRange", defaultValue=0.2, doc="The end of the fit range. Default=0.2")

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
        self.declareProperty(
            MatrixWorkspaceProperty("OutputWorkspaceResult", "", direction=Direction.Output), doc="The name of the result output workspaces"
        )

        self.declareProperty(
            MatrixWorkspaceProperty("OutputWorkspaceProb", "", optional=PropertyMode.Optional, direction=Direction.Output),
            doc="The name of the probability output workspaces",
        )

    def validateInputs(self):
        start_x = self.getProperty("MinRange").value
        end_x = self.getProperty("MaxRange").value
        issues = dict()

        # Validate fitting range in energy
        if start_x > end_x:
            issues["MaxRange"] = "Must be less than EnergyMin"

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
        alg = self.createChildAlgorithm("GroupWorkspaces", enableLogging=False)
        alg.setAlwaysStoreInADS(True)
        alg.setProperty("InputWorkspaces", ws_list)
        alg.setProperty("OutputWorkspace", f"{name}_workspaces")
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

    def make_fit_ws(self, engine, max_features, ws_list, x_unit, name):
        """
        Simple function for creating a fit ws
        :param engine: the quickBayes fit engine used
        :param max_features: the maximum number of features (e.g. lorentzians)
        :param ws_list: list of fit workspaces (inout)
        :param x_unit: the x axis unit
        :param name: name of the output
        :return the list of fitting workspaces
        """
        x = list(engine._x_data)
        y = list(engine._y_data)
        axis_names = ["data"]
        for j in range(max_features):
            print(j)
            x_data, fit, e, df, de = engine.get_fit_values(j)

            y += list(fit) + list(df)
            x += list(x_data) + list(x_data)
            axis_names.append(f"fit {j+1}")
            axis_names.append(f"diff {j+1}")
            ws = self.create_ws(
                OutputWorkspace=f"{name}_workspace",
                DataX=np.array(x),
                DataY=np.array(y),
                NSpec=len(axis_names),
                UnitX=x_unit,
                YUnitLabel="",
                VerticalAxisUnit="Text",
                VerticalAxisValues=axis_names,
            )
        ws_list.append(ws)
        return ws_list

    def make_results(
        self,
        results: Dict["str", ndarray],
        results_errors: Dict["str", ndarray],
        x_data: ndarray,
        x_unit: str,
        max_features: int,
        name: str,
    ):
        """
        Takes the output of quickBayes and makes Mantid workspaces
        :param results: dict of quickBayes parameter results
        :param results_errors: dict of quickBayes parameter errors
        :param x_data: the x data for plotting the results (e.g. Q)
        :param x_unit: the x unit
        :param max_features: the maximum number of features used
        :param name: the name of the output worksapce
        :return workspace of fit paramters and workspace of loglikelihoods (probs)
        """
        axis_names = []
        y_data = []
        e_data = []
        prob = []

        for key in results.keys():
            if "loglikelihood" in key:
                prob += list(results[key])
            else:
                y_data.append(results[key])
                e_data.append(results_errors[key])
                axis_names.append(key)

        params = self.create_ws(
            OutputWorkspace=f"{name}_results",
            DataX=np.array(x_data),
            DataY=np.array(y_data),
            DataE=np.array(e_data),
            NSpec=len(axis_names),
            UnitX=x_unit,
            YUnitLabel="",
            VerticalAxisUnit="Text",
            VerticalAxisValues=axis_names,
        )

        prob_ws = self.create_ws(
            OutputWorkspace=f"{name}_prob",
            DataX=np.array(x_data),
            DataY=np.array(prob),
            NSpec=max_features,
            UnitX=x_unit,
            YUnitLabel="",
            VerticalAxisUnit="Text",
            VerticalAxisValues=[f"{k + 1} feature(s)" for k in range(max_features)],
        )
        return f"{name}_results", f"{name}_prob"

    def calculate(self, sample_ws, report_progress, res_list, N, max_num_peaks, method, function):
        name = self.getPropertyValue("SampleWorkspace")
        # get inputs
        elastic = self.getProperty("Elastic").value
        BG_str = self.getPropertyValue("Background")
        BG = get_background_function(BG_str)
        start_x = self.getProperty("MinRange").value
        end_x = self.getProperty("MaxRange").value
        # work around for bug
        if start_x < sample_ws.readX(0)[0]:
            start_x = sample_ws.readX(0)[0]
        if end_x > sample_ws.readX(0)[-1]:
            end_x = sample_ws.readX(0)[-1]

        logger.information(" Number of spectra = {0} ".format(N))
        logger.information(" Erange : {0}  to {1} ".format(start_x, end_x))

        # initial values
        init_params = None
        results = {}
        results_errors = {}
        ws_list = []

        # calculation
        for spec in range(N):
            report_progress.report(f"spectrum {spec}")
            sx = sample_ws.readX(spec)
            sy = sample_ws.readY(spec)
            se = sample_ws.readE(spec)

            sample = {"x": sx, "y": sy, "e": se}

            workflow = method(results, results_errors)
            new_x, ry = workflow.preprocess_data(sample["x"], sample["y"], sample["e"], start_x, end_x, res_list[spec])

            # setup fit function
            func = function(BG, elastic, new_x, ry, start_x, end_x)
            lower, upper = func.get_bounds()

            params = init_params if init_params is not None else func.get_guess()
            # just want a guess the same length as lower, it is not used
            workflow.set_scipy_engine(func.get_guess(), lower, upper)

            # do the calculation
            workflow.execute(max_num_peaks, func, params)
            results, results_errors = workflow.get_parameters_and_errors

            init_params = func.read_from_report(results, 1, -1)

            engine = workflow.fit_engine

            ws_list = self.make_fit_ws(engine, max_num_peaks, ws_list, "DeltaE", f"{name}_{spec}_")

        sample_logs = [("background", BG_str), ("elastic_peak", elastic), ("energy_min", start_x), ("energy_max", end_x)]

        return ws_list, results, results_errors, sample_logs

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
        self.log().information("BayesQuasi input")
        program = self.getPropertyValue("Program")

        # get sample data
        name = self.getPropertyValue("SampleWorkspace")
        sample_ws, N = self.point_data(name)

        # get resolution data
        res_name = self.getPropertyValue("ResolutionWorkspace")
        res_ws, N_res_hist = self.point_data(res_name)

        # setup
        Q = GetThetaQ(sample_ws)
        report_progress = Progress(self, start=0.0, end=1.0, nreports=N)

        # do calculation
        if program == "QL":
            max_num_peaks = 3
            if N_res_hist == 1:
                prog = "QLr"  # res file
                res_list = self.duplicate_res(res_ws, N)
            elif N_res_hist == N:
                prog = "QLd"  # data file
                res_list = self.unique_res(res_ws, N)
            else:
                raise ValueError("RES file needs to have either 1 or the same number of histograms as sample.")
            ws_list, results, results_errors, sample_logs = self.calculate(
                sample_ws, report_progress, res_list, N, max_num_peaks, QLData, QlDataFunction
            )

        elif program == "QSe":
            max_num_peaks = 1
            if N_res_hist == 1:
                prog = "QSe"  # res file
                res_list = self.duplicate_res(res_ws, N)
            else:
                raise ValueError("Stretched Exp ONLY works with RES file")
            ws_list, results, results_errors, sample_logs = self.calculate(
                sample_ws, report_progress, res_list, N, max_num_peaks, QlStretchedExp, QSEFunction
            )

        sample_logs.append(("res_workspace", res_name))
        sample_logs.append(("fit_program", prog))

        # report results
        fits = self.group_ws(ws_list, name)
        self.setProperty("OutputWorkspaceFit", fits)
        self.add_sample_logs(fits, sample_logs, sample_ws)

        params, prob = self.make_results(results, results_errors, Q[1], "MomentumTransfer", max_num_peaks, name)
        self.add_sample_logs(params, sample_logs, sample_ws)

        self.setProperty("OutputWorkspaceResult", params)
        self.setProperty("OutputWorkspaceProb", prob)


# Register algorithm with Mantid
AlgorithmFactory.subscribe(BayesQuasi2)
