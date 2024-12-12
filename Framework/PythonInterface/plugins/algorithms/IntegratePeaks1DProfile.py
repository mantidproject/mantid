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
    MultiDomainFunction,
    AnalysisDataService as ADS,
)
from mantid.kernel import (
    Direction,
    FloatBoundedValidator,
    IntBoundedValidator,
    StringListValidator,
    EnabledWhenProperty,
    PropertyCriterion,
    StringArrayProperty,
    UnitParams,
)
from mantid.dataobjects import Workspace2D
from mantid.fitfunctions import FunctionWrapper, CompositeFunctionWrapper
import numpy as np
from scipy.ndimage import distance_transform_edt, label
from plugins.algorithms.IntegratePeaksSkew import InstrumentArrayConverter, get_fwhm_from_back_to_back_params, PeakData
from plugins.algorithms.IntegratePeaksShoeboxTOF import get_bin_width_at_tof, set_peak_intensity
from enum import Enum
from typing import Sequence, Tuple


class PEAK_STATUS(Enum):
    VALID = "Valid Peak"
    ON_EDGE = "Peak on detector edge"
    NO_PEAK = "No peak found"


MIN_TOF_WIDTH = 1e-3
MIN_INTENS_OVER_SIGMA = 0.5


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
            validator=FloatBoundedValidator(lower=MIN_INTENS_OVER_SIGMA),
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
        fit_kwargs = {
            "Minimizer": "Levenberg-Marquardt",
            "MaxIterations": 5000,
            "StepSizeMethod": "Sqrt epsilon",
            "IgnoreInvalidData": False,
            "CreateOutput": True,
            "OutputCompositeMembers": True,
        }
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
            peak_intens, peak_sigma = 0.0, 0.0
            status = PEAK_STATUS.NO_PEAK

            detid = peak.getDetectorID()
            bank_name = peaks.column("BankName")[ipk]

            # get fit range
            pk_tof = peak.getTOF()
            ispec_pk = ws.getIndicesFromDetectorIDs([detid])[0]
            if get_nbins_from_b2bexp_params:
                fit_width = nfwhm * get_fwhm_from_back_to_back_params(peak, ws, detid)
            else:
                bin_width = get_bin_width_at_tof(ws, ispec_pk, pk_tof)
                fit_width = self.getProperty("NBins").value * bin_width
            if peak_func_name == "BackToBackExponential":
                # take into account asymmetry of peak function in choosing window
                tof_start = pk_tof - fit_width / 3
            else:
                tof_start = pk_tof - fit_width / 2
            tof_end = tof_start + fit_width

            # check TOF is in limits of x-axis
            xdim = ws.getXDimension()
            if xdim.getMinimum() > tof_start or xdim.getMaximum() < tof_end:
                continue  # skip peak

            # get detector IDs in peak region
            peak_data = array_converter.get_peak_data(peak, detid, bank_name, nrows, ncols, nrows_edge, ncols_edge)

            # fit with constrained peak centers
            func_generator = PeakFunctionGenerator(peak_params_to_fix)
            initial_function, md_fit_kwargs, initial_peak_mask = func_generator.get_initial_fit_function_and_kwargs(
                ws, peak_data, peak.getDSpacing(), (tof_start, tof_end), peak_func_name, bg_func_name
            )
            if not initial_peak_mask.any():
                continue  # no peak
            fit_result = self.exec_fit(initial_function, **fit_kwargs, **md_fit_kwargs)
            if not fit_result["success"]:
                self.delete_fit_result_workspaces(fit_result)
                continue  # skip peak

            # update peak mask based on I/sig from fit
            *_, i_over_sigma = calc_intens_and_sigma_arrays(fit_result, error_strategy)
            non_bg_mask = np.zeros(peak_data.detids.shape, dtype=bool)
            non_bg_mask.flat[initial_peak_mask] = i_over_sigma > i_over_sig_threshold
            peak_mask = find_peak_cluster_in_window(non_bg_mask, (peak_data.irow, peak_data.icol))
            if not peak_mask.any():
                continue  # no peak

            is_on_edge = np.any(np.logical_and(peak_mask, peak_data.det_edges))
            if is_on_edge:
                status = PEAK_STATUS.ON_EDGE
                if not integrate_on_edge:
                    self.delete_fit_result_workspaces(fit_result)
                    continue  # skip peak

            # fit only peak pixels and let peak centers vary independently of DIFC ratio
            fit_mask = peak_mask.flat[initial_peak_mask]  # get bool for domains to be fitted from peak mask
            final_function = func_generator.get_final_fit_function(fit_result["Function"], fit_mask, frac_dspac_delta)
            fit_result = self.exec_fit(final_function, **fit_kwargs, **md_fit_kwargs)
            if not fit_result["success"]:
                self.delete_fit_result_workspaces(fit_result)
                continue  # skip peak

            # calculate intensity
            status = PEAK_STATUS.VALID
            intens, sigma, _ = calc_intens_and_sigma_arrays(fit_result, error_strategy)
            peak_intens = np.sum(intens[fit_mask])
            peak_sigma = np.sqrt(np.sum(sigma[fit_mask] ** 2))

            if output_file:
                intens_over_sig = peak_intens / peak_sigma if peak_sigma > 0 else 0.0
                results.append(
                    LineProfileResult(
                        ipk,
                        peak,
                        intens_over_sig,
                        status,
                        peak_mask,
                        fit_mask,
                        func_generator.ysum,
                        fit_result,
                        peak_data,
                    )
                )
            set_peak_intensity(peak, peak_intens, peak_sigma, do_lorz_cor)

            # delete fit workspaces
            self.delete_fit_result_workspaces(fit_result)

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

    def exec_fit(self, function, **kwargs):
        alg = self.createChildAlgorithm("Fit", enableLogging=False)
        alg.initialize()
        alg.setAlwaysStoreInADS(True)
        alg.setProperty("Function", function)  # needs to be done first
        alg.setProperties(kwargs)
        fit_result = {"success": False}
        try:
            alg.execute()
            fit_result["Function"] = alg.getProperty("Function").value  # getPropertyValue returns FunctionProperty not IFunction
            status = alg.getPropertyValue("OutputStatus")
            fit_result["success"] = status == "success" or status == "Changes in parameter value are too small"
            for prop in ("OutputWorkspace", "OutputNormalisedCovarianceMatrix", "OutputParameters"):
                fit_result[prop] = alg.getPropertyValue(prop)
            return fit_result
        except:
            pass
        return fit_result

    def delete_fit_result_workspaces(self, fit_result: dict):
        wsnames = [fit_result[field] for field in ("OutputWorkspace", "OutputNormalisedCovarianceMatrix", "OutputParameters")]
        self.exec_child_alg("DeleteWorkspaces", WorkspaceList=wsnames)


