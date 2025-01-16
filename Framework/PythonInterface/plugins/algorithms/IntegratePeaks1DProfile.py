# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import (
    DataProcessorAlgorithm,
    AlgorithmFactory,
    MatrixWorkspaceProperty,
    IPeaksWorkspaceProperty,
    WorkspaceUnitValidator,
    FileProperty,
    FileAction,
    Progress,
    FunctionFactory,
    IPeak,
)
from mantid.kernel import (
    Direction,
    FloatBoundedValidator,
    IntBoundedValidator,
    StringListValidator,
    EnabledWhenProperty,
    PropertyCriterion,
    UnitConversion,
    DeltaEModeType,
    StringArrayProperty,
)
from mantid.fitfunctions import FunctionWrapper
import numpy as np
from scipy.ndimage import binary_dilation
from plugins.algorithms.IntegratePeaksSkew import InstrumentArrayConverter, get_fwhm_from_back_to_back_params, PeakData
from plugins.algorithms.IntegratePeaksShoeboxTOF import get_bin_width_at_tof, set_peak_intensity
from enum import Enum
from typing import Callable, Sequence

MIN_TOF_WIDTH = 1e-3


class IntegratePeaks1DProfile(DataProcessorAlgorithm):
    def name(self):
        return "IntegratePeaks1DProfile"

    def category(self):
        return "Diffraction\\Reduction"

    def seeAlso(self):
        return ["IntegratePeaksSkew", "IntegratePeaksShoeboxTOF"]

    def summary(self):
        return (
            "Integrate single-crystal Bragg peaks in MatrixWorkspaces by fitting peaks TOF in adjacent "
            "pixels unitl I/sigma of the peak in a pixel is below a threshold. Algorithm is adapted and extended"
            "from an algorithm in Gutmann, M. J. (2005). SXD2001. ISIS Facility, RAL"
        )

    def PyInit(self):
        # Input
        self.declareProperty(
            MatrixWorkspaceProperty(
                name="InputWorkspace", defaultValue="", direction=Direction.Input, validator=WorkspaceUnitValidator("TOF")
            ),
            doc="A MatrixWorkspace to integrate (x-axis must be TOF).",
        )
        self.declareProperty(
            IPeaksWorkspaceProperty(name="PeaksWorkspace", defaultValue="", direction=Direction.Input),
            doc="A PeaksWorkspace containing the peaks to integrate.",
        )
        self.declareProperty(
            IPeaksWorkspaceProperty(name="OutputWorkspace", defaultValue="", direction=Direction.Output),
            doc="The output PeaksWorkspace will be a copy of the input PeaksWorkspace with the integrated intensities.",
        )
        # peak window dimensions
        self.declareProperty(
            name="NRows",
            defaultValue=11,
            direction=Direction.Input,
            validator=IntBoundedValidator(lower=3),
            doc="Number of row components in the detector to use in the convolution kernel. "
            "For WISH row components correspond to pixels along a single tube.",
        )
        self.declareProperty(
            name="NCols",
            defaultValue=11,
            direction=Direction.Input,
            validator=IntBoundedValidator(lower=3),
            doc="Number of column components in the detector to use in the convolution kernel. "
            "For WISH column components correspond to tubes.",
        )
        self.declareProperty(
            name="NBins",
            defaultValue=11,
            direction=Direction.Input,
            validator=IntBoundedValidator(lower=3),
            doc="Number of TOF bins to use in the convolution kernel.",
        )
        self.declareProperty(
            name="GetNBinsFromBackToBackParams",
            defaultValue=False,
            direction=Direction.Input,
            doc="If true the number of TOF bins used in the convolution kernel will be calculated from the FWHM of the "
            "BackToBackExponential peak using parameters defined in the instrument parameters.xml file.",
        )
        self.declareProperty(
            name="NFWHM",
            defaultValue=10,
            direction=Direction.Input,
            validator=IntBoundedValidator(lower=1),
            doc="If GetNBinsFromBackToBackParams=True then the number of TOF bins will be NFWHM x FWHM of the "
            "BackToBackExponential at the peak detector and TOF.",
        )
        use_nBins = EnabledWhenProperty("GetNBinsFromBackToBackParams", PropertyCriterion.IsDefault)
        self.setPropertySettings("NBins", use_nBins)
        use_nfwhm = EnabledWhenProperty("GetNBinsFromBackToBackParams", PropertyCriterion.IsNotDefault)
        self.setPropertySettings("NFWHM", use_nfwhm)
        for prop in ["NRows", "NCols", "NBins", "GetNBinsFromBackToBackParams", "NFWHM"]:
            self.setPropertyGroup(prop, "Peak Size")
        # fitting options
        self.declareProperty(
            name="CostFunction",
            defaultValue="Rsq",
            direction=Direction.Input,
            validator=StringListValidator(["RSq", "ChiSq", "Poisson"]),
            doc="Cost funtion to minimise",
        )
        self.declareProperty(
            name="PeakFunction",
            defaultValue="BackToBackExponential",
            direction=Direction.Input,
            validator=StringListValidator(["BackToBackExponential", "Gaussian"]),
            doc="Peak profile funtion to fit.",
        )
        self.declareProperty(
            StringArrayProperty(name="FixPeakParameters", direction=Direction.Input),
            doc="Peak parameters to fix in the fits (recommend fixing A for back-to-back exponential based functions).",
        )
        self.declareProperty(
            name="BackgroundFunction",
            defaultValue="FlatBackground",
            direction=Direction.Input,
            validator=StringListValidator(["FlatBackground", "LinearBackground"]),
            doc="Background function to fit.",
        )
        self.declareProperty(
            name="ErrorStrategy",
            defaultValue="Summation",
            direction=Direction.Input,
            validator=StringListValidator(["Hessian", "Summation"]),
            doc="If Hessian then error on the integrated intensity will be determined from the partial derivatives of "
            "the cost-function wrt the parameters (this is only supported for BackToBackExponential function for "
            "which the integrated intensity is a parameter. If Summation then the error will be the quadrature "
            "sum of the individual errors.",
        )
        self.declareProperty(
            name="IOverSigmaThreshold",
            defaultValue=2.5,
            direction=Direction.Input,
            validator=FloatBoundedValidator(lower=0.0),
            doc="Criterion to stop fitting.",
        )
        self.declareProperty(
            name="FractionalChangeDSpacing",
            defaultValue=0.02,
            direction=Direction.Input,
            validator=FloatBoundedValidator(lower=0.0),
            doc="Fractional change in peak centre allowed - this is a constraint in the fit.",
        )
        for prop in [
            "CostFunction",
            "PeakFunction",
            "FixPeakParameters",
            "BackgroundFunction",
            "ErrorStrategy",
            "IOverSigmaThreshold",
            "FractionalChangeDSpacing",
        ]:
            self.setPropertyGroup(prop, "Fit Options")
        # peak validation
        self.declareProperty(
            name="IntegrateIfOnEdge",
            defaultValue=False,
            direction=Direction.Input,
            doc="If IntegrateIfOnEdge=False then peaks on the detector edge will not be integrated.",
        )
        self.declareProperty(
            name="NRowsEdge",
            defaultValue=1,
            direction=Direction.Input,
            validator=IntBoundedValidator(lower=0),
            doc="Shoeboxes containing detectors NRowsEdge from the detector edge are defined as on the edge.",
        )
        self.declareProperty(
            name="NColsEdge",
            defaultValue=1,
            direction=Direction.Input,
            validator=IntBoundedValidator(lower=0),
            doc="Shoeboxes containing detectors NColsEdge from the detector edge are defined as on the edge.",
        )
        edge_check_enabled = EnabledWhenProperty("IntegrateIfOnEdge", PropertyCriterion.IsDefault)
        self.setPropertySettings("NRowsEdge", edge_check_enabled)
        self.setPropertySettings("NColsEdge", edge_check_enabled)
        for prop in ["IntegrateIfOnEdge", "NRowsEdge", "NColsEdge"]:
            self.setPropertyGroup(prop, "Edge Checking")
        # Corrections
        self.declareProperty(
            name="LorentzCorrection",
            defaultValue=True,
            direction=Direction.Input,
            doc="Correct the integrated intensity by multiplying by the Lorentz factor "
            "sin(theta)^2 / lambda^4 - do not do this if the data have already been corrected.",
        )
        self.setPropertyGroup("LorentzCorrection", "Corrections")
        # plotting
        self.declareProperty(
            FileProperty("OutputFile", "", FileAction.OptionalSave, ".pdf"),
            "Optional file path in which to write diagnostic plots (note this will slow the execution of algorithm).",
        )
        self.setPropertyGroup("OutputFile", "Plotting")

    def validateInputs(self):
        issues = dict()
        # check peak window dimensions
        for prop in ["NRows", "NCols", "NBins"]:
            if not self.getProperty(prop).value % 2:
                issues[prop] = f"{prop} must be an odd number."
        # check valid peak workspace
        ws = self.getProperty("InputWorkspace").value
        inst = ws.getInstrument()
        pk_ws = self.getProperty("PeaksWorkspace").value
        if inst.getName() != pk_ws.getInstrument().getName():
            issues["PeaksWorkspace"] = "PeaksWorkspace must have same instrument as the InputWorkspace."
        if pk_ws.getNumberPeaks() < 1:
            issues["PeaksWorkspace"] = "PeaksWorkspace must have at least 1 peak."
        # check that is getting dTOF from back-to-back params then they are present in instrument
        if self.getProperty("GetNBinsFromBackToBackParams").value:
            # check at least first peak in workspace has back to back params
            if not inst.getComponentByName(pk_ws.column("BankName")[0]).hasParameter("B"):
                issues["GetNBinsFromBackToBackParams"] = (
                    "Workspace doesn't have back to back exponential coefficients defined in the parameters.xml file."
                )
        # check error strategy compatible with peak function
        peak_func_name = self.getProperty("PeakFunction").value
        if peak_func_name != "BackToBackExponential" and self.getProperty("ErrorStrategy").value == "Hessian":
            issues["ErrorStrategy"] = "Hessian only supported for BackToBackExponential (for which integrated intensity is a parameter)."
        # check that fixed peak parameters are valid
        fix_params = self.getProperty("FixPeakParameters").value
        if fix_params:
            peak_func = FunctionFactory.Instance().createPeakFunction(peak_func_name)
            for param in fix_params:
                if not peak_func.hasParameter(param):
                    issues["FixPeakParameters"] = f"Parameter {param} is not in peak function."
                    break
        return issues

    def PyExec(self):
        # get input
        ws = self.getProperty("InputWorkspace").value
        peaks = self.getProperty("PeaksWorkspace").value
        # peak size
        get_nbins_from_b2bexp_params = self.getProperty("GetNBinsFromBackToBackParams").value
        nfwhm = self.getProperty("NFWHM").value
        nrows = self.getProperty("NRows").value
        ncols = self.getProperty("NCols").value
        # fit options
        cost_func_name = self.getProperty("CostFunction").value
        peak_func_name = self.getProperty("PeakFunction").value
        peak_params_to_fix = self.getProperty("FixPeakParameters").value
        bg_func_name = self.getProperty("BackgroundFunction").value
        i_over_sig_threshold = self.getProperty("IOverSigmaThreshold").value
        frac_dspac_delta = self.getProperty("FractionalChangeDSpacing").value
        error_strategy = self.getProperty("ErrorStrategy").value
        # validation
        integrate_on_edge = self.getProperty("IntegrateIfOnEdge").value
        nrows_edge = self.getProperty("NRowsEdge").value
        ncols_edge = self.getProperty("NColsEdge").value
        # corrections
        do_lorz_cor = self.getProperty("LorentzCorrection").value
        # saving file
        output_file = self.getProperty("OutputFile").value

        # create output table workspace
        peaks = self.exec_child_alg("CloneWorkspace", InputWorkspace=peaks, OutputWorkspace="out_peaks")

        # select cost function
        fit_kwargs = {"Minimizer": "Levenberg-Marquardt", "MaxIterations": 5000, "StepSizeMethod": "Sqrt epsilon"}
        match cost_func_name:
            case "RSq":
                fit_kwargs["CostFunction"] = "Unweighted least squares"
            case "ChiSq":
                fit_kwargs["CostFunction"] = "Least squares"
                fit_kwargs["IgnoreInvalidData"] = True
            case "Poisson":
                fit_kwargs["CostFunction"] = "Poisson"
                fit_kwargs["Minimizer"] = "Simplex"  # LM does not support Poisson cost function

        array_converter = InstrumentArrayConverter(ws)
        results = []
        prog_reporter = Progress(self, start=0.0, end=1.0, nreports=peaks.getNumberPeaks())
        for ipk, peak in enumerate(peaks):
            prog_reporter.report("Integrating")

            intens, sigma = 0.0, 0.0
            status = PEAK_STATUS.NO_PEAK

            detid = peak.getDetectorID()
            bank_name = peaks.column("BankName")[ipk]
            pk_tof = peak.getTOF()

            # check TOF is in limits of x-axis
            xdim = ws.getXDimension()
            if xdim.getMinimum() < pk_tof < xdim.getMaximum():
                # check peak is in extent of data
                ispec = ws.getIndicesFromDetectorIDs([detid])[0]
                bin_width = get_bin_width_at_tof(ws, ispec, pk_tof)  # used later to scale intensity
                if get_nbins_from_b2bexp_params:
                    fwhm = get_fwhm_from_back_to_back_params(peak, ws, detid)
                    nbins = max(3, int(nfwhm * fwhm / bin_width)) if fwhm is not None else self.getProperty("NBins").value
                else:
                    nbins = self.getProperty("NBins").value

                # get data array and crop
                peak_data = array_converter.get_peak_data(peak, detid, bank_name, nrows, ncols, nrows_edge, ncols_edge)

                # fit peak
                peak_fitter = PeakFitter(
                    peak,
                    peak_data,
                    nbins,
                    peak_func_name,
                    bg_func_name,
                    peak_params_to_fix,
                    frac_dspac_delta,
                    i_over_sig_threshold,
                    self.exec_fit,
                    fit_kwargs,
                    error_strategy,
                )
                peak_fitter.integrate_peak()

                # update intenisty and save results for later plotting
                if np.any(peak_fitter.successful):
                    # check edge
                    is_on_edge = np.any(np.logical_and(peak_fitter.attempted, peak_data.det_edges))
                    if integrate_on_edge or not is_on_edge:
                        status = PEAK_STATUS.VALID
                        intens = peak_fitter.intens_sum
                        sigma = np.sqrt(peak_fitter.sigma_sq_sum)
                    else:
                        status = PEAK_STATUS.ON_EDGE
                    if output_file:
                        intens_over_sig = intens / sigma if sigma > 0 else 0.0
                        results.append(
                            LineProfileResult(
                                ipk,
                                peak,
                                intens_over_sig,
                                peak_fitter,
                                peak_data,
                                status,
                            )
                        )

            set_peak_intensity(peak, intens, sigma, do_lorz_cor)

        # plot output
        if output_file:
            prog_reporter.resetNumSteps(len(results), start=0.0, end=1.0)
            plot_integration_results(output_file, results, prog_reporter)

        # assign output
        self.setProperty("OutputWorkspace", peaks)

    def exec_child_alg(self, alg_name, **kwargs):
        alg = self.createChildAlgorithm(alg_name, enableLogging=False)
        alg.initialize()
        alg.setProperties(kwargs)
        alg.execute()
        if "OutputWorkspace" in alg.outputProperties():
            return alg.getProperty("OutputWorkspace").value
        else:
            return None

    def exec_fit(self, ispec, **kwargs):
        alg = self.createChildAlgorithm("Fit", enableLogging=False)
        alg.initialize()
        alg.setProperties(kwargs)
        alg.setProperty("WorkspaceIndex", ispec)
        try:
            alg.execute()
            func = alg.getProperty("Function").value  # getPropertyValue returns FunctionProperty not IFunction
            status = alg.getPropertyValue("OutputStatus")
            success = status == "success" or "Changes in function value are too small" in status
            return success, func
        except:
            return False, None


