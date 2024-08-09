# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name,too-many-instance-attributes,too-many-branches,no-init
from mantid.api import AlgorithmFactory, WorkspaceGroupProperty, Progress
from mantid.kernel import Direction, IntBoundedValidator, FloatBoundedValidator
from mantid.utils.pip import package_installed
from mantid import logger
from IndirectCommon import get_two_theta_and_q
from mantid.api import AnalysisDataService as ADS
from quickBayesHelper import QuickBayesTemplate

from functools import partial

from numpy import ndarray
import numpy as np
import multiprocessing


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

    # Cannot make static as it prevents it being mocked later
    def QSEFixFunction(self, bg_function, elastic_peak, r_x, r_y, start_x, end_x):
        from quickBayes.functions.qse_fixed import QSEFixFunction

        return QSEFixFunction(bg_function=bg_function, elastic_peak=elastic_peak, r_x=r_x, r_y=r_y, start_x=start_x, end_x=end_x)

    def QSEGridSearch(self):
        from quickBayes.workflow.qse_search import QSEGridSearch

        return QSEGridSearch()

    def parallel(self, items, function, N):
        from quickBayes.utils.parallel import parallel

        return parallel(items=items, function=function, N=N)

    def do_one_spec(self, spec, data):
        sx = data["sample"].readX(spec)
        sy = data["sample"].readY(spec)
        se = data["sample"].readE(spec)

        sample = {"x": sx, "y": sy, "e": se}

        search = self.QSEGridSearch()
        new_x, ry = search.preprocess_data(
            x_data=sample["x"],
            y_data=sample["y"],
            e_data=sample["e"],
            start_x=data["start x"],
            end_x=data["end x"],
            res=data["res_list"][spec],
        )
        search.set_x_axis(start=data["beta start"], end=data["beta end"], N=data["N_beta"], label="beta")
        search.set_y_axis(start=data["FWHM start"], end=data["FWHM end"], N=data["N_FWHM"], label="FWHM")

        # setup fit function
        func = self.QSEFixFunction(
            bg_function=data["BG"], elastic_peak=data["elastic"], r_x=new_x, r_y=ry, start_x=data["start x"], end_x=data["end x"]
        )
        func.add_single_SE()
        func.set_delta_bounds(lower=[0, -0.5], upper=[200, 0.5])
        bounds = func.get_bounds()
        search.set_scipy_engine(guess=func.get_guess(), lower=bounds[0], upper=bounds[1])
        X, Y = search.execute(func=func)
        Z = search.get_grid
        contour = self.make_contour(X=X, Y=Y, Z=Z, spec=spec, name=data["name"])

        beta_slice, FWHM_slice = search.get_slices()
        beta = (search.get_x_axis.values, beta_slice)
        FWHM = (search.get_y_axis.values, FWHM_slice)
        return contour, beta, FWHM

    def calculate_wrapper(self, spec, data):
        return self.do_one_spec(spec, data)

    @staticmethod
    def get_background_function(BG_str):
        from quickBayes.utils.general import get_background_function

        return get_background_function(BG_str)

    def calculate(self, sample_ws, report_progress, res_list, N):
        data = {}

        data["name"] = self.getPropertyValue("SampleWorkspace")
        # get inputs
        data["elastic"] = self.getProperty("Elastic").value
        BG_str = self.getPropertyValue("Background")
        data["BG"] = self.get_background_function(BG_str)
        data["start x"] = self.getProperty("EMin").value
        data["end x"] = self.getProperty("EMax").value
        numCores = self.getProperty("NumberProcessors").value
        data["beta start"] = self.getProperty("StartBeta").value
        data["beta end"] = self.getProperty("EndBeta").value
        data["N_beta"] = self.getProperty("NumberBeta").value

        data["FWHM start"] = self.getProperty("StartFWHM").value
        data["FWHM end"] = self.getProperty("EndFWHM").value
        data["N_FWHM"] = self.getProperty("NumberFWHM").value

        # work around for bug
        if data["start x"] < sample_ws.readX(0)[0]:
            data["start x"] = sample_ws.readX(0)[0]
        if data["end x"] > sample_ws.readX(0)[-1]:
            data["end x"] = sample_ws.readX(0)[-1]

        logger.information(" Number of spectra = {0} ".format(N))
        logger.information(" Erange : {0}  to {1} ".format(data["start x"], data["end x"]))

        # initial values
        contour_list = []
        beta_list = []
        FWHM_list = []

        data["sample"] = sample_ws
        data["res_list"] = res_list
        data["report"] = report_progress

        # calculation
        calc = partial(self.calculate_wrapper, data=data)
        output = self.parallel(list(range(N)), calc, N=numCores)
        # record results
        for spec in range(N):
            contour_list.append(output[spec][0])
            beta_list.append(output[spec][1])
            FWHM_list.append(output[spec][2])

        sample_logs = [
            ("background", BG_str),
            ("elastic_peak", data["elastic"]),
            ("energy_min", data["start x"]),
            ("energy_max", data["end x"]),
            ("StartBeta", data["beta start"]),
            ("EndBeta", data["beta end"]),
            ("NumberBeta", data["N_beta"]),
            ("StartFWHM", data["FWHM start"]),
            ("EndFWHM", data["FWHM end"]),
            ("NumberFWHM", data["N_FWHM"]),
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

    def set_label(self, ws_str, label, unit):
        ws = ADS.retrieve(ws_str)
        axis = ws.getAxis(0)
        axis.setUnit("Label").setLabel(label, unit)
        return ws_str

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
        beta = self.make_slice_ws(slice_list=beta_list, x_data=x_data, x_unit=x_unit, name=f"{name}_Stretch_Beta")
        FWHM = self.make_slice_ws(slice_list=FWHM_list, x_data=x_data, x_unit="FWHM", name=f"{name}_Stretch_FWHM")
        FWHM = self.set_label(ws_str=FWHM, label="FWHM", unit="eV")
        beta = self.set_label(ws_str=beta, label="beta", unit="")

        slice_group = self.group_ws([beta, FWHM], name)

        return slice_group

    def PyExec(self):
        if not package_installed("quickBayes", show_warning=True):
            raise RuntimeError("Please install 'quickBayes' missing dependency")

        self.log().information("BayesStretch input")

        # get sample data
        name = self.getPropertyValue("SampleWorkspace")
        sample_ws, N = self.point_data(name=name)

        # get resolution data
        res_name = self.getPropertyValue("ResolutionWorkspace")
        res_ws, N_res_hist = self.point_data(name=res_name)

        # setup
        Q = get_two_theta_and_q(sample_ws)
        report_progress = Progress(self, start=0.0, end=1.0, nreports=N + 1)

        # do calculation
        if N_res_hist == 1:
            res_list = self.duplicate_res(res_ws=res_ws, N=N)
        elif N_res_hist == N:
            res_list = self.unique_res(res_ws=res_ws, N=N)
        else:
            raise ValueError("RES file needs to have either 1 or the same number of histograms as sample.")

        contour_list, beta_list, FWHM_list, sample_logs = self.calculate(
            sample_ws=sample_ws, report_progress=report_progress, res_list=res_list, N=N
        )
        sample_logs.append(("res_workspace", res_name))

        # report results
        contour_group = self.group_ws(ws_list=contour_list, name=self.getPropertyValue("OutputWorkspaceContour"))
        self.add_sample_logs(workspace=contour_group, sample_logs=sample_logs, data_ws=sample_ws)

        slice_group = self.make_results(
            beta_list=beta_list,
            FWHM_list=FWHM_list,
            x_data=Q[1],
            x_unit="MomentumTransfer",
            name=self.getPropertyValue("OutputWorkspaceFit"),
        )
        self.add_sample_logs(workspace=slice_group, sample_logs=sample_logs, data_ws=sample_ws)

        self.setProperty("OutputWorkspaceFit", slice_group)
        self.setProperty("OutputWorkspaceContour", contour_group)


AlgorithmFactory.subscribe(BayesStretch2)  # Register algorithm with Mantid