class PeakFunctionGenerator:
    def __init__(self, peak_params_to_fix: Sequence[str]):
        self.cen_par_name: str = None
        self.intens_par_name: str = None
        self.peak_params_to_fix: Sequence[str] = peak_params_to_fix
        self.peak_mask: np.ndarray[float] = None
        self.ysum: np.ndarray[float] = None

    def get_initial_fit_function_and_kwargs(
        self, ws: Workspace2D, peak_data: PeakData, dpk: float, tof_range: tuple[float, float], peak_func_name: str, bg_func_name: str
    ) -> str:
        ispecs = ws.getIndicesFromDetectorIDs([int(d) for d in peak_data.detids.flat])
        tof_start, tof_end = tof_range
        function = MultiDomainFunction()
        si = ws.spectrumInfo()
        fit_kwargs = {}
        # estimate background
        istart = ws.yIndexOfX(tof_start)
        iend = ws.yIndexOfX(tof_end)
        # init bg func (global)
        bg_func = FunctionFactory.createFunction(bg_func_name)
        # init peak func
        peak_func = FunctionFactory.Instance().createPeakFunction(peak_func_name)
        # save parameter names for future ties/constraints
        peak_func.setIntensity(1.0)
        self.intens_par_name = next(peak_func.getParamName(ipar) for ipar in range(peak_func.nParams()) if peak_func.isExplicitlySet(ipar))
        self.cen_par_name = peak_func.getCentreParameterName()
        avg_bg = 0
        idom = 0
        peak_mask = np.zeros(len(ispecs), dtype=bool)
        self.ysum = np.zeros(peak_data.detids.shape)  # required for plotting later
        for ii, ispec in enumerate(ispecs):
            # check stats in pixel
            intens, sigma, bg = self._estimate_intensity_and_background(ws, ispec, istart, iend)
            self.ysum.flat[ii] = ws.readY(ispec)[istart:iend].sum()
            avg_bg += bg
            peak_mask[ii] = sigma > 0 and intens / sigma > MIN_INTENS_OVER_SIGMA  # low threshold for initial fit
            if peak_mask[ii]:
                # add peak
                difc = si.diffractometerConstants(int(ispec))[UnitParams.difc]
                peak_func.setCentre(difc * dpk)
                peak_func.setIntensity(intens)
                comp_func = CompositeFunctionWrapper(FunctionWrapper(peak_func), FunctionWrapper(bg_func), NumDeriv=True)
                function.add(comp_func.function)
                function.setDomainIndex(idom, idom)
                key_suffix = f"_{idom}" if idom > 0 else ""
                fit_kwargs["InputWorkspace" + key_suffix] = ws.name()
                fit_kwargs["StartX" + key_suffix] = tof_start
                fit_kwargs["EndX" + key_suffix] = tof_end
                fit_kwargs["WorkspaceIndex" + key_suffix] = int(ispec)
                idom += 1
        # set background (background global tied to first domain function)
        function[0][1]["A0"] = avg_bg / len(ispecs)
        # set instrument specific parameters
        iset_initial = np.array([function[0][0].isExplicitlySet(ipar) for ipar in range(peak_func.nParams())])
        ispec_pk = np.ravel_multi_index([peak_data.irow, peak_data.icol], peak_data.detids.shape)
        function[0][0].setMatrixWorkspace(ws, int(ispec_pk), 0, 0)
        iset_final = np.array([function[0][0].isExplicitlySet(ipar) for ipar in range(peak_func.nParams())])
        ipars_to_tie = np.flatnonzero(np.logical_not(iset_initial, iset_final))
        pars_to_tie = [function[0][0].parameterName(int(ipar)) for ipar in ipars_to_tie]  # global peak parameters
        return self._add_parameter_ties_and_constraints(function, pars_to_tie), fit_kwargs, peak_mask

    @staticmethod
    def _estimate_intensity_and_background(ws: Workspace2D, ispec: int, istart: int, iend: int) -> Tuple[float, float, float]:
        bin_width = np.diff(ws.readX(ispec)[istart:iend])
        bin_width = np.hstack((bin_width, bin_width[-1]))  # easier than checking iend and istart not out of bounds
        y = ws.readY(ispec)[istart:iend]
        if not np.any(y > 0):
            return 0.0, 0.0, 0.0
        e = ws.readE(ispec)[istart:iend]
        ibg, _ = PeakData.find_bg_pts_seed_skew(y)
        bg = np.mean(y[ibg])
        intensity = np.sum((y - bg) * bin_width)
        sigma = np.sqrt(np.sum((e * bin_width) ** 2))
        return intensity, sigma, bg

    def _add_parameter_ties_and_constraints(self, function: MultiDomainFunction, pars_to_tie: Sequence[str]) -> str:
        # fix peak params requested
        [function[0][0].fixParameter(par) for par in self.peak_params_to_fix]
        additional_pars_to_fix = set(self.peak_params_to_fix) - set(pars_to_tie)
        ties = []
        for idom in range(1, function.nDomains()):
            # tie global params to first
            for par in pars_to_tie:
                ties.append(f"f{idom}.f0.{par}=f0.f0.{par}")  # global peak pars
            for ipar_bg in range(function[idom][1].nParams()):
                par = function[idom][1].getParamName(ipar_bg)
                ties.append(f"f{idom}.f1.{par}=f0.f1.{par}")
            # tie center using ratio of DIFC
            ratio = function[idom][0][self.cen_par_name] / function[0][0][self.cen_par_name]
            ties.append(f"f{idom}.f0.{self.cen_par_name}={ratio}*f0.f0.{self.cen_par_name}")
            for par in additional_pars_to_fix:
                # pars to be fixed but not global/already tied
                function[idom][0].fixParameter(par)
        # add ties as string (orders of magnitude quicker than self.function.tie)
        return f"{str(function)};ties=({','.join(ties)})"

    def get_final_fit_function(self, function: MultiDomainFunction, peak_mask: np.ndarray[bool], frac_dspac_delta: float) -> str:
        function[0][0].freeAll()
        [function[0][0].fixParameter(par_name) for par_name in self.peak_params_to_fix]
        idom_peak = np.argmax(peak_mask)  # first domain containing peak
        for idom, comp_func in enumerate(function):
            # reset ties on peak function
            function.removeTie(f"f{idom}.f0.{self.cen_par_name}")
            if not peak_mask.flat[idom]:
                comp_func[0][self.intens_par_name] = 0
                comp_func[0].fixParameter(self.intens_par_name)
                comp_func[0].fixParameter(self.cen_par_name)
                for ipar_bg in range(comp_func[1].nParams()):
                    par = comp_func[1].getParamName(ipar_bg)
                    function.removeTie(f"f{idom}.f1.{par}")
                    comp_func[1].fixParameter(par)
            else:
                function.removeTie(f"f{idom}.f0.{self.cen_par_name}")
                xcen_lo = comp_func[0][self.cen_par_name] * (1 - frac_dspac_delta)
                xcen_hi = comp_func[0][self.cen_par_name] * (1 + frac_dspac_delta)
                comp_func.addConstraints(f"{xcen_lo}<f0.{self.cen_par_name}<{xcen_hi}")
            # reset ties on background
            for ipar_bg in range(comp_func[1].nParams()):
                par = comp_func[1].getParamName(ipar_bg)
                if idom > 0:
                    function.removeTie(f"f{idom}.f1.{par}")
                if not peak_mask.flat[idom]:
                    comp_func[1].fixParameter(par)
                elif idom != idom_peak:
                    function.tie(f"f{idom}.f1.{par}", f"f{idom_peak}.f1.{par}")
        return str(function)


