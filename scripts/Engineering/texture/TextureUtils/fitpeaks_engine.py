# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

# FitPeaks (multithreaded) fitting engine for the texture peak-fitting workflow.
#
# FitPeaks is a C++ algorithm that fits a set of peaks across every spectrum of a workspace and
# parallelises over spectra with OpenMP (no GIL, no ADS contention).  For the texture workflow we
# call it once per (workspace, peak): one peak fit across all detector spectra, multithreaded.
#
# The focused data is in d-spacing, but the fit itself is done in TOF (the domain the instrument
# parameter file defines the peak-shape parameters in): the combined per-peak crops are converted to
# TOF by _combine_peak_crops_to_tof, and because each detector maps the same d to a different TOF the
# per-spectrum centres and fit windows are computed by _compute_tof_windows and handed to FitPeaks as
# workspaces.  The fitted centre is converted back to d-spacing with _tof_to_d for the output table.
# The centre is seeded from the high-SNR summed-spectrum fit (fit_initial_summed_spectra); the
# peak-shape starting values come from the instrument parameters and are then refined per-spectrum by
# carrying each rebunch-smoothing pass's fitted shape forward (see _fit_all_peaks_fitpeaks), while the
# heavy per-spectrum fitting is delegated to the parallel algorithm.
#
# The shared seeding, cropping and small helpers live in fitting_utils; this module composes them.

from os import makedirs, path
from typing import Sequence, Tuple

import numpy as np

from mantid.simpleapi import (
    SaveNexus,
    logger,
    CreateEmptyTableWorkspace,
    FitPeaks,
    CreateWorkspace,
    ConvertUnits,
    Rebunch,
    CloneWorkspace,
    AppendSpectra,
    EstimatePeakIntensities,
    DeleteWorkspace,
)
from mantid.api import AnalysisDataService as ADS, FunctionFactory
from mantid.dataobjects import Workspace2D

from Engineering.EnggUtils import convert_TOFerror_to_derror

from .fitting_utils import (
    _PEAK_CENTRE_PARAM,
    _PEAK_INTENSITY_PARAM,
    _param_names,
    _d_to_tof,
    _tof_to_d,
    _fit_parameters_path,
    fit_initial_summed_spectra,
    _get_run_and_prefix_from_ws_log,
    _get_grouping_from_ws_log,
    get_default_values,
    replace_nans,
)

# large sentinel: FitPeaks leaves chi2 at DBL_MAX for peaks it rejected/failed to fit
_FITPEAKS_BAD_CHI2 = 1e300

# fractional bound on the fitted centre applied to every fit pass - the guiding rebunch-smoothing
# passes and the authoritative raw fit alike: each centre is constrained (PositionToleranceMode=
# "Constrain" with PositionToleranceFractional=True) to within this fraction of its per-spectrum fit
# window width of the seeded centre, so a low-SNR fit cannot pull the centre far from the summed-fit
# position.
_WINDOW_TOL_FRAC = 0.05


