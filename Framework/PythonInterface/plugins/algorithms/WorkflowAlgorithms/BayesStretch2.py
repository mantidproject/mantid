# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name,too-many-instance-attributes,too-many-branches,no-init
from mantid.api import AlgorithmFactory, WorkspaceGroupProperty, Progress
from mantid.kernel import Direction, IntBoundedValidator, FloatBoundedValidator
from mantid import logger
from IndirectCommon import GetThetaQ
from mantid.api import AnalysisDataService as ADS
from quickBayesHelper import QuickBayesTemplate

from functools import partial

# from typing import List
from numpy import ndarray
import numpy as np
import multiprocessing

try:
    from quickBayes.utils.parallel import parallel
except (Exception, Warning):
    import subprocess

    print(
        subprocess.Popen(
            "python -m pip install -U quickBayes==1.0.0b14",
            shell=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            stdin=subprocess.PIPE,
        ).communicate()
    )
    print(
        subprocess.Popen(
            "python -m pip install -U joblib",
            shell=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            stdin=subprocess.PIPE,
        ).communicate()
    )
    from quickBayes.utils.parallel import parallel
from quickBayes.functions.qse_fixed import QSEFixFunction
from quickBayes.utils.general import get_background_function
from quickBayes.workflow.qse_search import QSEGridSearch


