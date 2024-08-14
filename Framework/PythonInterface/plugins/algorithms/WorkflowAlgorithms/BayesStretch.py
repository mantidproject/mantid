# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name,too-many-instance-attributes,too-many-branches,no-init
from mantid.api import PythonAlgorithm, AlgorithmFactory, MatrixWorkspaceProperty, WorkspaceGroupProperty, Progress
from mantid.kernel import StringListValidator, Direction
import mantid.simpleapi as s_api
from mantid import config, logger
import os
import numpy as np


class BayesStretch(PythonAlgorithm):
    _sam_name = None
    _sam_ws = None
    _res_name = None
    _e_min = None
    _e_max = None
    _sam_bins = None
    _elastic = None
    _background = None
    _nbet = None
    _nsig = None
    _loop = None

    _erange = None
    _nbins = None

    def category(self):
        return "Workflow\\MIDAS"

    def summary(self):
        return (
            "This algorithm is deprecated, please use BayesStretch2 instead. \n"
            + "*************************************************************** \n \n"
            + "This is a variation of the stretched exponential option of Quasi."
        )

    def PyInit(self):
        self.declareProperty(
            MatrixWorkspaceProperty("SampleWorkspace", "", direction=Direction.Input), doc="Name of the Sample input Workspace"
        )

        self.declareProperty(
            MatrixWorkspaceProperty("ResolutionWorkspace", "", direction=Direction.Input), doc="Name of the resolution input Workspace"
        )

        self.declareProperty(name="EMin", defaultValue=-0.2, doc="The start of the fitting range")

        self.declareProperty(name="EMax", defaultValue=0.2, doc="The end of the fitting range")

        self.declareProperty(name="SampleBins", defaultValue=1, doc="The number of sample bins")

        self.declareProperty(name="Elastic", defaultValue=True, doc="Fit option for using the elastic peak")

        self.declareProperty(
            name="Background",
            defaultValue="Flat",
            validator=StringListValidator(["Sloping", "Flat", "Zero"]),
            doc="Fit option for the type of background",
        )

        self.declareProperty(name="NumberSigma", defaultValue=50, doc="Number of sigma values")

        self.declareProperty(name="NumberBeta", defaultValue=30, doc="Number of beta values")

        self.declareProperty(name="Loop", defaultValue=True, doc="Switch Sequential fit On/Off")

        self.declareProperty(
            WorkspaceGroupProperty("OutputWorkspaceFit", "", direction=Direction.Output), doc="The name of the fit output workspaces"
        )

        self.declareProperty(
            WorkspaceGroupProperty("OutputWorkspaceContour", "", direction=Direction.Output),
            doc="The name of the contour output workspaces",
        )

    def validateInputs(self):
        self._get_properties()
        issues = dict()

        # Validate fitting range in energy
        if self._e_min > self._e_max:
            issues["EMax"] = "Must be less than EnergyMin"

        if self._res_ws is None:
            issues["ResolutionWorkspace"] = "Must be a MatrixWorkspace"

        if self._sam_ws is None:
            issues["SampleWorkspace"] = "Must be a MatrixWorkspace"
        else:
            # Validate fitting range within data range
            data_min = self._sam_ws.readX(0)[0]
            if self._e_min < data_min:
                issues["EMin"] = "EMin must be more than the minimum x range of the data."
            data_max = self._sam_ws.readX(0)[-1]
            if self._e_max > data_max:
                issues["EMax"] = "EMax must be less than the maximum x range of the data"

        return issues

    # pylint: disable=too-many-locals
    def PyExec(self):
        from quasielasticbayes import Quest as Que

        from IndirectBayes import CalcErange, GetXYE
        from IndirectCommon import check_x_range, check_analysers_or_e_fixed, get_efixed, get_two_theta_and_q, check_hist_zero

        setup_prog = Progress(self, start=0.0, end=0.3, nreports=5)
        logger.warning("This algorithm is deprecated, please use BayesStretch2 instead.")
        logger.information("BayesStretch input")
        logger.information("Sample is %s" % self._sam_name)
        logger.information("Resolution is %s" % self._res_name)

        setup_prog.report("Converting to binary for Fortran")
        fitOp = self._encode_fit_ops(self._elastic, self._background)

        setup_prog.report("Establishing save path")
        workdir = self._establish_save_path()

        setup_prog.report("Checking X Range")
        check_x_range(self._erange, "Energy")

        setup_prog.report("Checking Analysers")
        check_analysers_or_e_fixed(self._sam_name, self._res_name)
        setup_prog.report("Obtaining EFixed, theta and Q")
        efix = get_efixed(self._sam_name)
        theta, Q = get_two_theta_and_q(self._sam_name)

        setup_prog.report("Checking Histograms")
        nsam, ntc = check_hist_zero(self._sam_name)

        # check if we're performing a sequential fit
        if not self._loop:
            nsam = 1

        logger.information("Version is Stretch")
        logger.information("Number of spectra = %s " % nsam)
        logger.information("Erange : %f to %f " % (self._erange[0], self._erange[1]))

        setup_prog.report("Creating FORTRAN Input")
        fname = self._sam_name[:-4] + "_Stretch"
        wrks = os.path.join(workdir, self._sam_name[:-4])
        logger.information("lptfile : %s_Qst.lpt" % wrks)
        lwrk = len(wrks)
        wrks.ljust(140, " ")
        wrkr = self._res_name
        wrkr.ljust(140, " ")
        eBet0 = np.zeros(self._nbet)  # set errors to zero
        eSig0 = np.zeros(self._nsig)  # set errors to zero
        rscl = 1.0
        Qaxis = ""

        workflow_prog = Progress(self, start=0.3, end=0.7, nreports=nsam * 3)

        # Empty arrays to hold Sigma and Bet x,y,e values
        xSig, ySig, eSig = [], [], []
        xBet, yBet, eBet = [], [], []

        for m in range(nsam):
            logger.information("Group %i at angle %f" % (m, theta[m]))
            nsp = m + 1
            nout, bnorm, Xdat, Xv, Yv, Ev = CalcErange(self._sam_name, m, self._erange, self._nbins[0])
            Ndat = nout[0]
            Imin = nout[1]
            Imax = nout[2]

            # get resolution data (4096 = FORTRAN array length)
            Nb, Xb, Yb, _ = GetXYE(self._res_name, 0, 4096)
            numb = [nsam, nsp, ntc, Ndat, self._nbins[0], Imin, Imax, Nb, self._nbins[1], self._nbet, self._nsig]
            reals = [efix, theta[m], rscl, bnorm]

            workflow_prog.report("Processing spectrum number %i" % m)
            xsout, ysout, xbout, ybout, zpout = Que.quest(numb, Xv, Yv, Ev, reals, fitOp, Xdat, Xb, Yb, wrks, wrkr, lwrk)
            dataXs = xsout[: self._nsig]  # reduce from fixed FORTRAN array
            dataYs = ysout[: self._nsig]
            dataXb = xbout[: self._nbet]
            dataYb = ybout[: self._nbet]
            zpWS = fname + "_Zp" + str(m)
            if m > 0:
                Qaxis += ","
            Qaxis += str(Q[m])

            dataXz = []
            dataYz = []
            dataEz = []

            for n in range(self._nsig):
                yfit_list = np.split(zpout[: self._nsig * self._nbet], self._nsig)
                dataYzp = yfit_list[n]

                dataXz = np.append(dataXz, xbout[: self._nbet])
                dataYz = np.append(dataYz, dataYzp[: self._nbet])
                dataEz = np.append(dataEz, eBet0)

            zpWS = fname + "_Zp" + str(m)
            self._create_workspace(zpWS, [dataXz, dataYz, dataEz], self._nsig, dataXs, True)

            xSig = np.append(xSig, dataXs)
            ySig = np.append(ySig, dataYs)
            eSig = np.append(eSig, eSig0)
            xBet = np.append(xBet, dataXb)
            yBet = np.append(yBet, dataYb)
            eBet = np.append(eBet, eBet0)

            if m == 0:
                groupZ = zpWS
            else:
                groupZ = groupZ + "," + zpWS

        # create workspaces for sigma and beta
        workflow_prog.report("Creating OutputWorkspace")
        self._create_workspace(fname + "_Sigma", [xSig, ySig, eSig], nsam, Qaxis)
        self._create_workspace(fname + "_Beta", [xBet, yBet, eBet], nsam, Qaxis)

        group = fname + "_Sigma," + fname + "_Beta"
        fit_ws = fname + "_Fit"
        s_api.GroupWorkspaces(InputWorkspaces=group, OutputWorkspace=fit_ws)
        contour_ws = fname + "_Contour"
        s_api.GroupWorkspaces(InputWorkspaces=groupZ, OutputWorkspace=contour_ws)

        # Add some sample logs to the output workspaces
        log_prog = Progress(self, start=0.8, end=1.0, nreports=6)
        log_prog.report("Copying Logs to Fit workspace")
        copy_log_alg = self.createChildAlgorithm("CopyLogs", enableLogging=False)
        copy_log_alg.setProperty("InputWorkspace", self._sam_name)
        copy_log_alg.setProperty("OutputWorkspace", fit_ws)
        copy_log_alg.execute()

        log_prog.report("Adding Sample logs to Fit workspace")
        self._add_sample_logs(fit_ws, self._erange, self._nbins[0])

        log_prog.report("Copying logs to Contour workspace")
        copy_log_alg.setProperty("InputWorkspace", self._sam_name)
        copy_log_alg.setProperty("OutputWorkspace", contour_ws)
        copy_log_alg.execute()

        log_prog.report("Adding sample logs to Contour workspace")
        self._add_sample_logs(contour_ws, self._erange, self._nbins[0])
        log_prog.report("Finialising log copying")

        # sort x axis
        s_api.SortXAxis(InputWorkspace=fit_ws, OutputWorkspace=fit_ws, EnableLogging=False)
        s_api.SortXAxis(InputWorkspace=contour_ws, OutputWorkspace=contour_ws, EnableLogging=False)

        self.setProperty("OutputWorkspaceFit", fit_ws)
        self.setProperty("OutputWorkspaceContour", contour_ws)
        log_prog.report("Setting workspace properties")

    # ----------------------------- Helper functions -----------------------------

    def _encode_fit_ops(self, elastic, background):
        """
        Encode the fit options are boolean values for use in FORTRAN
        @param elastic      :: If the peak is elastic
        @param background   :: Type of background to fit
        @return fit_ops [elastic, background, width, resNorm]
        """

        if background == "Sloping":
            o_bgd = 2
        elif background == "Flat":
            o_bgd = 1
        elif background == "Zero":
            o_bgd = 0

        fitOp = [1 if elastic else 0, o_bgd, 0, 0]
        return fitOp

    def _establish_save_path(self):
        """
        @return the directory to save FORTRAN outputs to
        """
        workdir = config["defaultsave.directory"]
        if not os.path.isdir(workdir):
            workdir = os.getcwd()
            logger.information("Default Save directory is not set.")
            logger.information("Defaulting to current working Directory: " + workdir)
        return workdir

    # pylint: disable=too-many-arguments
    def _create_workspace(self, name, xye, num_spec, vert_axis, is_zp_ws=False):
        """
        Creates a workspace from FORTRAN data

        @param name         :: Full name of outputworkspace
        @param xye          :: List of axis data [x, y , e]
        @param num_spec     :: Number of spectra
        @param vert_axis    :: The values on the vertical axis
        @param is_zp_ws     :: Creating a zp_ws (if True)
        """

        unit_x = ""
        if is_zp_ws:
            unit_x = "MomentumTransfer"

        ws = s_api.CreateWorkspace(
            OutputWorkspace=name,
            DataX=xye[0],
            DataY=xye[1],
            DataE=xye[2],
            Nspec=num_spec,
            UnitX=unit_x,
            VerticalAxisUnit="MomentumTransfer",
            VerticalAxisValues=vert_axis,
        )

        unitx = ws.getAxis(0).setUnit("Label")
        if is_zp_ws:
            unity = ws.getAxis(1).setUnit("Label")
            unitx.setLabel("beta", "")
            unity.setLabel("sigma", "")
        else:
            if name[-4:] == "Beta":
                unitx.setLabel("beta", "")
            else:
                unitx.setLabel("sigma", "")

    def _add_sample_logs(self, workspace, erange, sample_binning):
        """
        Add the Bayes Stretch specific values to the sample logs
        """
        energy_min, energy_max = erange

        log_names = ["res_file", "background", "elastic_peak", "energy_min", "energy_max", "sample_binning"]
        log_values = [self._res_name, str(self._background), str(self._elastic), energy_min, energy_max, sample_binning]

        add_log = self.createChildAlgorithm("AddSampleLogMultiple", enableLogging=False)
        add_log.setProperty("Workspace", workspace)
        add_log.setProperty("LogNames", log_names)
        add_log.setProperty("LogValues", log_values)
        add_log.setProperty("ParseType", True)  # Should determine String/Number type
        add_log.execute()

    def _get_properties(self):
        self._sam_name = self.getPropertyValue("SampleWorkspace")
        self._sam_ws = self.getProperty("SampleWorkspace").value
        self._res_name = self.getPropertyValue("ResolutionWorkspace")
        self._res_ws = self.getProperty("ResolutionWorkspace").value
        self._e_min = self.getProperty("EMin").value
        self._e_max = self.getProperty("EMax").value
        self._sam_bins = self.getPropertyValue("SampleBins")
        self._elastic = self.getProperty("Elastic").value
        self._background = self.getPropertyValue("Background")
        self._nbet = self.getProperty("NumberBeta").value
        self._nsig = self.getProperty("NumberSigma").value
        self._loop = self.getProperty("Loop").value

        self._erange = [self._e_min, self._e_max]
        # [sample_bins, resNorm_bins=1]
        self._nbins = [self._sam_bins, 1]


AlgorithmFactory.subscribe(BayesStretch)  # Register algorithm with Mantid