def _populate_fitpeaks_output_table(
    out_tab,
    num_spec: int,
    peak_param_names: Sequence[str],
    param_slices: dict,
    err_slices: dict,
    i_est_vals: np.ndarray,
    fit_mask: np.ndarray,
    no_fit_value_dict: dict | None,
    nan_replacement: str | None,
) -> None:
    """Build one per-(ws, peak) output table (columns wsindex, I_est, then for each peak-function
    parameter p: p, p_err, p/p_err) from de-interleaved slices of a giant FitPeaks result.
    param_slices/err_slices map each parameter name to a numpy array of length num_spec (this
    workspace's rows already sliced out of the combined result), and i_est_vals/fit_mask are the
    matching per-spectrum arrays.  X0 has already been converted from the fitted TOF back to
    d-spacing by the caller, so the slices are written out as they are."""
    out_tab.addColumn("int", "wsindex")
    out_tab.addColumn("double", "I_est")
    for p in peak_param_names:
        out_tab.addColumn("double", p)
        out_tab.addColumn("double", f"{p}_err")
        out_tab.addColumn("double", f"{p}/{p}_err")

    default_vals = get_default_values(peak_param_names, no_fit_value_dict)

    table_vals = np.zeros((num_spec, 3 * len(peak_param_names) + 1))
    for ispec in range(num_spec):
        if fit_mask[ispec]:
            row = [i_est_vals[ispec]]
            for p in peak_param_names:
                val, err = param_slices[p][ispec], err_slices[p][ispec]
                # a genuinely singular fit can still return a non-finite or zero parameter error;
                # guard the ratio so the output never contains nan/inf - report the value with an
                # infinite error and a zero ratio, matching the unfit-spectrum convention.
                if np.isfinite(val) and np.isfinite(err) and err > 0:
                    row += [val, err, val / err]
                else:
                    row += [val if np.isfinite(val) else default_vals[p], np.inf, 0.0]
        else:
            row = [default_vals.get("I_est", np.nan)]
            for p in peak_param_names:
                row += [default_vals[p], np.inf, 0.0]
        table_vals[ispec] = row

    if nan_replacement:
        table_vals = replace_nans(table_vals, nan_replacement)
    for i, row in enumerate(table_vals):
        out_tab.addRow([i] + list(row))


def _combine_peak_crops_to_tof(per_ws_crops: Sequence[str], combined_ws: str) -> Workspace2D:
    """Combine every workspace's (common-grid, d-spacing) spectra for this peak into one workspace,
    then convert to TOF so the fit runs in the domain the instrument parameter file is defined in
    (peak-shape params A, B are loaded from it in TOF by FitPeaks' setMatrixWorkspace)."""
    CloneWorkspace(InputWorkspace=per_ws_crops[0], OutputWorkspace=combined_ws)
    for crop_ws in per_ws_crops[1:]:
        AppendSpectra(combined_ws, crop_ws, OutputWorkspace=combined_ws)
    return ConvertUnits(InputWorkspace=combined_ws, OutputWorkspace=combined_ws, Target="TOF")


def _compute_tof_windows(spectrum_info, centre: float, xmin: float, xmax: float) -> Tuple[np.ndarray, np.ndarray, np.ndarray]:
    """Per-spectrum TOF centre/window arrays for a peak whose centre and window are given in
    d-spacing.  Each detector maps the same d to a different TOF, so FitPeaks needs per-spectrum
    centres and windows supplied via workspaces."""
    n_total = spectrum_info.size()
    tof_centre = np.empty(n_total)
    tof_lo = np.empty(n_total)
    tof_hi = np.empty(n_total)
    for k in range(n_total):
        dc = spectrum_info.diffractometerConstants(k)
        tof_centre[k] = _d_to_tof(centre, dc)
        tof_lo[k] = _d_to_tof(xmin, dc)
        tof_hi[k] = _d_to_tof(xmax, dc)
    return tof_centre, tof_lo, tof_hi


def _estimate_peak_intensities(ws: Workspace2D, windows_ws: str) -> np.ndarray:
    """Per-spectrum, fit-independent peak-area estimate for the I_est column: the trapezoidal
    integral of (data - background) over each spectrum's TOF window.  It is deliberately independent
    of the fitted I (rather than a copy of it) so the two provide a real cross-check.

    The per-spectrum skew-background + windowed integral is delegated to the multithreaded C++
    EstimatePeakIntensities algorithm, reusing the same per-spectrum window workspace (FitPeaks
    FitPeakWindowWorkspace convention) that guides the fit passes.  A single peak is estimated here, so
    the result table's Intensity column is already in spectrum order."""
    table_ws = "__fitpeaks_i_est_table"
    try:
        table = EstimatePeakIntensities(InputWorkspace=ws, PeakWindowWorkspace=windows_ws, OutputWorkspace=table_ws)
        i_est = np.asarray(table.column("Intensity"), dtype=float)
    finally:
        if ADS.doesExist(table_ws):
            DeleteWorkspace(table_ws)
    return i_est


