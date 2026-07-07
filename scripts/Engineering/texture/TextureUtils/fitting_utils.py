# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

# Shared core for the texture peak-fitting workflow: the small conversion/IO helpers, the
# crop/rebin routines, the summed-spectrum seeding shared by both engines, and the public
# fit_all_peaks entry point that dispatches to one of the two fitting engines:
#   - fitpeaks_engine._fit_all_peaks_fitpeaks       (multithreaded FitPeaks)
#   - multidomain_engine._fit_all_peaks_multidomain (iterative MultiDomainFunction)
# Both engines import the helpers below; the engine modules are imported lazily inside
# fit_all_peaks so this module has no import-time dependency on them.

from os import path
from typing import Sequence, Tuple, List

import numpy as np

from mantid.simpleapi import (
    logger,
    Fit,
    Rebin,
    SumSpectra,
    AppendSpectra,
    CloneWorkspace,
    CropWorkspaceRagged,
)
from mantid.api import FunctionFactory, IFunction, CompositeFunction
from mantid.dataobjects import Workspace2D
from mantid.kernel import DeltaEModeType, UnitConversion

from plugins.algorithms.peakdata_utils import PeakData


# Peak-function parameter names shared across both fitting engines
_PEAK_INTENSITY_PARAM = "I"
_PEAK_CENTRE_PARAM = "X0"


# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ shared small helpers ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


def _param_names(func: IFunction) -> List[str]:
    """Return the ordered list of parameter names of a Mantid function."""
    return [func.getParamName(i) for i in range(func.nParams())]


def _d_to_tof(value: float, diff_consts) -> float:
    """Convert a d-spacing value to TOF for a single detector's diffractometer constants."""
    return UnitConversion.run("dSpacing", "TOF", value, 0, DeltaEModeType.Elastic, diff_consts)


def _tof_to_d(value: float, diff_consts) -> float:
    """Convert a TOF value to d-spacing for a single detector's diffractometer constants."""
    return UnitConversion.run("TOF", "dSpacing", value, 0, DeltaEModeType.Elastic, diff_consts)


def _fit_parameters_path(save_dir: str, override_dir: bool, grouping: str, peak: float, out_file: str) -> str:
    """Path for a per-(ws, peak) fit-parameters file: flat in save_dir when override_dir, else
    nested under save_dir/grouping/peak/."""
    if override_dir:
        return path.join(save_dir, out_file)
    return path.join(save_dir, grouping, str(peak), out_file)


# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ crop / rebin helpers ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


def crop_and_rebin(ws: Workspace2D | str, out_ws: str, lower: float, upper: float, rebin_params: str | Sequence[float]) -> None:
    CropWorkspaceRagged(ws, lower, upper, OutputWorkspace="__tmp_peak_window")
    Rebin("__tmp_peak_window", rebin_params, OutputWorkspace=out_ws)


def _get_min_bin(ws: Workspace2D) -> float:
    return min(np.diff(ws.x(i)).min() for i in range(ws.getNumberHistograms()))


def crop_wss_and_combine(
    wss: Sequence[Workspace2D | str], peak: float, lower: float, upper: float, output: str
) -> Tuple[Workspace2D, List[str]]:
    cropped_rebinned_wss = [f"rebin_ws_{peak}_0"]
    peak_window_ws = CropWorkspaceRagged(wss[0], lower, upper, OutputWorkspace="__peak_window_crop")
    rebin_params = (lower, _get_min_bin(peak_window_ws), upper)
    Rebin("__peak_window_crop", rebin_params, OutputWorkspace=f"rebin_ws_{peak}_0")
    CloneWorkspace(InputWorkspace=f"rebin_ws_{peak}_0", OutputWorkspace=f"rebin_ws_{peak}")
    for iws, ws in enumerate(wss[1:]):
        intermediate_ws = f"rebin_ws_{peak}_{iws + 1}"
        cropped_rebinned_wss.append(intermediate_ws)
        crop_and_rebin(ws, intermediate_ws, lower, upper, rebin_params)
        AppendSpectra(f"rebin_ws_{peak}", intermediate_ws, OutputWorkspace=f"rebin_ws_{peak}")
    return SumSpectra(f"rebin_ws_{peak}", OutputWorkspace=output), cropped_rebinned_wss