class LineProfileResult:
    """
    This class holds result of line profile integration of a single-crystal Bragg peak
    """

    def __init__(
        self,
        ipk: int,
        pk: IPeak,
        intens_over_sig: float,
        status: Enum,
        peak_mask: np.ndarray[bool],
        fit_mask: np.ndarray[bool],
        ysum: np.ndarray[float],
        fit_result: dict,
        peak_data: PeakData,
    ):
        self.irow, self.icol = peak_data.irow, peak_data.icol
        self.tofs = None
        self.ysum = ysum
        self.yfoc = None
        self.efoc = None
        self.yfoc_fit = None
        self.peak_mask = peak_mask
        # extract peak properties into title
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
        self._init_foc_data(fit_result, fit_mask)

    def _init_foc_data(self, fit_result, fit_mask):
        ndoms = fit_result["Function"].nDomains()
        ydat = np.array([get_eval_ws(fit_result["OutputWorkspace"], idom).readY(0) for idom in range(ndoms) if fit_mask[idom]])
        edat_sq = np.array([get_eval_ws(fit_result["OutputWorkspace"], idom).readE(0) for idom in range(ndoms) if fit_mask[idom]]) ** 2
        yfit = np.array([get_eval_ws(fit_result["OutputWorkspace"], idom).readY(1) for idom in range(ndoms) if fit_mask[idom]])
        self.tofs = get_eval_ws(fit_result["OutputWorkspace"], 0).readX(0)
        self.tofs = 0.5 * (self.tofs[1:] + self.tofs[:-1])
        self.yfoc = ydat.sum(axis=0)
        self.efoc = np.sqrt(edat_sq.sum(axis=0))
        self.yfoc_fit = yfit.sum(axis=0)

    def plot_integrated_peak(self, fig, axes, norm_func):
        # plot colorfill of TOF integrated data on LHS
        im = axes[0].imshow(self.ysum)
        im.set_norm(norm_func())
        axes[0].plot(*np.where(self.peak_mask)[::-1], "ow")
        axes[0].plot(self.icol, self.irow, "+r")  # peak centre
        axes[0].set_xlabel("Col")
        axes[0].set_ylabel("Row")
        # plot focussed spectrum
        axes[1].errorbar(self.tofs, self.yfoc, yerr=self.efoc, marker="o", color="k", capsize=2, ls="")
        axes[1].plot(self.tofs, self.yfoc_fit, "-r")
        axes[1].set_xlabel("TOF (mus)")
        axes[1].set_ylabel("Intensity (a.u.)")
        fig.colorbar(im, ax=axes[0], location="left", label="Intensity (a.u.)")
        # set title
        fig.suptitle(self.title)


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


