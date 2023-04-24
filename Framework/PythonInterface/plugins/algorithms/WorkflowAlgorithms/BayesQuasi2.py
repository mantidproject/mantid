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
from IndirectCommon import *
from IndirectCommon import GetThetaQ
from quickBayesHelper import make_fit_ws, make_results, add_sample_logs
from quickBayes.functions.qldata_function import QlDataFunction
from quickBayes.utils.general import get_background_function
from quickBayes.workflow.QlData import QLData


class BayesQuasi2(PythonAlgorithm):
    def category(self):
        return "Workflow\\MIDAS"

    def summary(self):
        return "This algorithm uses the Python quickBayes package to fit" + " the quasielastic data (Lorentzians or stretched exponential)."

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

    # pylint: disable=too-many-locals,too-many-statements
    def PyExec(self):
        self.log().information("BayesQuasi input")
        program = self.getPropertyValue("Program")

        sample_ws = self.getPropertyValue("SampleWorkspace")
        sample_ws = s_api.ConvertToPointData(sample_ws)

        res_ws = self.getPropertyValue("ResolutionWorkspace")
        res_ws = s_api.ConvertToPointData(res_ws)

        start_x = self.getProperty("MinRange").value
        end_x = self.getProperty("MaxRange").value

        if start_x < sample_ws.readX(0)[0]:
            start_x = sample_ws.readX(0)[0]
        if end_x > sample_ws.readX(0)[-1]:
            end_x = sample_ws.readX(0)[-1]

        elastic = self.getProperty("Elastic").value
        BG_str = self.getPropertyValue("Background")

        N = sample_ws.getNumberHistograms()
        report_progress = Progress(self, start=0.0, end=1.0, nreports=N)
        nres = CheckHistZero(res_ws.name())[0]

        # setup_prog.report("Checking Histograms")
        if program == "QL":
            if nres == 1:
                prog = "QLr"  # res file
            else:
                prog = "QLd"  # data file
                CheckHistSame(sample_ws.name(), "Sample", res_ws.name(), "Resolution")
        elif program == "QSe":
            if nres == 1:
                prog = "QSe"  # res file
            else:
                raise ValueError("Stretched Exp ONLY works with RES file")

        logger.information("Version is {0}".format(prog))
        logger.information(" Number of spectra = {0} ".format(N))
        logger.information(" Erange : {0}  to {1} ".format(start_x, end_x))

        # -4 to remove "_red" extension
        name = sample_ws.name()[:-4] + "_" + prog

        results_errors = {}
        results = {}
        init_params = None
        ws_list = []
        res_list = []
        max_num_peaks = 3
        BG = get_background_function(BG_str)

        if res_ws.getNumberHistograms() == 1:
            for j in range(N):
                res_list.append({"x": res_ws.readX(0), "y": res_ws.readY(0)})
        elif res_ws.getNumberHistograms() == N:
            for j in range(N):
                res_list.append({"x": res_ws.readX(j), "y": res_ws.readY(j)})
        Q = GetThetaQ(sample_ws)

        sample_logs = [
            ("res_workspace", res_ws),
            ("fit_program", prog),
            ("background", BG_str),
            ("elastic_peak", elastic),
            ("energy_min", start_x),
            ("energy_max", end_x),
        ]

        for spec in range(N):
            report_progress.report(f"spectrum {spec}")
            sx = sample_ws.readX(spec)
            sy = sample_ws.readY(spec)
            se = sample_ws.readE(spec)

            sample = {"x": sx, "y": sy, "e": se}

            workflow = QLData(results, results_errors)
            new_x, ry = workflow.preprocess_data(sample["x"], sample["y"], sample["e"], start_x, end_x, res_list[spec])

            # setup fit function
            func = QlDataFunction(BG, elastic, new_x, ry, start_x, end_x)
            lower, upper = func.get_bounds()

            params = init_params if init_params is not None else func.get_guess()
            # just want a guess the same length as lower, it is not used
            workflow.set_scipy_engine(func.get_guess(), lower, upper)

            # do the calculation
            workflow.execute(max_num_peaks, func, params)
            results, results_errors = workflow.get_parameters_and_errors

            init_params = func.read_from_report(results, 1, -1)

            engine = workflow.fit_engine

            ws_list = make_fit_ws(engine, max_num_peaks, f"{name}_{spec}_", ws_list, "DeltaE")

        fits = s_api.GroupWorkspaces(ws_list, OutputWorkspace=f"{name}_workspaces")
        add_sample_logs(fits, sample_logs, sample_ws)

        params, prob = make_results(results, results_errors, Q[1], "MomentumTransfer", max_num_peaks, name)
        add_sample_logs(params, sample_logs, sample_ws)

        self.setProperty("OutputWorkspaceFit", fits)
        self.setProperty("OutputWorkspaceResult", params)
        self.setProperty("OutputWorkspaceProb", prob)


# Register algorithm with Mantid
AlgorithmFactory.subscribe(BayesQuasi2)