class PeakFitter:
    def __init__(
        self,
        pk,
        peak_data,
        nbins,
        peak_func_name,
        bg_func_name,
        peak_params_to_fix,
        frac_dspac_delta,
        i_over_sig_threshold,
        exec_fit,
        fit_kwargs,
        error_strategy,
    ):
        self.ws = peak_data.ws
        self.pk: IPeak = pk
        self.peak_pos: tuple[int, int] = (peak_data.irow, peak_data.icol)
        self.frac_dspac_delta: float = frac_dspac_delta
        self.i_over_sig_threshold: float = i_over_sig_threshold
        # extract data
        self.tofs: np.ndarray = None
        self.y: np.ndarray = None
        self.esq: np.ndarray = None
        self.ispecs: np.ndarray = None
        self.yfit_foc: np.ndarray = None
        self.successful: np.ndarray = None
        self.attempted: np.ndarray = None
        self.intens_sum: float = 0
        self.sigma_sq_sum: float = 0
        self.peak_func_name: str = peak_func_name
        self.bg_func_name: str = bg_func_name
        self.peak_params_to_fix: Sequence[str] = peak_params_to_fix
        self.exec_fit: Callable = exec_fit
        self.fit_kwargs: dict = None
        self.error_strategy: str = error_strategy
        self.cached_params: dict = None
        self.nfixed_default: int = None
        self.nfixed: int = None

        self.get_and_clip_data_arrays(peak_data, nbins)
        self.calc_limits_on_fwhm()
        self.ysum = self.y.sum(axis=2)
        self.yfit_foc: np.ndarray = np.zeros(self.tofs.shape)
        self.successful: np.ndarray = np.zeros(self.ispecs.shape, dtype=bool)
        self.attempted: np.ndarray = self.successful.copy()
        self.update_peak_position()
        self.fit_kwargs = {
            "InputWorkspace": self.ws,
            "CreateOutput": False,
            "CalcErrors": True,
            "StartX": self.tofs[0],
            "EndX": self.tofs[-1],
            **fit_kwargs,
        }

    def get_tof_slice_for_cropping(self, nbins):
        # get tof indices and limits
        self.ispec = int(self.ispecs[self.peak_pos])
        itof = self.ws.yIndexOfX(self.pk.getTOF(), self.ispec)
        if self.peak_func_name == "BackToBackExponential":
            # take into account asymmetry of peak function in choosing window
            nbins_left = nbins // 3
            istart = itof - nbins_left
            iend = itof + (nbins - nbins_left)
        else:
            istart = itof - nbins // 2
            iend = itof + nbins // 2
        self.tof_slice = slice(
            int(np.clip(istart, a_min=0, a_max=self.ws.blocksize())), int(np.clip(iend, a_min=0, a_max=self.ws.blocksize()))
        )

    def get_and_clip_data_arrays(self, peak_data, nbins):
        tofs, y, esq, self.ispecs = peak_data.get_data_arrays()  # 3d arrays [rows x cols x tof]
        self.get_tof_slice_for_cropping(nbins)
        self.tofs = tofs[self.peak_pos[0], self.peak_pos[1], self.tof_slice]  # take x at peak cen, should be same for all det
        # crop data array to TOF region of peak
        self.y = y[:, :, self.tof_slice]
        self.esq = esq[:, :, self.tof_slice]

    def calc_limits_on_fwhm(self):
        self.min_fwhm = self.tofs[1 + (len(self.tofs) // 2)] - self.tofs[len(self.tofs) // 2]  # FWHM > bin-width
        self.max_fwhm = (self.tofs[-1] - self.tofs[0]) / 3  # must be at least 3 FWHM in data range

    def update_peak_position(self):
        # search for pixel with highest TOF integrated counts in 3x3 window around peak position
        irow_min = np.clip(self.peak_pos[0] - 1, a_min=0, a_max=self.ysum.shape[0])
        irow_max = np.clip(self.peak_pos[0] + 2, a_min=0, a_max=self.ysum.shape[0])  # add 1 as last index not in slice
        icol_min = np.clip(self.peak_pos[1] - 1, a_min=0, a_max=self.ysum.shape[1])
        icol_max = np.clip(self.peak_pos[1] + 2, a_min=0, a_max=self.ysum.shape[1])  # add 1 as last index not in slice
        imax = np.unravel_index(np.argmax(self.ysum[irow_min:irow_max, icol_min:icol_max]), (irow_max - irow_min, icol_max - icol_min))
        self.peak_pos = (imax[0] + irow_min, imax[1] + icol_min)

    def calc_tof_peak_centre_and_bounds(self, ispec):
        # need to do this for each spectrum as DIFC different for each
        diff_consts = self.ws.spectrumInfo().diffractometerConstants(int(ispec))
        tof_pk = UnitConversion.run("dSpacing", "TOF", self.pk.getDSpacing(), 0, DeltaEModeType.Elastic, diff_consts)
        tof_pk_min = np.clip(tof_pk * (1 - self.frac_dspac_delta), a_min=self.tofs[0], a_max=self.tofs[-1])
        tof_pk_max = np.clip(tof_pk * (1 + self.frac_dspac_delta), a_min=self.tofs[0], a_max=self.tofs[-1])
        return tof_pk, tof_pk_min, tof_pk_max

    def create_peak_function_with_initial_params(self, ispec, intensity, bg, tof_pk, tof_pk_min, tof_pk_max):
        # need to create from scratch for setMatrixWorkspace to overwrite parameters
        peak_func = FunctionFactory.Instance().createPeakFunction(self.peak_func_name)
        # set initial parameters of peak
        peak_func.setCentre(tof_pk)
        # initialise instrument specific parameters (e.g A,B,S for case of BackToBackExponential)
        peak_func.setMatrixWorkspace(self.ws, ispec, 0, 0)
        if np.isclose(peak_func.fwhm(), 0.0):
            # width not set by default - set width based max of d-spacing tolerance or bin-width
            fwhm = np.clip(self.frac_dspac_delta * tof_pk, a_min=self.min_fwhm, a_max=self.max_fwhm)
            peak_func.setFwhm(fwhm)
        peak_func.setIntensity(intensity)
        # fix parameters
        self.nfixed_default = len([ipar for ipar in range(peak_func.nParams()) if peak_func.isFixed(ipar)])
        [peak_func.fixParameter(par_name) for par_name in self.peak_params_to_fix]
        self.nfixed = len([ipar for ipar in range(peak_func.nParams()) if peak_func.isFixed(ipar)])
        self.add_constraints_to_peak_function(peak_func, tof_pk_min, tof_pk_max)
        bg_func = FunctionWrapper(self.bg_func_name)
        bg_func.setParameter("A0", bg)  # set constant background
        return FunctionWrapper(peak_func) + bg_func, peak_func

    def add_constraints_to_peak_function(self, peak_func, tof_pk_min, tof_pk_max):
        # set minimum of all parameters to be 0
        [peak_func.addConstraints(f"0<{peak_func.parameterName(ipar)}") for ipar in range(peak_func.nParams())]
        # constrain centre and width
        peak_func.addConstraints(f"{tof_pk_min}<{peak_func.getCentreParameterName()}<{tof_pk_max}")
        # assume constant scale factor between FWHM and width parameter
        width_par_name = peak_func.getWidthParameterName()
        scale_factor = peak_func.getParameterValue(width_par_name) / peak_func.fwhm()
        peak_func.addConstraints(f"{self.min_fwhm * scale_factor}<{width_par_name}<{self.max_fwhm * scale_factor}")

    def fit_spectrum(self, profile_func, ispec):
        return self.exec_fit(ispec, Function=str(profile_func), **self.fit_kwargs)

    def integrate_peak(self):
        self.fit_nearest([self.peak_pos])

    def fit_nearest(self, inearest):
        # fit in order of max intensity
        isort = np.argsort([-self.ysum[inear] for inear in inearest])
        any_successful = False
        for inear in isort:
            irow, icol = inearest[inear]
            self.attempted[irow, icol] = True
            # check enough counts in spectrum
            initial_intens, initial_sigma, bg = self.estimate_intensity_sigma_and_background(irow, icol)
            intens_over_sigma = initial_intens / initial_sigma if initial_sigma > 0 else 0.0
            if intens_over_sigma < self.i_over_sig_threshold:
                continue  # skip this spectrum
            # get center and check min data extent
            ispec = int(self.ispecs[irow, icol])
            tof_pk, tof_pk_min, tof_pk_max = self.calc_tof_peak_centre_and_bounds(ispec)
            if tof_pk < self.tofs[0] or tof_pk > self.tofs[-1]:
                continue  # peak not in data limits
            # update initial parameter guesses
            profile_func, peak_func = self.create_peak_function_with_initial_params(
                ispec, initial_intens, bg, tof_pk, tof_pk_min, tof_pk_max
            )
            # fit
            success, profile_func = self.fit_spectrum(profile_func, ispec)
            if success:
                any_successful = True
                profile_func.freeAll()
                [profile_func[0].fixParameter(par_name) for par_name in self.peak_params_to_fix]
                success_final, profile_func_final = self.fit_spectrum(profile_func, ispec)
                if success_final:
                    profile_func = profile_func_final
                # make peak function and get fwhm and intensity
                [peak_func.setParameter(iparam, profile_func.getParameterValue(iparam)) for iparam in range(peak_func.nParams())]
                if not self.min_fwhm < peak_func.fwhm() < self.max_fwhm:
                    continue  # skip
                intens = peak_func.intensity()
                if self.error_strategy == "Hessian":
                    [peak_func.setError(iparam, profile_func.getError(iparam)) for iparam in range(peak_func.nParams())]
                    sigma = peak_func.intensityError()
                else:
                    sigma = calc_sigma_from_summation(self.tofs, self.esq[irow, icol, :], FunctionWrapper(peak_func)(self.tofs))
                intens_over_sigma = intens / sigma if sigma > 0 else 0.0
                if intens_over_sigma > self.i_over_sig_threshold:
                    self.successful[irow, icol] = True
                    self.yfit_foc += FunctionWrapper(profile_func)(self.tofs)
                    self.intens_sum = self.intens_sum + intens
                    self.sigma_sq_sum = self.sigma_sq_sum + sigma**2
        if not any_successful:
            # no neighbours successfully fitted - terminate here
            return
        # if did break start process again
        inearest = self.find_neighbours()
        return self.fit_nearest(inearest)

    def find_neighbours(self):
        mask = binary_dilation(self.successful)
        mask = np.logical_and(mask, ~self.attempted)
        return list(zip(*np.where(mask)))

    def estimate_intensity_sigma_and_background(self, irow, icol):
        if not np.any(self.y[irow, icol, :] > 0):
            return 0.0, 0.0, 0.0
        ibg, _ = PeakData.find_bg_pts_seed_skew(self.y[irow, icol, :])
        bg = np.mean(self.y[irow, icol, ibg])
        bin_width = np.diff(self.tofs)
        intensity = np.sum((0.5 * (self.y[irow, icol, 1:] + self.y[irow, icol, :-1]) - bg) * bin_width)
        sigma = np.sqrt(np.sum(0.5 * (self.esq[irow, icol, 1:] + self.esq[irow, icol, :-1]) * (bin_width**2)))
        return intensity, sigma, bg


class PEAK_STATUS(Enum):
    VALID = "Valid Peak"
    ON_EDGE = "Peak on detector edge"
    NO_PEAK = "No peak found"


class LineProfileResult:
    """
    This class holds result of line profile integration of a single-crystal Bragg peak
    """

    def __init__(self, ipk, pk, intens_over_sig, peak_fitter, peak_data, status):
        self.irow, self.icol = peak_data.irow, peak_data.icol
        self.tofs = peak_fitter.tofs
        self.ysum = peak_fitter.ysum
        self.yfoc = peak_fitter.y[peak_fitter.successful].sum(axis=0)
        self.efoc = np.sqrt(peak_fitter.esq[peak_fitter.successful].sum(axis=0))
        self.yfoc_fit = peak_fitter.yfit_foc
        self.successful = peak_fitter.successful
        # extract peak properties inot title
        intens_over_sig = np.round(intens_over_sig, 1)
        hkl = np.round(pk.getHKL(), 2)
        wl = np.round(pk.getWavelength(), 2)
        tth = np.round(np.degrees(pk.getScattering()), 1)
        d = np.round(pk.getDSpacing(), 2)
        self.title = (
            f"{ipk} ({','.join(str(hkl)[1:-1].split())})"
            f"\n$I/\\sigma$={intens_over_sig}\n"
            rf"$\lambda$={wl} $\AA$; "
            rf"$2\theta={tth}^\circ$; "
            rf"d={d} $\AA$"
            f"\n{status.value}"
        )

    def plot_integrated_peak(self, fig, axes, norm_func):
        # plot colorfill of TOF integrated data on LHS
        im = axes[0].imshow(self.ysum)
        im.set_norm(norm_func())
        axes[0].plot(*np.where(self.successful)[::-1], "ow")
        axes[0].plot(self.icol, self.irow, "+r")  # peak centre
        axes[0].set_xlabel("Col")
        axes[0].set_ylabel("Row")
        # plot focussed spectrum
        axes[1].errorbar(self.tofs, self.yfoc, yerr=self.efoc, marker="o", color="k", capsize=2, ls="")
        axes[1].plot(self.tofs, self.yfoc_fit, "-r")
        axes[1].set_xlabel("TOF (mus)")
        axes[1].set_ylabel("Intensity (a.u.)")
        # set title
        fig.suptitle(self.title)


def calc_sigma_from_summation(xspec, esq_spec, ypeak, cutoff=0.025):
    nbins = len(ypeak)
    ypeak_cumsum = np.cumsum(ypeak)
    ypeak_cumsum /= ypeak_cumsum[-1]
    ilo = np.clip(np.argmin(abs(ypeak_cumsum - cutoff)), a_min=0, a_max=nbins // 2)
    ihi = np.clip(np.argmin(abs(ypeak_cumsum - (1 - cutoff))), a_min=nbins // 2, a_max=nbins - 1) + 1
    bin_width = np.diff(xspec[ilo:ihi])
    return np.sqrt(np.sum(0.5 * (esq_spec[ilo : ihi - 1] + esq_spec[ilo + 1 : ihi]) * (bin_width**2)))


def plot_integration_results(output_file, results, prog_reporter):
    # import inside this function as not allowed to import at point algorithms are registered
    from matplotlib.pyplot import subplots, close
    from matplotlib.colors import Normalize
    from matplotlib.backends.backend_pdf import PdfPages

    try:
        with PdfPages(output_file) as pdf:
            for result in results:
                prog_reporter.report("Plotting")
                fig, axes = subplots(1, 2, figsize=(12, 5), layout="compressed", subplot_kw={"projection": "mantid"})
                result.plot_integrated_peak(fig, axes, Normalize)
                pdf.savefig(fig)
                close(fig)
    except OSError:
        raise RuntimeError(
            f"OutputFile ({output_file}) could not be opened - please check it is not open by "
            f"another programme and that the user has permission to write to that directory."
        )


# register algorithm with mantid
AlgorithmFactory.subscribe(IntegratePeaks1DProfile)
