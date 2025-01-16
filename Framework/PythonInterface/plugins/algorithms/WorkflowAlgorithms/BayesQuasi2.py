# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name,too-many-instance-attributes,too-many-branches,no-init,redefined-builtin

from mantid.api import AlgorithmFactory, MatrixWorkspaceProperty, PropertyMode, Progress
from mantid.kernel import StringListValidator, Direction
from mantid.utils.pip import package_installed
from mantid import logger
from IndirectCommon import get_two_theta_and_q

from typing import Dict
from numpy import ndarray
import numpy as np

from quickBayesHelper import QuickBayesTemplate


class BayesQuasi2(QuickBayesTemplate):
    def summary(self):
        return "This algorithm uses the Python quickBayes package to fit the quasielastic data (Lorentzians or stretched exponential)."

    def category(self):
        return "Workflow\\MIDAS"

    def version(self):
        return 1

    def PyInit(self):
        self.declareProperty(
            name="Program",
            defaultValue="QL",
            validator=StringListValidator(["QL", "QSe"]),
            doc="The type of program to run (either QL or QSe)",
        )
        super().PyInit()
        self.declareProperty(
            MatrixWorkspaceProperty("OutputWorkspaceResult", "", optional=PropertyMode.Optional, direction=Direction.Output),
            doc="The name of the result output workspaces",
        )

        self.declareProperty(
            MatrixWorkspaceProperty("OutputWorkspaceProb", "", optional=PropertyMode.Optional, direction=Direction.Output),
            doc="The name of the probability output workspaces",
        )

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
        errors = list(engine._e_data)
        axis_names = ["data"]

        for j in range(max_features):
            x_data, fit, e, diff_fit, diff_e = engine.get_fit_values(j)

            y += list(fit) + list(diff_fit)
            x += list(x_data) + list(x_data)
            errors += list(e) + list(diff_e)
            axis_names.append(f"fit {j + 1}")
            axis_names.append(f"diff {j + 1}")
        ws = self.create_ws(
            OutputWorkspace=name,
            DataX=np.array(x),
            DataY=np.array(y),
            NSpec=len(axis_names),
            UnitX=x_unit,
            YUnitLabel="",
            VerticalAxisUnit="Text",
            VerticalAxisValues=axis_names,
            DataE=np.array(errors),
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
        name_params: str,
        name_prob: str,
    ):
        """
        Takes the output of quickBayes and makes Mantid workspaces
        :param results: dict of quickBayes parameter results
        :param results_errors: dict of quickBayes parameter errors
        :param x_data: the x data for plotting the results (e.g. Q)
        :param x_unit: the x unit
        :param max_features: the maximum number of features used
        :param name_params: the name of parameters workspace
        :param name_prob: the name of the probability worksapce
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

        _ = self.create_ws(
            OutputWorkspace=name_params,
            DataX=np.array(x_data),
            DataY=np.array(y_data),
            DataE=np.array(e_data),
            NSpec=len(axis_names),
            UnitX=x_unit,
            YUnitLabel="",
            VerticalAxisUnit="Text",
            VerticalAxisValues=axis_names,
        )

        _ = self.create_ws(
            OutputWorkspace=name_prob,
            DataX=np.array(x_data),
            DataY=np.array(prob),
            NSpec=max_features,
            UnitX=x_unit,
            YUnitLabel="",
            VerticalAxisUnit="Text",
            VerticalAxisValues=[f"{k + 1} feature(s)" for k in range(max_features)],
        )
        return name_params, name_prob

    def calculate(self, sample_ws, report_progress, res_list, N, max_num_peaks, method, function):
        name = self.getPropertyValue("SampleWorkspace")
        prog = self.getProperty("Program").value
        # get inputs
        elastic = self.getProperty("Elastic").value
        BG_str = self.getPropertyValue("Background")
        BG = self.get_background_function(BG_str)
        start_x = self.getProperty("EMin").value
        end_x = self.getProperty("EMax").value
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
            new_x, ry = workflow.preprocess_data(
                x_data=sample["x"], y_data=sample["y"], e_data=sample["e"], start_x=start_x, end_x=end_x, res=res_list[spec]
            )

            # setup fit function
            func = function(bg_function=BG, elastic_peak=elastic, r_x=new_x, r_y=ry, start_x=start_x, end_x=end_x)
            lower, upper = func.get_bounds()

            params = init_params if init_params is not None else func.get_guess()
            # just want a guess the same length as lower, it is not used
            workflow.set_scipy_engine(guess=func.get_guess(), lower=lower, upper=upper)

            # do the calculation
            workflow.execute(max_num_features=max_num_peaks, func=func, params=params)

            results, results_errors = workflow.get_parameters_and_errors

            init_params = func.read_from_report(report_dict=results, N=1, index=-1)

            engine = workflow.fit_engine

            ws_list = self.make_fit_ws(
                engine=engine, max_features=max_num_peaks, ws_list=ws_list, x_unit="DeltaE", name=f"{name}_{prog}_Workspace_{spec}"
            )

        sample_logs = [("background", BG_str), ("elastic_peak", elastic), ("energy_min", start_x), ("energy_max", end_x)]

        return ws_list, results, results_errors, sample_logs

    # Cannot make static as it prevents it being mocked later
    def QLData(self):
        from quickBayes.workflow.QlData import QLData

        return QLData

    def QlDataFunction(self):
        from quickBayes.functions.qldata_function import QlDataFunction

        return QlDataFunction

    def QSEFunction(self):
        from quickBayes.functions.qse_function import QSEFunction

        return QSEFunction

    def QlStretchedExp(self):
        from quickBayes.workflow.QSE import QlStretchedExp

        return QlStretchedExp

    def get_background_function(self, bg_str):
        from quickBayes.utils.general import get_background_function

        return get_background_function(bg_str)

    def PyExec(self):
        if not package_installed("quickBayes", show_warning=True):
            raise RuntimeError("Please install 'quickBayes' missing dependency")

        self.log().information("BayesQuasi input")
        program = self.getPropertyValue("Program")

        # get sample data
        name = self.getPropertyValue("SampleWorkspace")
        sample_ws, N = self.point_data(name=name)

        # get resolution data
        res_name = self.getPropertyValue("ResolutionWorkspace")
        res_ws, N_res_hist = self.point_data(name=res_name)

        # setup
        Q = get_two_theta_and_q(sample_ws)
        report_progress = Progress(self, start=0.0, end=1.0, nreports=N)

        # do calculation
        if program == "QL":
            max_num_peaks = 3
            if N_res_hist == 1:
                prog = "QLr"  # res file
                res_list = self.duplicate_res(res_ws=res_ws, N=N)
            elif N_res_hist == N:
                prog = "QLd"  # data file
                res_list = self.unique_res(res_ws=res_ws, N=N)
            else:
                raise ValueError("RES file needs to have either 1 or the same number of histograms as sample.")
            ws_list, results, results_errors, sample_logs = self.calculate(
                sample_ws=sample_ws,
                report_progress=report_progress,
                res_list=res_list,
                N=N,
                max_num_peaks=max_num_peaks,
                method=self.QLData(),
                function=self.QlDataFunction(),
            )

        elif program == "QSe":
            max_num_peaks = 1
            if N_res_hist == 1:
                prog = "QSe"  # res file
                res_list = self.duplicate_res(res_ws=res_ws, N=N)
            elif N_res_hist == N:
                prog = "QSe"  # data file
                res_list = self.unique_res(res_ws=res_ws, N=N)
            else:
                raise ValueError("RES file needs to have either 1 or the same number of histograms as sample.")
            ws_list, results, results_errors, sample_logs = self.calculate(
                sample_ws=sample_ws,
                report_progress=report_progress,
                res_list=res_list,
                N=N,
                max_num_peaks=max_num_peaks,
                method=self.QlStretchedExp(),
                function=self.QSEFunction(),
            )

        sample_logs.append(("res_workspace", res_name))
        sample_logs.append(("fit_program", prog))

        # report results
        fits = self.group_ws(ws_list=ws_list, name=self.getPropertyValue("OutputWorkspaceFit"))

        self.setProperty("OutputWorkspaceFit", fits)
        self.add_sample_logs(workspace=fits, sample_logs=sample_logs, data_ws=sample_ws)

        params, prob = self.make_results(
            results=results,
            results_errors=results_errors,
            x_data=Q[1],
            x_unit="MomentumTransfer",
            max_features=max_num_peaks,
            name_params=self.getPropertyValue("OutputWorkspaceResult"),
            name_prob=self.getPropertyValue("OutputWorkspaceProb"),
        )
        self.add_sample_logs(workspace=params, sample_logs=sample_logs, data_ws=sample_ws)

        self.setProperty("OutputWorkspaceResult", params)
        self.setProperty("OutputWorkspaceProb", prob)


# Register algorithm with Mantid
AlgorithmFactory.subscribe(BayesQuasi2)