def find_peak_cluster_in_window(mask, predicted_pos):
    labels, nlabels = label(mask)
    if nlabels < 1:
        return np.zeros(mask.shape, dtype=bool)  # no peak found
    peak_label = labels[predicted_pos]
    if peak_label == 0:
        inearest = distance_transform_edt(labels == 0, return_distances=False, return_indices=True)
        peak_label = labels[tuple(inearest)][predicted_pos]
    return labels == peak_label


def calc_sigma_from_summation(xdat, edat_sq, ypeak, cutoff=0.025):
    nbins = len(ypeak)
    ypeak_cumsum = np.cumsum(abs(ypeak))
    ypeak_cumsum /= ypeak_cumsum[-1]
    ilo = np.clip(np.argmin(abs(ypeak_cumsum - cutoff)), a_min=0, a_max=nbins // 2)
    ihi = np.clip(np.argmin(abs(ypeak_cumsum - (1 - cutoff))), a_min=nbins // 2, a_max=nbins - 1) + 1
    bin_width = np.diff(xdat[ilo : ihi + 1])
    return np.sqrt(np.sum(edat_sq[ilo:ihi] * (bin_width**2)))


def calc_intens_and_sigma_arrays(fit_result, error_strategy):
    function = fit_result["Function"]
    intens = np.zeros(function.nDomains())
    sigma = np.zeros(intens.shape)
    intens_over_sig = np.zeros(intens.shape)
    peak_func = FunctionFactory.Instance().createPeakFunction(function[0][0].name())
    for idom, comp_func in enumerate(function):
        [peak_func.setParameter(iparam, comp_func.getParameterValue(iparam)) for iparam in range(peak_func.nParams())]
        intens[idom] = peak_func.intensity()
        if error_strategy == "Hessian":
            [peak_func.setError(iparam, comp_func.getError(iparam)) for iparam in range(peak_func.nParams())]
            sigma[idom] = peak_func.intensityError()
        else:
            ws_fit = get_eval_ws(fit_result["OutputWorkspace"], idom)
            sigma[idom] = calc_sigma_from_summation(ws_fit.readX(0), ws_fit.readE(0) ** 2, ws_fit.readY(3))
    ivalid = ~np.isclose(sigma, 0)
    intens_over_sig[ivalid] = intens[ivalid] / sigma[ivalid]
    return intens, sigma, intens_over_sig


def get_eval_ws(out_ws_name, idom):
    return ADS.retrieve(f"{out_ws_name[:-1]}_{idom}")


# register algorithm with mantid
AlgorithmFactory.subscribe(IntegratePeaks1DProfile)