def _refine_centre_seed(
    param_tab_name: str, centre_seed: np.ndarray, valid: np.ndarray, tof_lo: np.ndarray, tof_hi: np.ndarray
) -> np.ndarray:
    """Carry a pass's fitted centre (from the param_tab_name FitPeaks parameter table) forward as the
    next pass's seed.  Only accept a refined centre that lies inside the window it was fitted in -
    with ConstrainPeakPositions off the fitted X0 can drift past the data edge, and FitPeaks rejects
    (fatally) a seed centre outside the window on the next pass / raw fit."""
    x0_pass = np.asarray(ADS.retrieve(param_tab_name).column(_PEAK_CENTRE_PARAM), dtype=float)
    refine = valid & np.isfinite(x0_pass) & (x0_pass > tof_lo) & (x0_pass < tof_hi)
    centre_seed[refine] = x0_pass[refine]
    return centre_seed


def _build_seed_table(param_tab_name: str, seed_tab_name: str, seed_carry_names: Sequence[str], valid: np.ndarray, n_total: int):
    """Build the per-spectrum PeakParameterValueTable for the next pass from the pass just run
    (param_tab_name).  One column per carried shape parameter and one row per spectrum in workspace
    index order (FitPeaks maps table row -> workspace index).  A spectrum whose fit was not valid (or
    a non-finite fitted value) is written as NaN, which FitPeaks reads as 'no seed for this
    spectrum/parameter' and leaves at the value from the instrument parameters - so a failed guiding
    fit never poisons the next pass's shape seed.  Returns the table name, or None when there is
    nothing to carry (the next pass then seeds its shape from the instrument parameters)."""
    if not seed_carry_names:
        return None
    src = ADS.retrieve(param_tab_name)
    cols = {p: np.asarray(src.column(p), dtype=float) for p in seed_carry_names}
    tab = CreateEmptyTableWorkspace(OutputWorkspace=seed_tab_name)
    for p in seed_carry_names:
        tab.addColumn("double", p)
    for ispec in range(n_total):
        if valid[ispec]:
            tab.addRow([v if np.isfinite(v := cols[p][ispec]) else np.nan for p in seed_carry_names])
        else:
            tab.addRow([np.nan] * len(seed_carry_names))
    return seed_tab_name