def _make_composite(peak_func: IFunction, bg_func: IFunction) -> CompositeFunction:
    """Build a CompositeFunction from existing C++ function objects without
    serialising them (which would discard workspace references, fixed-parameter
    state, etc.).  NumDeriv is NOT set so that each member function uses its own
    derivative method (analytical for LinearBackground, numerical fallback for
    IkedaCarpenterPV) — this avoids redundant expensive peak evaluations when
    computing derivatives for background parameters."""
    comp = FunctionFactory.createFunction("CompositeFunction")
    comp.add(peak_func)
    comp.add(bg_func)
    return comp


# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ summed-spectrum seeding ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


def fit_initial_summed_spectra(
    wss: Sequence[Workspace2D | str],
    peaks: Sequence[float],
    peak_window: float,
    fit_kwargs: dict,
    peak_func_name: str,
    return_shared_params: bool = False,
) -> Tuple[Sequence[Tuple[float, float]], Sequence[Workspace2D]]:
    # return_shared_params: if True, also return (as the middle element of the tuple) a list - one
    # dict per peak - of the peak function parameters fitted on the summed spectrum.  These are used
    # by the FitPeaks engine to seed the per-spectrum fits.  They are unit-consistent with that
    # engine because both fit in the workspace's native (d-spacing) units.
    x0_lims = []
    all_peak_crop_wss = []
    shared_params = []
    for i, peak in enumerate(peaks):
        # set the ws bounds based on the supplied peak window
        low_bound, hi_bound = peak - peak_window, peak + peak_window
        window_ws, peak_crop_wss = crop_wss_and_combine(wss, peak, low_bound, hi_bound, f"peak_window_{i}")

        # the outer list is peak index and the inner list is each ws (str) in wss cropped and rebinned for that peak
        all_peak_crop_wss.append(peak_crop_wss)

        # set up a function to fit
        bg_func = FunctionFactory.createFunction("LinearBackground")
        peak_func = FunctionFactory.Instance().createPeakFunction(peak_func_name)

        # estimate starting params
        intens, sigma, bg, centre = _estimate_intensity_background_and_centre(window_ws, 0, 0, len(window_ws.x(0)) - 1, peak)
        bg_func.setParameter("A0", bg)
        peak_func.setMatrixWorkspace(window_ws, 0, low_bound, hi_bound)
        peak_func.setParameter(_PEAK_CENTRE_PARAM, centre)
        peak_func.setParameter(_PEAK_INTENSITY_PARAM, intens)
        peak_func.addConstraints(f"{low_bound} < {_PEAK_CENTRE_PARAM} < {hi_bound}")
        peak_func.addConstraints(f"{_PEAK_INTENSITY_PARAM} > 0")

        # for IkedaCarpenterPV, fix instrument-dependent parameters during the initial summed fit
        # this fit is only to estimate peak centre - too many free params causes numerical instability
        if peak_func_name == "IkedaCarpenterPV":
            for par in ("Alpha0", "Alpha1", "Beta0", "Kappa"):
                peak_func.fixParameter(par)

        comp_func = _make_composite(peak_func, bg_func)
        fit_kwargs["InputWorkspace"] = window_ws.name()
        fit_kwargs["StartX"] = low_bound
        fit_kwargs["EndX"] = hi_bound

        fit_object = Fit(
            Function=comp_func,
            Output=f"composite_fit_{peak}",
            MaxIterations=50,  # if it hasn't fit in 50 it is likely because the texture has the peak missing
            **fit_kwargs,
        )
        out_peak_func = fit_object.Function.function.getFunction(0)
        x0 = out_peak_func.getParameterValue(_PEAK_CENTRE_PARAM)
        x0_lims.append((x0 * (1 - 3e-3), x0 * (1 + 3e-3)))
        if return_shared_params:
            shared_params.append(
                {out_peak_func.getParamName(ip): out_peak_func.getParameterValue(ip) for ip in range(out_peak_func.nParams())}
            )
    if return_shared_params:
        return x0_lims, shared_params, all_peak_crop_wss
    return x0_lims, all_peak_crop_wss


# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ fit_all_peaks dispatcher ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


def fit_all_peaks(
    wss: Sequence[str],
    peaks: Sequence[float],
    peak_window: float,
    save_dir: str,
    override_dir: bool = False,
    i_over_sigma_thresh: float = 2.0,
    nan_replacement: str | None = "zeros",
    no_fit_value_dict: dict | None = None,
    smooth_vals: Sequence[int] = (3, 2),
    tied_bkgs: Sequence[bool] = (False, True),
    final_fit_raw: bool = True,
    parameters_to_tie: Sequence[str] | None = None,
    subsequent_fit_param_fix: Sequence[str] | None = None,
    peak_func_name: str = "BackToBackExponential",
    last_fit_ic: bool = False,
    max_fit_iters: int = 50,
    engine: str = "fitpeaks",
) -> None:
    """
    Fit all the peaks given in all the spectra of all the workspaces, for use in a texture analysis workflow
    wss: Workspace names of all the workspaces to fit
    peaks: Sequence of peak positions in d-spacing
    peak_window: size of the window to create around the desired peak for purpose of fitting
    save_dir: directory to save the results in
    override_dir: flag which, if True, will save files directly into save_dir rather than creating a folder structure
    i_over_sigma_thresh: I/sig less than this value will be deemed as no peak and parameter values will be nan or specified value
    nan_replacement: method options are ("zero", "min", "max", "mean") will try to replace the nan values in columns
                     zero - will replace all nans with 0.0
                     min/max/mean - will replace all nans in a column with the min/max/mean non-nan value (otherwise will remain nan)
    no_fit_value_dict: allows the user to specify the unfit default value of parameters as a dict of key:value pairs
    smooth_vals: the number of bins which should be combined together to improve SNR stats
    tied_bkgs: a bool flag for each of the subsequent fits whether the background fits should be independent for spectra
    final_fit_raw: flag for whether the final fit should be done with no smoothing
    parameters_to_tie: the shared broadening/shape parameters. On the first (coarsely rebunched)
                       fit these are tied across spectra to establish a single, unit-correct value;
                       every subsequent fit (including the final raw fit) then holds that value
                       FIXED per-spectrum rather than tying, so the domains decouple and the
                       multi-domain solve stays block-diagonal. If None, defaults are used based on
                       peak function:
                       BackToBackExponential: ("A", "B"), IkedaCarpenterPV: ("Alpha0", "Alpha1", "Beta0", "Kappa")
    subsequent_fit_param_fix: parameters which should be fixed after the initial fit (Default is None)
    peak_func_name: peak function to use, should be either BackToBackExponential or IkedaCarpenterPV
    max_fit_iters: maximum number of iterations for a single fit
    engine: fitting engine to use.
            "fitpeaks" (default) - hand each peak to the FitPeaks algorithm, which fits it across all
                spectra of all workspaces at once with OpenMP multithreading.  Fits in TOF (like the
                multidomain engine) but with FitPeaks' weighted "Least squares" cost (it cannot do
                unweighted); rebunch smoothing only refines the per-spectrum centre seed for a final
                raw fit, which is what gets reported.  Faster on many spectra.
            "multidomain" - the original iterative MultiDomainFunction fit (smoothing passes,
                summed-centre seeding, tied-then-fixed broadening) run serially per (ws, peak).
                Retained for comparison/validation and for the unweighted-cost behaviour.
    """

    # currently the only fit functions intended to be used - less flexibility here allows for less user input
    supported_peaks = ("BackToBackExponential", "IkedaCarpenterPV")
    bg_func_name = "LinearBackground"

    if peak_func_name not in supported_peaks:
        logger.warning(
            f"Provided peak function: '{peak_func_name}' not one of the supported peak functions: ({', '.join(supported_peaks)})."
            f" Behaviour may be unreliable."
        )

    # define some parameters for the fit
    fit_kwargs = {
        "StepSizeMethod": "Sqrt epsilon",
        "IgnoreInvalidData": False,
        "CreateOutput": True,
        "OutputCompositeMembers": True,
        "Minimizer": "Levenberg-Marquardt",
        "CostFunction": "Unweighted least squares",
    }

    if engine == "fitpeaks":
        from .fitpeaks_engine import _fit_all_peaks_fitpeaks

        _fit_all_peaks_fitpeaks(
            wss,
            peaks,
            peak_window,
            save_dir,
            override_dir,
            i_over_sigma_thresh,
            nan_replacement,
            no_fit_value_dict,
            peak_func_name,
            max_fit_iters,
            fit_kwargs,
            smooth_vals,
            last_fit_ic,
        )
        return
    elif engine == "multidomain":
        from .multidomain_engine import _fit_all_peaks_multidomain

        _fit_all_peaks_multidomain(
            wss,
            peaks,
            peak_window,
            save_dir,
            override_dir,
            i_over_sigma_thresh,
            nan_replacement,
            no_fit_value_dict,
            smooth_vals,
            tied_bkgs,
            final_fit_raw,
            parameters_to_tie,
            subsequent_fit_param_fix,
            peak_func_name,
            bg_func_name,
            last_fit_ic,
            max_fit_iters,
            fit_kwargs,
        )
        return
    else:
        raise ValueError(f"Unknown fitting engine '{engine}'. Expected 'fitpeaks' or 'multidomain'.")


# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ shared fitting utility functions ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


def _tie_bkg(function, approx_bkgs: np.ndarray, ties: List[str]) -> Tuple[object, List[str]]:
    function[0][1]["A0"] = np.mean(approx_bkgs)
    for idom in range(1, function.nDomains()):
        for ipar_bg in range(function[idom][1].nParams()):
            par = function[idom][1].getParamName(ipar_bg)
            ties.append(f"f{idom}.f1.{par}=f0.f1.{par}")
    return function, ties


def _estimate_intensity_background_and_centre(
    ws: Workspace2D, ispec: int, istart: int, iend: int, peak: float
) -> Tuple[float, float, float, float]:
    xdat = ws.x(ispec)[istart:iend]
    bin_width = np.diff(xdat)
    bin_width = np.hstack((bin_width, bin_width[-1]))  # easier than checking iend and istart not out of bounds
    y = ws.y(ispec)[istart:iend]
    if not np.any(y > 0):
        return 0.0, 0.0, 0.0, peak
    e = ws.e(ispec)[istart:iend]
    ibg, _ = PeakData.find_bg_pts_seed_skew(y)
    bg = np.mean(y[ibg])
    intensity = np.trapezoid((y - bg), xdat)
    sigma = np.sqrt(np.sum((e * bin_width) ** 2))
    centre_arg = np.argmax(y)
    centre = xdat[centre_arg]
    return intensity, sigma, bg, centre


def _get_run_and_prefix_from_ws_log(ws: Workspace2D, wsname: str) -> Tuple[str, str]:
    try:
        run = str(ws.getRun().getLogData("run_number").value)
        prefix = wsname.split(run)[0]
    except:
        run = "unknown"
        prefix = ""
    return run, prefix


def _get_grouping_from_ws_log(ws: Workspace2D) -> str:
    try:
        grouping = str(ws.getRun().getLogData("Grouping").value)
    except RuntimeError:
        grouping = "GROUP"
    return grouping


def get_default_values(params: Sequence[str], no_fit_dict: dict | None) -> dict:
    defaults = dict(zip(params, [np.nan for _ in params]))
    if isinstance(no_fit_dict, dict):
        for k, v in no_fit_dict.items():
            defaults[k] = v
    return defaults


def replace_nans(vals: np.ndarray, method: str | None = None) -> np.ndarray:
    if not method:
        return vals
    if method == "zeros":
        return np.nan_to_num(vals, nan=0)
    func = {"mean": np.nanmean, "max": np.nanmax, "min": np.nanmin}[method]
    out = vals.copy()
    col_stat = func(out, axis=0)
    nan_mask = np.isnan(out)
    out[nan_mask] = col_stat[np.where(nan_mask)[1]]
    return out
