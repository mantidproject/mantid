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
# The focused data is in d-spacing, which is detector-independent, so a single peak centre works for
# every spectrum and no TOF round-trip is needed.  Starting values for the peak shape parameters are
# seeded from the high-SNR summed-spectrum fit (fit_initial_summed_spectra), keeping the good seeding
# of the existing routine while delegating the heavy per-spectrum fitting to the parallel algorithm.
#
# The shared seeding, cropping and small helpers live in fitting_utils; this module composes them.

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
)
from mantid.api import AnalysisDataService as ADS, FunctionFactory
from mantid.dataobjects import Workspace2D

from Engineering.EnggUtils import convert_TOFerror_to_derror

from .fitting_utils import (
    _PEAK_CENTRE_PARAM,
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
    """Build one per-(ws, peak) output table (matching the multidomain engine layout: columns
    wsindex, I_est, then for each peak-function parameter p: p, p_err, p/p_err) from de-interleaved
    slices of a giant FitPeaks result.  param_slices/err_slices map each parameter name to a numpy
    array of length num_spec (this workspace's rows already sliced out of the combined result), and
    i_est_vals/fit_mask are the matching per-spectrum arrays.  X0 is already in d-spacing (FitPeaks
    fits the d-spacing workspace) so, unlike the multidomain path, it needs no TOF->d conversion."""
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
    then convert to TOF so the fit runs in the same domain as the multidomain engine (peak-shape
    params A, B are then loaded from the instrument parameter file in TOF by FitPeaks'
    setMatrixWorkspace - the same source the multidomain engine uses)."""
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
    workspace, and a single FitPeaks call fits that peak across all of them at once.  FitPeaks
    parallelises over spectra with OpenMP, so this one giant call runs (n_workspaces * n_spectra)
    peak fits concurrently - the maximum parallel width FitPeaks offers (folding the different peaks
    into the same call would not widen it, as each thread fits its spectrum's peaks sequentially, and
    would forfeit the per-peak window).  The flat result table is then de-interleaved back into the
    per-(ws, peak) parameter tables produced by the multidomain engine.

    The fit is done in TOF (the combined d-spacing workspace is converted to TOF) so the fitted peak
    area (I) is in the same units as the multidomain engine; A and B are loaded from the instrument
    parameter file by FitPeaks, as in the multidomain engine.  X0 is converted back to d-spacing for
    the output table.  Rebunch-smoothing is used only to guide the fit: each peak is fit on the
    progressively finer rebunched (higher-SNR) versions solely to refine the per-spectrum centre seed,
    then the final, authoritative fit is on the raw (unsmoothed) data over the full peak window, seeded
    with that centre.  Only the raw fit is reported - spectra where it fails are left unfit rather than
    falling back to a smoothed result, since a rebunched fit's peak position carries the bias of its
    coarser binning.
    Remaining differences from the multidomain engine are inherent to FitPeaks: weighted "Least
    squares" cost (vs unweighted), independent per-spectrum A/B (vs tied-then-fixed), and a
    positive-area validity check (vs the post-fit I/sigma mask).

    last_fit_ic mirrors the multidomain engine: the smoothing/centre-refinement passes fit the
    requested peak_func_name, then the final authoritative (reported) fit switches to
    IkedaCarpenterPV, seeded from a preceding raw fit with the requested function.  As with A/B above,
    FitPeaks fits IC's instrument-dependent parameters per-spectrum (loaded from the parameter file)
    rather than fixing them as the multidomain engine does."""
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

        combined_ws = f"__fitpeaks_combined_{ipeak}"
        combined_tof = _combine_peak_crops_to_tof(all_peak_crop_wss[ipeak], combined_ws)

        # d-spacing peak centre / window from the summed fit, mapped to per-spectrum TOF
        x0_lo, x0_hi = x0_lims[ipeak]
        centre = 0.5 * (x0_lo + x0_hi)
        xmin, xmax = peak - peak_window, peak + peak_window
        si = combined_tof.spectrumInfo()
        n_total = si.size()
        tof_centre, tof_lo, tof_hi = _compute_tof_windows(si, centre, xmin, xmax)

        # every pass (smooth and raw) fits the full peak window; the window is created once here, only
        # the per-spectrum centre seed changes between passes
        centres_ws = f"__fitpeaks_centres_{ipeak}"
        windows_ws = f"__fitpeaks_windows_{ipeak}"
        window_x = np.empty(2 * n_total)
        window_x[0::2] = tof_lo
        window_x[1::2] = tof_hi
        CreateWorkspace(DataX=window_x, DataY=np.zeros(2 * n_total), NSpec=n_total, OutputWorkspace=windows_ws)

        # a single position tolerance (TOF) generous enough to span each spectrum's window, so the
        # per-spectrum fit window is the effective bound (mirrors the multidomain position freedom)
        pos_tol = float(np.max(np.maximum(tof_hi - tof_centre, tof_centre - tof_lo)))

        if n_ws == 0 or n_total % n_ws != 0:
            raise RuntimeError(
                f"Combined workspace has {n_total} spectra for {n_ws} workspace(s); cannot de-interleave - "
                f"spectra counts must be uniform across workspaces."
            )
        # AppendSpectra concatenates workspaces in order, so combined row (iws*n_spec + ispec)
        n_spec = n_total // n_ws

        param_tab_name = f"__fitpeaks_params_{ipeak}"
        err_tab_name = f"__fitpeaks_errs_{ipeak}"

        def _run_fitpeaks_pass(fit_ws: str, centre_seed: np.ndarray, pass_peak_func_name: str):
            """Run one FitPeaks pass over all spectra (full peak window), seeded with the given
            per-spectrum TOF centres, fitting pass_peak_func_name.  Returns (param_table,
            error_table, valid_mask)."""
            CreateWorkspace(DataX=centre_seed, DataY=np.zeros(n_total), NSpec=n_total, OutputWorkspace=centres_ws)
            FitPeaks(
                InputWorkspace=fit_ws,
                PeakCentersWorkspace=centres_ws,
                FitPeakWindowWorkspace=windows_ws,
                PeakFunction=pass_peak_func_name,
                BackgroundType="Linear",
                # PositionTolerance rejects fits whose centre drifts out of the window (a post-fit
                # check, independent of ConstrainPeakPositions).  ConstrainPeakPositions is left OFF:
                # it adds a +/-0.5*FWHM boundary constraint on the centre, and Mantid implements
                # constraints as penalty terms whose curvature is folded into the Hessian that
                # CalcErrors inverts - so a weak peak pushed against that boundary gets a spuriously
                # tiny (near-singular) position error.  Without it the fit reports a genuine covariance
                # error, and PositionTolerance still bounds the result.
                PositionTolerance=[pos_tol],
                ConstrainPeakPositions=False,
                CopyLastGoodPeakParameters=False,
                RespectFixedPeakParameters=True,
                StrictConvergence=False,
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
                FittedPeaksWorkspace=f"__fitpeaks_model_{ipeak}",
                OutputWorkspace=f"__fitpeaks_pos_{ipeak}",
            )
            param_table = ADS.retrieve(param_tab_name)
            error_table = ADS.retrieve(err_tab_name)
            chi2 = np.asarray(param_table.column("chi2"), dtype=float)
            i_col = np.asarray(param_table.column("I"), dtype=float)
            # a fit is usable if it converged (finite, non-sentinel chi2) with a positive peak area
            valid = np.isfinite(chi2) & (chi2 < _FITPEAKS_BAD_CHI2) & (chi2 > 0) & (i_col > 0)
            return param_table, error_table, valid

        def _refine_centre_seed(centre_seed: np.ndarray, valid: np.ndarray) -> np.ndarray:
            """Carry a pass's fitted centre forward as the next pass's seed.  Only accept a refined
            centre that lies inside the window it was fitted in - with ConstrainPeakPositions off the
            fitted X0 can drift past the data edge, and FitPeaks rejects (fatally) a seed centre
            outside the window on the next pass / raw fit."""
            x0_pass = np.asarray(ADS.retrieve(param_tab_name).column("X0"), dtype=float)
            refine = valid & np.isfinite(x0_pass) & (x0_pass > tof_lo) & (x0_pass < tof_hi)
            centre_seed[refine] = x0_pass[refine]
            return centre_seed

        # Rebunch-smoothing is used ONLY to guide the raw fit's starting centre - a rebunched fit's peak
        # position carries the bias of its coarser binning, so its parameters are never reported.  Fit
        # the progressively finer rebunched (higher-SNR) versions coarsest-first, carrying forward the
        # fitted centre so a weak peak gets a well-located seed.  The final, authoritative fit is on the
        # raw (unsmoothed) data over the full window, seeded with that refined centre; only it is
        # reported, and spectra where it fails are left unfit (no smoothed fallback).
        centre_seed = tof_centre.copy()
        if _PEAK_CENTRE_PARAM in seed_peak_param_names:
            for sv in sorted((int(s) for s in smooth_vals), reverse=True):  # coarsest first
                sm_ws = f"__fitpeaks_smooth_{ipeak}_{sv}"
                Rebunch(InputWorkspace=combined_tof, OutputWorkspace=sm_ws, NBunch=sv)
                _, _, valid = _run_fitpeaks_pass(sm_ws, centre_seed, peak_func_name)
                centre_seed = _refine_centre_seed(centre_seed, valid)

        # last_fit_ic: refine the centre once more with a raw fit of the requested function before the
        # authoritative fit switches to IC (mirrors the multidomain engine seeding IC from a raw fit)
        if final_peak_func_name != peak_func_name:
            _, _, valid = _run_fitpeaks_pass(combined_tof, centre_seed, peak_func_name)
            centre_seed = _refine_centre_seed(centre_seed, valid)

        # authoritative raw fit over the full window, seeded with the refined centres
        param_cols = {p: np.zeros(n_total) for p in peak_param_names}
        err_cols = {p: np.full(n_total, np.inf) for p in peak_param_names}
        param_table, error_table, fit_mask_all = _run_fitpeaks_pass(combined_tof, centre_seed, final_peak_func_name)
        for p in peak_param_names:
            param_cols[p][fit_mask_all] = np.asarray(param_table.column(p), dtype=float)[fit_mask_all]
            err_cols[p][fit_mask_all] = np.asarray(error_table.column(p), dtype=float)[fit_mask_all]

        # convert the fitted centre (and its error) from TOF back to d-spacing per spectrum, so the
        # output table matches the multidomain engine (which also reports X0 in d-spacing)
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

        i_all = param_cols["I"]

        for iws in range(n_ws):
            run, prefix, grouping = ws_meta[iws]
            sl = slice(iws * n_spec, (iws + 1) * n_spec)
            param_slices = {p: param_cols[p][sl] for p in peak_param_names}
            err_slices = {p: err_cols[p][sl] for p in peak_param_names}

            out_ws = f"{prefix}{run}_{peak}_{grouping}_Fit_Parameters"
            out_tab = CreateEmptyTableWorkspace(OutputWorkspace=out_ws)
            _populate_fitpeaks_output_table(
                out_tab, n_spec, peak_param_names, param_slices, err_slices, i_all[sl], fit_mask_all[sl], no_fit_value_dict, nan_replacement
            )

            out_file = out_ws + ".nxs"
            out_path = _fit_parameters_path(save_dir, override_dir, grouping, peak, out_file)
            SaveNexus(InputWorkspace=out_ws, Filename=out_path)