def _fit_all_peaks_fitpeaks(
    wss: Sequence[str],
    peaks: Sequence[float],
    peak_window: float,
    save_dir: str,
    override_dir: bool,
    i_over_sigma_thresh: float,
    nan_replacement: str | None,
    no_fit_value_dict: dict | None,
    peak_func_name: str,
    max_fit_iters: int,
    fit_kwargs: dict,
    smooth_vals: Sequence[int] = (3, 2),
    last_fit_ic: bool = False,
) -> None:
    """FitPeaks-based (multithreaded) implementation of fit_all_peaks.

    For each peak, every workspace's spectra are combined (they were already cropped+rebinned onto a
    common grid and appended by crop_wss_and_combine during the summed-spectrum seeding step) into one
    workspace, and a single FitPeaks call fits that peak across all of them at once.

    The fit is done in TOF with X0 converted back to d-spacing for the output table.
    Rebunch-smoothing guides the fit for poor SNR: each peak is fit on the progressively finer
    rebunched (higher-SNR) versions, coarsest-first, and each pass's per-spectrum result is carried
    forward as the next pass's starting values - the fitted centre (via PeakCentersWorkspace) and the
    fitted peak-shape parameters (via a per-spectrum PeakParameterValueTable, excluding the centre and
    the rebunch-scale-dependent intensity).  The final, authoritative fit is on the raw (unsmoothed)
    data over the full window, seeded with those carried centre and shape; only it is reported, and
    spectra where it fails are left unfit (no smoothed fallback).  Every pass (smoothing and final)
    constrains the peak centre to within +/-10% of its per-spectrum seed so a low-SNR fit cannot pull
    it far from the summed-fit position.

    Weak peaks are rejected the same way as the multidomain engine: after the final fit,
    spectra whose fitted I/sigma is at or below i_over_sigma_thresh are treated as "no peak" and get
    the unfit defaults."""

    # x0 seeds (d-spacing) and the per-workspace cropped+rebinned (common-grid) ws names per peak
    x0_lims, all_peak_crop_wss = fit_initial_summed_spectra(wss, peaks, peak_window, fit_kwargs.copy(), peak_func_name)

    # last_fit_ic: the smoothing/centre-refinement passes use the requested function; the final
    # authoritative fit switches to IkedaCarpenterPV (unless already fitting IC)
    final_peak_func_name = "IkedaCarpenterPV" if last_fit_ic and peak_func_name != "IkedaCarpenterPV" else peak_func_name

    # raw peak-function parameter names (excluding background): seed_* names guide the refinement
    # passes; peak_param_names (from the final function) drive the authoritative fit and output table
    seed_base_peak_func = FunctionFactory.Instance().createPeakFunction(peak_func_name)
    seed_peak_param_names = _param_names(seed_base_peak_func)
    base_peak_func = FunctionFactory.Instance().createPeakFunction(final_peak_func_name)
    peak_param_names = _param_names(base_peak_func)

    # peak-shape parameters carried per-spectrum from one smoothing pass to the next (as the next
    # pass's PeakParameterValueTable starting values).  The centre is excluded - it is seeded/refined
    # per-spectrum through PeakCentersWorkspace (a table X0 would override that).  The intensity is
    # excluded too - it is a peak area whose scale changes with the rebunch bin width, so a value
    # fitted at one NBunch mis-seeds the next; FitPeaks re-estimates it per-spectrum each pass.  What
    # remains (the decay/width shape, e.g. A/B/S or the alphas/sigma/gamma) is binning-invariant in
    # TOF, so carrying the higher-SNR fitted value forward gives a weak peak a better shape seed.
    seed_carry_names = [p for p in seed_peak_param_names if p not in (_PEAK_CENTRE_PARAM, _PEAK_INTENSITY_PARAM)]

    # FitPeaks requires MaxFitIterations >= 49
    max_fit_iters = max(49, max_fit_iters)

    n_ws = len(wss)
    # per-workspace metadata for output naming (retrieve each raw workspace once for its run logs)
    ws_meta = []
    for wsname in wss:
        ws = ADS.retrieve(wsname)
        run, prefix = _get_run_and_prefix_from_ws_log(ws, wsname)
        ws_meta.append((run, prefix, _get_grouping_from_ws_log(ws)))

    for ipeak, peak in enumerate(peaks):
        logger.notice(f"Fitting peak {peak} across all {n_ws} workspace(s) ({ipeak + 1}/{len(peaks)})")

        # every ADS workspace this peak creates, removed in the finally block below so a failure
        # part way through a peak does not leave temporaries behind either
        temp_wss = {f"peak_window_{ipeak}", f"rebin_ws_{peak}", *all_peak_crop_wss[ipeak]}
        try:
            combined_ws = f"__fitpeaks_combined_{ipeak}"
            temp_wss.add(combined_ws)
            combined_tof = _combine_peak_crops_to_tof(all_peak_crop_wss[ipeak], combined_ws)

            # d-spacing peak centre / window from the summed fit, mapped to per-spectrum TOF
            x0_lo, x0_hi = x0_lims[ipeak]
            centre = 0.5 * (x0_lo + x0_hi)
            xmin, xmax = peak - peak_window, peak + peak_window
            si = combined_tof.spectrumInfo()
            n_total = si.size()
            tof_centre, tof_lo, tof_hi = _compute_tof_windows(si, centre, xmin, xmax)

            # every fit pass (smooth and raw) and the I_est estimate share one per-spectrum window
            # workspace (FitPeaks FitPeakWindowWorkspace convention: [lo, hi] per spectrum); only the
            # per-spectrum centre seed changes between fit passes, so the window is created once here
            centres_ws = f"__fitpeaks_centres_{ipeak}"
            windows_ws = f"__fitpeaks_windows_{ipeak}"
            temp_wss.update((centres_ws, windows_ws))
            window_x = np.empty(2 * n_total)
            window_x[0::2] = tof_lo
            window_x[1::2] = tof_hi
            CreateWorkspace(DataX=window_x, DataY=np.zeros(2 * n_total), NSpec=n_total, OutputWorkspace=windows_ws)

            # fit-independent per-spectrum peak-area estimate (I_est) over the same windows, from the raw
            # data - computed once here since it does not depend on the fit result
            i_est_all = _estimate_peak_intensities(combined_tof, windows_ws)

            if n_ws == 0 or n_total % n_ws != 0:
                raise RuntimeError(
                    f"Combined workspace has {n_total} spectra for {n_ws} workspace(s); cannot de-interleave - "
                    f"spectra counts must be uniform across workspaces."
                )
            # AppendSpectra concatenates workspaces in order, so combined row (iws*n_spec + ispec)
            n_spec = n_total // n_ws

            param_tab_name = f"__fitpeaks_params_{ipeak}"
            err_tab_name = f"__fitpeaks_errs_{ipeak}"
            seed_tab_name = f"__fitpeaks_seed_{ipeak}"

            def _run_fitpeaks_pass(
                fit_ws: str | Workspace2D,
                centre_seed: np.ndarray,
                pass_peak_func_name: str,
                seed_table: str | None = None,
            ):
                """Run one FitPeaks pass over all spectra (full peak window), seeded with the given
                per-spectrum TOF centres, fitting pass_peak_func_name.  The fitted centre is bounded to
                seed +/- _WINDOW_TOL_FRAC of each spectrum's own fit window width (PositionToleranceMode=
                "Constrain" with PositionToleranceFractional=True, which lets FitPeaks scale the single
                fraction per spectrum), so a low-SNR fit cannot pull the centre far from its seed.
                seed_table, if given, is a PeakParameterValueTable of per-spectrum shape starting values
                (from the previous pass, see _build_seed_table); its columns must be parameters of
                pass_peak_func_name, so the caller only supplies it for passes fitting that function.
                Returns (param_table, error_table, valid_mask)."""
                CreateWorkspace(DataX=centre_seed, DataY=np.zeros(n_total), NSpec=n_total, OutputWorkspace=centres_ws)
                # per-spectrum shape seed: FitPeaks applies these AFTER setCentre, but the table excludes
                # X0 so the per-spectrum PeakCentersWorkspace centre is preserved.  The table property does
                # not resolve a name string via simpleapi, so pass the retrieved TableWorkspace object.
                shape_seed_kwargs = {"PeakParameterValueTable": ADS.retrieve(seed_table)} if seed_table else {}
                model_ws, pos_ws = f"__{fit_ws}_model_{ipeak}", f"__fitpeaks_pos_{ipeak}"
                temp_wss.update((param_tab_name, err_tab_name, model_ws, pos_ws))
                FitPeaks(
                    InputWorkspace=fit_ws,
                    PeakCentersWorkspace=centres_ws,
                    FitPeakWindowWorkspace=windows_ws,
                    PeakFunction=pass_peak_func_name,
                    BackgroundType="Linear",
                    **shape_seed_kwargs,
                    PositionTolerance=[_WINDOW_TOL_FRAC],
                    PositionToleranceMode="Constrain",
                    PositionToleranceFractional=True,
                    ConstrainPeakPositions=False,
                    CopyLastGoodPeakParameters=False,
                    RespectFixedPeakParameters=True,
                    StrictConvergence=False,
                    CalculateUnconstrainedErrors=True,
                    # the focused peaks sit on a modest background; FitPeaks' high-background peak-stripping
                    # (on by default) over-subtracts and collapses the fitted peak area, so disable it
                    HighBackground=False,
                    Minimizer=fit_kwargs.get("Minimizer", "Levenberg-Marquardt"),
                    CostFunction=fit_kwargs.get("CostFunction", "Unweighted least squares"),
                    MaxFitIterations=max_fit_iters,
                    # rely on the post-fit validity check (positive area + finite chi2) rather than
                    # FitPeaks' internal signal-to-sigma pre-check, which rejected genuine weak peaks
                    MinimumSignalToSigmaRatio=0,
                    RawPeakParameters=True,
                    OutputPeakParametersWorkspace=param_tab_name,
                    OutputParameterFitErrorsWorkspace=err_tab_name,
                    FittedPeaksWorkspace=model_ws,
                    OutputWorkspace=pos_ws,
                )
                param_table = ADS.retrieve(param_tab_name)
                error_table = ADS.retrieve(err_tab_name)
                chi2 = np.asarray(param_table.column("chi2"), dtype=float)
                i_col = np.asarray(param_table.column(_PEAK_INTENSITY_PARAM), dtype=float)
                # a fit is usable if it converged (finite, non-sentinel chi2) with a positive peak area
                valid = np.isfinite(chi2) & (chi2 < _FITPEAKS_BAD_CHI2) & (chi2 >= 0) & (i_col > 0)
                return param_table, error_table, valid

            def _carry_pass_forward(centre_seed: np.ndarray, valid: np.ndarray):
                """Carry the pass just run forward: its fitted centres become the next pass's centre seed
                and its fitted shape the next pass's PeakParameterValueTable."""
                centre_seed = _refine_centre_seed(param_tab_name, centre_seed, valid, tof_lo, tof_hi)
                seed_table = _build_seed_table(param_tab_name, seed_tab_name, seed_carry_names, valid, n_total)
                if seed_table:
                    temp_wss.add(seed_table)
                return centre_seed, seed_table

            # Rebunch-smoothing is used ONLY to guide the raw fit's starting centre - a rebunched fit's peak
            # position carries the bias of its coarser binning, so its parameters are never reported.  Fit
            # the progressively finer rebunched (higher-SNR) versions coarsest-first, carrying forward the
            # fitted centre so a weak peak gets a well-located seed.  The final, authoritative fit is on the
            # raw (unsmoothed) data over the full window, seeded with that refined centre; only it is
            # reported, and spectra where it fails are left unfit (no smoothed fallback).
            centre_seed = tof_centre.copy()
            # per-spectrum shape seed carried between passes; None on the first pass (shape then comes
            # from the instrument parameters), then each pass's fitted shape guides the next
            seed_table = None
            if _PEAK_CENTRE_PARAM in seed_peak_param_names:
                for sv in sorted((int(s) for s in smooth_vals), reverse=True):  # coarsest first
                    sm_ws = f"__fitpeaks_smooth_{ipeak}_{sv}"
                    temp_wss.add(sm_ws)
                    Rebunch(InputWorkspace=combined_tof, OutputWorkspace=sm_ws, NBunch=sv)
                    # every pass constrains its centre to a fraction of each spectrum's own fit window so a
                    # coarse, low-SNR rebunched fit cannot drag the centre far from the summed-fit position
                    _, _, valid = _run_fitpeaks_pass(sm_ws, centre_seed, peak_func_name, seed_table=seed_table)
                    # carry this pass's centre and shape into the next
                    centre_seed, seed_table = _carry_pass_forward(centre_seed, valid)

            # last_fit_ic: refine the centre once more with a raw fit of the requested function before the
            # authoritative fit switches to IC
            if final_peak_func_name != peak_func_name:
                _, _, valid = _run_fitpeaks_pass(combined_tof, centre_seed, peak_func_name, seed_table=seed_table)
                centre_seed, seed_table = _carry_pass_forward(centre_seed, valid)

            # authoritative raw fit over the full window, seeded with the refined centres and constrained
            # to a fraction of each spectrum's fit window so the reported centre stays anchored to the summed fit
            param_cols = {p: np.zeros(n_total) for p in peak_param_names}
            err_cols = {p: np.full(n_total, np.inf) for p in peak_param_names}
            param_table, error_table, fit_mask_all = _run_fitpeaks_pass(
                combined_tof,
                centre_seed,
                final_peak_func_name,
                # the carried shape table's columns are peak_func_name's parameters, so it is only a valid
                # seed when the final fit has not switched functions (last_fit_ic); otherwise let FitPeaks
                # estimate IC's shape from the instrument parameters and observation
                seed_table=seed_table if final_peak_func_name == peak_func_name else None,
            )
            for p in peak_param_names:
                param_cols[p][fit_mask_all] = np.asarray(param_table.column(p), dtype=float)[fit_mask_all]
                err_cols[p][fit_mask_all] = np.asarray(error_table.column(p), dtype=float)[fit_mask_all]

            # reject weak peaks: a converged, positive-area fit whose intensity is not statistically
            # significant (I/sigma at or below i_over_sigma_thresh) is treated as "no peak" so its row
            # gets the unfit defaults.  This honours fit_all_peaks' i_over_sigma_thresh contract; sigma
            # here is the fitted intensity's covariance error (I_err), the per-spectrum significance
            # measure FitPeaks provides.
            if _PEAK_INTENSITY_PARAM in peak_param_names:
                i_vals, i_errs = param_cols[_PEAK_INTENSITY_PARAM], err_cols[_PEAK_INTENSITY_PARAM]
                i_over_sigma = np.divide(i_vals, i_errs, out=np.zeros_like(i_vals), where=np.isfinite(i_errs) & (i_errs > 0))
                fit_mask_all = fit_mask_all & (i_over_sigma > i_over_sigma_thresh)

            # convert the fitted centre (and its error) from TOF back to d-spacing per spectrum, so the
            # output table reports X0 in the same units as the input data
            if _PEAK_CENTRE_PARAM in peak_param_names:
                x0_tof, x0_tof_err = param_cols[_PEAK_CENTRE_PARAM], err_cols[_PEAK_CENTRE_PARAM]
                x0_d = np.zeros(n_total)
                x0_d_err = np.zeros(n_total)
                for k in range(n_total):
                    if fit_mask_all[k]:
                        dc = si.diffractometerConstants(k)
                        d_val = _tof_to_d(x0_tof[k], dc)
                        x0_d[k] = d_val
                        x0_d_err[k] = convert_TOFerror_to_derror(dc, x0_tof_err[k], d_val)
                param_cols[_PEAK_CENTRE_PARAM] = x0_d
                err_cols[_PEAK_CENTRE_PARAM] = x0_d_err

            for iws in range(n_ws):
                run, prefix, grouping = ws_meta[iws]
                sl = slice(iws * n_spec, (iws + 1) * n_spec)
                param_slices = {p: param_cols[p][sl] for p in peak_param_names}
                err_slices = {p: err_cols[p][sl] for p in peak_param_names}

                out_ws = f"{prefix}{run}_{peak}_{grouping}_Fit_Parameters"
                out_tab = CreateEmptyTableWorkspace(OutputWorkspace=out_ws)
                _populate_fitpeaks_output_table(
                    out_tab,
                    n_spec,
                    peak_param_names,
                    param_slices,
                    err_slices,
                    i_est_all[sl],
                    fit_mask_all[sl],
                    no_fit_value_dict,
                    nan_replacement,
                )

                out_file = out_ws + ".nxs"
                out_path = _fit_parameters_path(save_dir, override_dir, grouping, peak, out_file)
                # SaveNexus will not create the save_dir/grouping/peak tree itself
                makedirs(path.dirname(out_path), exist_ok=True)
                SaveNexus(InputWorkspace=out_ws, Filename=out_path)

        finally:
            for name in temp_wss:
                if ADS.doesExist(name):
                    DeleteWorkspace(name)
