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
    SpectrumInfo,
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
from collections.abc import Sequence
from scipy.optimize import approx_fprime, minimize
from scipy.ndimage import binary_dilation
from IntegratePeaksSkew import InstrumentArrayConverter, get_fwhm_from_back_to_back_params
from IntegratePeaksShoeboxTOF import get_bin_width_at_tof, find_nearest_peak_in_data_window, set_peak_intensity
from enum import Enum
from typing import Callable


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
        match cost_func_name:
            case "RSq":
                cost_func = cost_func_rsq
                weight_func = calc_weights_rsq
            case "ChiSq":
                cost_func = cost_func_chisq
                weight_func = calc_weights_chisq
            case "Poisson":
                cost_func = cost_func_poisson
                weight_func = calc_weights_poisson

        array_converter = InstrumentArrayConverter(ws)
        bg_func = FunctionWrapper(bg_func_name)
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
                    ws,
                    peaks,
                    ipk,
                    peak,
                    peak_data,
                    nbins,
                    error_strategy,
                    weight_func,
                    cost_func,
                    peak_func_name,
                    bg_func,
                    peak_params_to_fix,
                    frac_dspac_delta,
                    i_over_sig_threshold,
                )
                successful, attempted, yfits, intens_sum, sigma_sq_sum = peak_fitter.integrate_peak()

                # update intenisty and save results for later plotting
                if np.any(successful):
                    # check edge
                    is_on_edge = np.any(np.logical_and(attempted, peak_data.det_edges))
                    if integrate_on_edge or not is_on_edge:
                        status = PEAK_STATUS.VALID
                        intens = intens_sum
                        sigma = np.sqrt(sigma_sq_sum)
                    else:
                        status = PEAK_STATUS.ON_EDGE
                    if output_file:
                        intens_over_sig = intens / sigma if sigma > 0 else 0.0
                        results.append(
                            LineProfileResult(
                                ipk,
                                peak,
                                intens_over_sig,
                                peak_fitter.tofs,
                                peak_fitter.y,
                                peak_fitter.esq,
                                yfits,
                                successful,
                                peak_data.irow,
                                peak_data.icol,
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


class PeakFitter:
    def __init__(
        self,
        ws,
        peaks,
        ipk,
        pk,
        peak_data,
        nbins,
        error_strategy,
        weight_func,
        cost_func,
        peak_func_name,
        bg_func,
        peak_params_to_fix,
        frac_dspac_delta,
        i_over_sig_threshold,
    ):
        self.pk: IPeak = pk
        self.peak_pos: tuple[int, int] = (peak_data.irow, peak_data.icol)
        self.spec_info: SpectrumInfo = ws.spectrumInfo()
        self.error_strategy: str = error_strategy
        self.weight_func: Callable = weight_func
        self.cost_func: Callable = cost_func
        self.det_edges: np.ndarray = peak_data.det_edges
        self.frac_dspac_delta: float = frac_dspac_delta
        self.i_over_sig_threshold: float = i_over_sig_threshold
        self.tofs: np.ndarray = None
        self.y: np.ndarray = None
        self.esq: np.ndarray = None
        self.ispecs: np.ndarray = None
        self.peak_func: IPeakFunction = None
        self.profile_func: FunctionWrapper = None
        self.nparams_pk: int = None
        self.nparams_bg: int = None
        self.iparam_cen: int = None
        self.ifix_initial: list[int] = None  # parameters to keep fixed on initial fit
        self.ifix_final: list[int] = None

        self.get_data_arrays(ws, peak_data, nbins)
        self.update_initial_peak_position(ws, peaks, ipk)
        self.get_composite_function_with_initial_params(ws, peak_func_name, bg_func)
        self.get_index_of_parameters_to_fix(peak_params_to_fix)

    def get_data_arrays(self, ws, peak_data, nbins):
        self.tofs, self.y, self.esq, self.ispecs = get_and_clip_data_arrays(ws, peak_data, self.pk.getTOF(), nbins)

    def update_initial_peak_position(self, ws, peaks, ipk):
        itof = np.argmin(abs(self.tofs - self.pk.getTOF()))
        peak_pos = find_nearest_peak_in_data_window(
            self.y, self.ispecs, self.tofs, ws, peaks, ipk, (*self.peak_pos, itof), min_threshold=0.0
        )
        self.peak_pos = (peak_pos[0], peak_pos[1]) if peak_pos is not None else None  # omit tof index

    def get_composite_function_with_initial_params(self, ws, peak_func_name, bg_func):
        # need to create from scratch for setMatrxiWorkspace to overwrite parameters
        self.peak_func = FunctionFactory.Instance().createPeakFunction(peak_func_name)
        # set centre and determine the index of the centre parameter
        nparams = self.peak_func.nParams()
        default_params = np.array([self.peak_func.getParameterValue(iparam) for iparam in range(nparams)])
        self.peak_func.setCentre(self.pk.getTOF())
        self.iparam_cen = next(
            iparam for iparam in range(nparams) if not np.isclose(self.peak_func.getParameterValue(iparam), default_params[iparam])
        )
        if self.peak_pos is not None:
            # initilaise instrument specific parameters (e.g A,B,S for case of BackTobackExponential)
            self.peak_func.setMatrixWorkspace(ws, int(self.ispecs[self.peak_pos]), 0, 0)
            if np.isclose(self.peak_func.fwhm(), 0.0):
                # width not set by default - set width based max of d-spacing tolerance or bin-width
                bin_width = self.tofs[-1] - self.tofs[-2]
                fwhm = max(self.frac_dspac_delta * self.pk.getTOF(), bin_width)
                self.peak_func.setFwhm(fwhm)
        # combine peak and background function wrappers
        self.profile_func = FunctionWrapper(self.peak_func) + bg_func
        self.nparams_pk = self.peak_func.nParams()
        self.nparams_bg = bg_func.nParams()
        self.p_guess = np.array([self.profile_func.getParameter(iparam) for iparam in range(self.profile_func.nParams())])

    def get_index_of_parameters_to_fix(self, peak_params_to_fix):
        # get index of fixed peak parameters
        self.ifix_final = [self.peak_func.getParameterIndex(param) for param in peak_params_to_fix]
        self.ifix_initial = [iparam for iparam in range(self.nparams_pk) if self.peak_func.isFixed(iparam)]
        # fix background parameter in initial fit
        self.ifix_initial = [iparam + self.nparams_pk for iparam in range(self.nparams_bg)]
        # make sure initial fit includes fixed parameters specified
        self.ifix_initial = list(set(self.ifix_initial).union(set(self.ifix_final)))

    def calc_tof_peak_centre_and_bounds(self, ispec):
        diff_consts = self.spec_info.diffractometerConstants(int(ispec))
        tof_pk = UnitConversion.run("dSpacing", "TOF", self.pk.getDSpacing(), 0, DeltaEModeType.Elastic, diff_consts)
        tof_pk_min = np.clip(tof_pk * (1 - self.frac_dspac_delta), a_min=self.tofs[0], a_max=self.tofs[-1])
        tof_pk_max = np.clip(tof_pk * (1 + self.frac_dspac_delta), a_min=self.tofs[0], a_max=self.tofs[-1])
        return tof_pk, tof_pk_min, tof_pk_max

    def fit_spectrum(self, p_guess, weights, irow, icol, cen_min, cen_max, ifix):
        # set bounds (lb=0.0 for all params)
        bounds = np.array(len(p_guess) * [0.0, np.inf]).reshape(-1, 2)
        for iparam in ifix:
            bounds[iparam, :] = p_guess[iparam]
        bounds[self.iparam_cen, :] = [cen_min, cen_max]
        bounds[self.nparams_pk + 1 : self.profile_func.nParams(), 0] = -np.inf  # reset lower bound of higher order bg terms
        # fit
        result = minimize(
            calc_cost_func,
            p_guess,
            args=(self.profile_func, self.tofs, self.y[irow, icol, :], weights, self.cost_func),
            bounds=tuple(bounds),
            method="Nelder-Mead",
            options={"adaptive": True, "maxfev": 2000},
        )
        # set parameters (is this altered in-place?)
        [self.profile_func.setParameter(iparam, param) for iparam, param in enumerate(result.x)]
        yfit = self.profile_func(self.tofs)
        return result, yfit

    def integrate_peak(self):
        successful = np.zeros(self.ispecs.shape, dtype=bool)
        attempted = successful.copy()
        inearest = [self.peak_pos]  # index of initial spectrum to fit
        yfits = np.zeros(self.y.shape)
        intens_sum, sigma_sq_sum = 0.0, 0.0
        if self.peak_pos is None:
            # no peak found
            return successful, attempted, yfits, intens_sum, sigma_sq_sum
        else:
            return self.fit_nearest(attempted, successful, inearest, intens_sum, sigma_sq_sum, yfits)

    def fit_nearest(self, attempted, successful, inearest, intens_sum, sigma_sq_sum, yfits):
        # pre-calculate values used later to check validity of fit
        bin_width = self.tofs[-1] - self.tofs[-2]
        tof_range = self.tofs[-1] - self.tofs[0]
        # fit in order of max intensity
        isort = np.argsort([-self.y.sum(axis=2)[inear] for inear in inearest])
        any_successful = False
        for inear in isort:
            irow, icol = inearest[inear]
            attempted[irow, icol] = True
            # check enough counts in spectrum
            intial_intens, intial_sigma, bg = self.estimate_intensity_sigma_and_background(irow, icol)
            intens_over_sigma = intial_intens / intial_sigma if intial_sigma > 0 else 0.0
            if intens_over_sigma < self.i_over_sig_threshold:
                continue  # skip this spectrum
            # get center and check min data extent
            tof_pk, tof_pk_min, tof_pk_max = self.calc_tof_peak_centre_and_bounds(self.ispecs[irow, icol])
            if tof_pk < self.tofs[0] or tof_pk > self.tofs[-1]:
                continue  # peak not in data limits
            # update initial parameter guesses
            p_guess = self.p_guess.copy()
            p_guess[self.iparam_cen] = tof_pk
            self.set_peak_intensity(intial_intens, p_guess)
            self.set_constant_background(bg, p_guess)
            # fit
            weights = self.weight_func(self.y[irow, icol, :], self.esq[irow, icol, :])
            nfix = len(self.ifix_initial)
            result, yfit = self.fit_spectrum(p_guess, weights, irow, icol, tof_pk_min, tof_pk_max, ifix=self.ifix_initial)
            if result.success:
                any_successful = True
                if len(self.ifix_final) < len(self.ifix_initial):
                    # fit again but free some previously fixed parameters
                    result_final, yfit_final = self.fit_spectrum(
                        result.x, weights, irow, icol, tof_pk_min, tof_pk_max, ifix=self.ifix_final
                    )
                    if result_final.success:
                        result = result_final
                        yfit = yfit_final
                        nfix = len(self.ifix_final)
                # check fit realistic (unfortunately Nelder-Mead does not support non-linear constraints in fit)
                fwhm = self.get_fwhm_from_function_with_params()
                if fwhm < bin_width or fwhm > tof_range:
                    continue  # skip
                intens = self.get_intensity_from_function()
                # calculate intensity and errors
                if self.error_strategy == "Hessian":
                    sigma = calc_sigma_from_hessian(
                        result, self.profile_func, self.tofs, self.y[irow, icol, :], weights, self.cost_func, nfix
                    )
                else:
                    sigma = calc_sigma_from_summation(self.tofs, self.esq[irow, icol, :], self.profile_func[0](self.tofs))
                intens_over_sigma = intens / sigma if sigma > 0 else 0.0
                if intens_over_sigma > self.i_over_sig_threshold:
                    successful[irow, icol] = True
                    yfits[irow, icol, :] = yfit
                    intens_sum = intens_sum + intens
                    sigma_sq_sum = sigma_sq_sum + sigma**2
        if not any_successful:
            # no neighbours successfully fitted - terminate here
            return successful, attempted, yfits, intens_sum, sigma_sq_sum
        # if did break start process again
        inearest = self.find_neighbours(successful, attempted)
        return self.fit_nearest(attempted, successful, inearest, intens_sum, sigma_sq_sum, yfits)

    def find_neighbours(self, successful, attempted):
        mask = binary_dilation(successful)
        mask = np.logical_and(mask, ~attempted)
        return list(zip(*np.where(mask)))

    def get_fwhm_from_function_with_params(self):
        [self.peak_func.setParameter(iparam, self.profile_func.getParameterValue(iparam)) for iparam in range(self.nparams_pk)]
        return self.peak_func.fwhm()

    def get_intensity_from_function(self):
        [self.peak_func.setParameter(iparam, self.profile_func.getParameterValue(iparam)) for iparam in range(self.nparams_pk)]
        return self.peak_func.intensity()

    def set_peak_intensity(self, intensity, params):
        [self.peak_func.setParameter(iparam, params[iparam]) for iparam in range(self.nparams_pk)]
        self.peak_func.setIntensity(intensity)
        params[: self.nparams_pk] = [self.peak_func.getParameterValue(iparam) for iparam in range(self.nparams_pk)]

    def set_constant_background(self, bg, params):
        params[-self.nparams_bg :] = 0  # reset all background terms to zero
        params[-self.nparams_bg] = bg  # constant is always first parameter in supported background functions

    def estimate_intensity_sigma_and_background(self, irow, icol):
        ipositive = self.y[irow, icol, :] > 0
        if not ipositive.any():
            return 0.0, 0.0, 0.0
        bg = np.min(self.y[irow, icol, :][ipositive])
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

    def __init__(self, ipk, pk, intens_over_sig, tofs, y, esq, yfits, successful, irow, icol, status):
        self.irow, self.icol = irow, icol
        self.tofs = tofs
        self.ysum = np.sum(y, axis=2)  # integrate over TOF
        self.yfoc = y[successful].sum(axis=0)
        self.efoc = np.sqrt(esq[successful].sum(axis=0))
        self.yfoc_fit = yfits[successful].sum(axis=0)
        self.successful = successful
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


def calc_sigma_from_hessian(result, profile_func, x, y, weights, cost_func, nfixed, min_step=1e-5):
    step_size = min_step * result.x
    step_size[step_size < min_step] = min_step
    hess = calc_hessian(calc_cost_func, result.x, step_size, profile_func, x, y, weights, cost_func)
    cov = np.linalg.inv(hess) * 2.0 * result.fun / (len(y) - (len(result.x) - nfixed))
    return np.sqrt(cov[0, 0])  # first parameter is intenisity for BackToBackExponential (only func supported)


def is_sequence(seq):
    # Strings are sequences so check for those separately
    if isinstance(seq, str):
        return False
    # NumPy array is not a Sequence
    return isinstance(seq, np.ndarray) or isinstance(seq, Sequence)


def calc_hessian(func, x0, step_size, *args):
    nparam = len(x0)
    if not is_sequence(step_size):
        step_size = nparam * [step_size]
    hessian = np.zeros((nparam, nparam))
    jac = approx_fprime(x0, func, step_size, *args)
    for iparam in range(nparam):
        x0[iparam] = x0[iparam] + step_size[iparam]
        jac_dx = approx_fprime(x0, func, step_size, *args)
        hessian[:, iparam] = (jac_dx - jac) / step_size
        # reset param
        x0[iparam] = x0[iparam] - step_size[iparam]
    return hessian


def replace_nans(arr):
    isfinite = np.isfinite(arr)
    if not isfinite.all():
        isnan = ~isfinite
        arr[isnan] = np.interp(np.flatnonzero(isnan), np.flatnonzero(isfinite), arr[isfinite])
    return arr


def calc_weights_rsq(y, esq):
    return None


def calc_weights_chisq(y, esq):
    with np.errstate(divide="ignore", invalid="ignore"):
        scale = replace_nans(y / esq)
    weights = esq.copy()
    iempty = np.isclose(esq, 0.0)
    weights[iempty] = 1 / scale[iempty]
    return weights


def calc_weights_poisson(y, esq):
    with np.errstate(divide="ignore", invalid="ignore"):
        scale = replace_nans(y / esq)
    return scale


def cost_func_rsq(ycalc, y, ignored):
    return np.sum((ycalc - y) ** 2)


def cost_func_chisq(ycalc, y, weights):
    # weights are e^2
    return np.sum(((ycalc - y) ** 2) / weights)


def cost_func_poisson(ycalc, y, weights):
    # weights = y/e**2 (i.e. y = counts/weights, e = sqrt(counts)/weights)
    y_cnts = y * weights
    ycalc_cnts = ycalc * weights
    inonzero = ~np.isclose(y_cnts, 0.0)
    ycalc_cnts[np.logical_and(inonzero, np.isclose(ycalc_cnts, 0.0))] = 1e-4  # penalise fit but avoid overflow
    # evaluate poisson deviation Eq. 1 in
    # T.A. Laurence, B. Chromy (2009) Report LLNL-JRNL-420247; https://www.osti.gov/servlets/purl/991824
    chisq_p = 2 * (np.sum(ycalc_cnts - y_cnts) - np.sum(y_cnts[inonzero] * np.log(ycalc_cnts[inonzero] / y_cnts[inonzero])))
    return chisq_p


def calc_cost_func(params, profile_func, x, y, weights, cost_func):
    try:
        [profile_func.setParameter(iparam, param) for iparam, param in enumerate(params)]
        ycalc = profile_func(x)
        ycalc[~np.isfinite(ycalc)] = 0  # can get NaNs far from peak in some functions
    except:
        ycalc = np.zeros(y.shape)
    return cost_func(ycalc, y, weights)


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


def get_and_clip_data_arrays(ws, peak_data, pk_tof, nbins):
    x, y, esq, ispecs = peak_data.get_data_arrays()  # 3d arrays [rows x cols x tof]
    x = x[peak_data.irow, peak_data.icol, :]  # take x at peak centre, should be same for all detectors
    ispec = ispecs[peak_data.irow, peak_data.icol]
    # crop data array to TOF region of peak
    itof = ws.yIndexOfX(pk_tof, int(ispec))  # need index in y now (note x values are points even if were edges)
    tof_slice = slice(
        int(np.clip(itof - nbins // 2, a_min=0, a_max=len(x))),
        int(np.clip(itof + nbins // 2, a_min=0, a_max=len(x))),
    )
    x = x[tof_slice]
    y = y[:, :, tof_slice]
    esq = esq[:, :, tof_slice]
    return x, y, esq, ispecs


# register algorithm with mantid
AlgorithmFactory.subscribe(IntegratePeaks1DProfile)