class BayesStretch2(QuickBayesTemplate):
    def category(self):
        return "Workflow\\MIDAS"

    def summary(self):
        return "Creates a grid showing the variation of a stretched exponential function for different FWHM and beta values."

    def PyInit(self):
        self.declareProperty(
            name="NumberProcessors",
            defaultValue=multiprocessing.cpu_count(),
            doc="Number of cpu's to use, default is all'",
            validator=IntBoundedValidator(lower=1),
        )

        self.declareProperty(name="NumberFWHM", defaultValue=3, doc="Number of sigma values", validator=IntBoundedValidator(lower=1))

        self.declareProperty(name="NumberBeta", defaultValue=3, doc="Number of beta values", validator=IntBoundedValidator(lower=1))

        self.declareProperty(
            name="StartBeta", defaultValue=0.5, doc="Start of beta values", validator=FloatBoundedValidator(lower=0.5, upper=1.0)
        )
        self.declareProperty(
            name="EndBeta", defaultValue=1.0, doc="End of beta values", validator=FloatBoundedValidator(lower=0.5, upper=1.00001)
        )

        self.declareProperty(
            name="StartFWHM", defaultValue=0.01, doc="Start of FWHM values", validator=FloatBoundedValidator(lower=0.0, upper=1.0)
        )
        self.declareProperty(
            name="EndFWHM", defaultValue=0.1, doc="End of FWHM values", validator=FloatBoundedValidator(lower=0.0, upper=1.0)
        )

        super().PyInit()

        self.declareProperty(
            WorkspaceGroupProperty("OutputWorkspaceContour", "", direction=Direction.Output),
            doc="The name of the contour output workspaces",
        )

    def do_one_spec(self, spec, data):
        sample_ws = data["sample"]
        start_x = data["start x"]
        end_x = data["end x"]
        res_list = data["res_list"]
        # report_progress = data['report']
        beta_start = data["beta start"]
        beta_end = data["beta end"]
        N_beta = data["N_beta"]
        FWHM_start = data["FWHM start"]
        FWHM_end = data["FWHM end"]
        N_FWHM = data["N_FWHM"]
        BG = data["BG"]
        elastic = data["elastic"]
        name = data["name"]

        # report_progress.report(f"spectrum {spec}")
        sx = sample_ws.readX(spec)
        sy = sample_ws.readY(spec)
        se = sample_ws.readE(spec)

        sample = {"x": sx, "y": sy, "e": se}

        search = QSEGridSearch()
        new_x, ry = search.preprocess_data(sample["x"], sample["y"], sample["e"], start_x, end_x, res_list[spec])
        search.set_x_axis(beta_start, beta_end, N_beta, "beta")
        search.set_y_axis(FWHM_start, FWHM_end, N_FWHM, "FWHM")

        # setup fit function
        func = QSEFixFunction(BG, elastic, new_x, ry, start_x, end_x)
        func.add_single_SE()
        func.set_delta_bounds([0, -0.5], [200, 0.5])

        search.set_scipy_engine(func.get_guess(), *func.get_bounds())
        X, Y = search.execute(func)
        Z = search.get_grid
        contour = self.make_contour(X, Y, Z, spec, name)

        beta_slice, FWHM_slice = search.get_slices()
        beta = (search.get_x_axis.values, beta_slice)
        FWHM = (search.get_y_axis.values, FWHM_slice)
        return contour, beta, FWHM

    def calculate_wrapper(self, spec, data):
        return self.do_one_spec(spec, data)

    def calculate(self, sample_ws, report_progress, res_list, N):
        name = self.getPropertyValue("SampleWorkspace")
        # get inputs
        elastic = self.getProperty("Elastic").value
        BG_str = self.getPropertyValue("Background")
        BG = get_background_function(BG_str)
        start_x = self.getProperty("EMin").value
        end_x = self.getProperty("EMax").value
        numCores = self.getProperty("NumberProcessors").value
        ########################################
        beta_start = self.getProperty("StartBeta").value
        beta_end = self.getProperty("EndBeta").value
        N_beta = self.getProperty("NumberBeta").value

        FWHM_start = self.getProperty("StartFWHM").value
        FWHM_end = self.getProperty("EndFWHM").value
        N_FWHM = self.getProperty("NumberFWHM").value
        ######################################

        # work around for bug
        if start_x < sample_ws.readX(0)[0]:
            start_x = sample_ws.readX(0)[0]
        if end_x > sample_ws.readX(0)[-1]:
            end_x = sample_ws.readX(0)[-1]

        logger.information(" Number of spectra = {0} ".format(N))
        logger.information(" Erange : {0}  to {1} ".format(start_x, end_x))

        # initial values
        contour_list = []
        beta_list = []
        FWHM_list = []
        #
        data = {}
        data["sample"] = sample_ws
        data["start x"] = start_x
        data["end x"] = end_x
        data["res_list"] = res_list
        data["report"] = report_progress
        data["beta start"] = beta_start
        data["beta end"] = beta_end
        data["N_beta"] = N_beta
        data["FWHM start"] = FWHM_start
        data["FWHM end"] = FWHM_end
        data["N_FWHM"] = N_FWHM
        data["BG"] = BG
        data["elastic"] = elastic
        data["name"] = name

        # calculation
        calc = partial(self.calculate_wrapper, data=data)
        output = parallel(list(range(N)), calc, N=numCores)
        # record results
        for spec in range(N):
            contour_list.append(output[spec][0])
            beta_list.append(output[spec][1])
            FWHM_list.append(output[spec][2])

        sample_logs = [
            ("background", BG_str),
            ("elastic_peak", elastic),
            ("energy_min", start_x),
            ("energy_max", end_x),
            ("StartBeta", beta_start),
            ("EndBeta", beta_end),
            ("NumberBeta", N_beta),
            ("StartFWHM", FWHM_start),
            ("EndFWHM", FWHM_end),
            ("NumberFWHM", N_FWHM),
        ]

        return contour_list, beta_list, FWHM_list, sample_logs

    def make_contour(self, X, Y, Z, spec, name):
        """
        Create a countour workspace
        :param X: the x data for the contour (e.g. FWHM)
        :param Y: the y data for the contour (e.g. beta)
        :param Z: the z data for the contour (e.g. the cost function)
        :param spec: the spectrum number
        :param name: part of the name for the output workspace
        :returns the name of the output worksapce
        """
        x = []
        y = []
        z = []
        for j in range(len(Y)):
            x += list(X[j])
            y.append(Y[j][0])
            z += list(Z[j])
        ws_str = self.create_ws(
            OutputWorkspace=f"{name}_Stretch_Zp{spec}",
            DataX=np.array(x),
            DataY=np.array(z),
            NSpec=len(y),
            UnitX="",
            YUnitLabel="",
            VerticalAxisUnit="MomentumTransfer",
            VerticalAxisValues=y,
        )
        # set labels and units
        ws = ADS.retrieve(ws_str)
        x_unit = ws.getAxis(0).setUnit("Label")
        y_unit = ws.getAxis(1).setUnit("Label")
        x_unit.setLabel("beta", "")
        y_unit.setLabel("FWHM", "")
        return ws_str

    def make_slice_ws(self, slice_list, x_data, x_unit, name):
        """
        Creates a workspace of a slice of the countour plot
        :param slice_list: the z values from the slice
        :param x_data: the x values for the slice
        :param x_unit: the unit for the x data
        :param name: the name of the output workspace
        :return the workspace generated
        """
        axis_names = []
        y_data = []
        xx = []

        for j, slice in enumerate(slice_list):
            xx += list(slice[0])
            y_data += list(slice[1])
            axis_names.append(x_data[j])

        return self.create_ws(
            OutputWorkspace=name,
            DataX=np.array(xx),
            DataY=np.array(y_data),
            NSpec=len(axis_names),
            UnitX=x_unit,
            YUnitLabel="",
            VerticalAxisUnit="Text",
            VerticalAxisValues=axis_names,
        )

    def make_results(
        self,
        beta_list,
        FWHM_list,
        x_data: ndarray,
        x_unit: str,
        name: str,
    ):
        """
        Takes the output of quickBayes and makes Mantid workspaces
        :param beta_list: a list of the z values as a function of beta parameter
        :param FWHM_list: a list of the z values as a function of FWHM parameter
        :param x_data: the x data for plotting the results (e.g. Q)
        :param x_unit: the x unit
        :param name: the name of the output worksapce
        :return group workspaces with the FWHM and beta slices
        """
        beta = self.make_slice_ws(beta_list, x_data, x_unit, f"{name}_Stretch_Beta")
        FWHM = self.make_slice_ws(FWHM_list, x_data, x_unit, f"{name}_Stretch_FWHM")
        slice_group = self.group_ws([beta, FWHM], f"{name}_Stretch_Fit")

        return slice_group

    def PyExec(self):
        self.log().information("BayesStretch input")

        # get sample data
        name = self.getPropertyValue("SampleWorkspace")
        sample_ws, N = self.point_data(name)

        # get resolution data
        res_name = self.getPropertyValue("ResolutionWorkspace")
        res_ws, N_res_hist = self.point_data(res_name)

        # setup
        Q = GetThetaQ(sample_ws)
        report_progress = Progress(self, start=0.0, end=1.0, nreports=N + 1)

        # do calculation
        if N_res_hist == 1:
            res_list = self.duplicate_res(res_ws, N)
        elif N_res_hist == N:
            res_list = self.unique_res(res_ws, N)
        else:
            raise ValueError("RES file needs to have either 1 or the same number of histograms as sample.")

        contour_list, beta_list, FWHM_list, sample_logs = self.calculate(sample_ws, report_progress, res_list, N)
        sample_logs.append(("res_workspace", res_name))

        # report results
        contour_group = self.group_ws(contour_list, f"{name}_Stretch_Contour")
        self.add_sample_logs(contour_group, sample_logs, sample_ws)

        slice_group = self.make_results(beta_list, FWHM_list, Q[1], "MomentumTransfer", name)
        self.add_sample_logs(slice_group, sample_logs, sample_ws)

        self.setProperty("OutputWorkspaceFit", slice_group)
        self.setProperty("OutputWorkspaceContour", contour_group)


AlgorithmFactory.subscribe(BayesStretch2)  # Register algorithm with Mantid
