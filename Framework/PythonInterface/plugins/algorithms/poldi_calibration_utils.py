# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Forward model of POLDI folded counts and its fit, used by PoldiCalibrateForwardModel.

Each pixel i sees every reflection k through every chopper slit s as a Gaussian peak in time, folded onto one chopper
cycle of length T, and fitted with a per-spectrum scale s_i and flat background b_i:

    y_i(t) = s_i * sum_k A_k phi(2 d_k sin(theta_i)) sum_s G(t; (c_i d_k + tz + f_s T) mod T, sigma_ik) + b_i
    sigma_ik^2 = (kappa T)^2 + (sigma_rel c_i d_k)^2

phi is the incident flux, c_i the pixel's chopper-path DIFC, f_s the slit's offset as a fraction of the cycle and tz
the timing offset. The geometry enters only through c_i and theta_i."""

from __future__ import annotations

import copy
from collections.abc import Sequence
from typing import TYPE_CHECKING

import numpy as np
from scipy.special import erf

from plugins.algorithms.calibration_utils import (
    BankGeometry,
    NormalEquationSums,
    RigidBanks,
    ScaleBackgroundFit,
    WeightedCounts,
    project_out,
    weighted_residuals,
)
from plugins.algorithms.diffraction_utils import get_tths_and_l2s
from plugins.algorithms.poldi_utils import _get_flux_arrays, get_instrument_settings_from_log, get_t0_parameters, get_tof_d1Ang

if TYPE_CHECKING:
    from mantid.api import MatrixWorkspace

CHOPPER = "chopper"
LAMBDA_RANGE = (1.0, 5.99)  # Angstrom, inside the IDF's WavelengthDistribution table (0.986 - 5.994)


class PixelGeometry(BankGeometry):
    """BankGeometry with POLDI's chopper slits and incident flux, and a DIFC covering the chopper-detector path."""

    def __init__(self, ws: MatrixWorkspace, bank_names: Sequence[str]):
        cycle_time, slit_offsets, _, l1_chop = get_instrument_settings_from_log(ws)
        super().__init__(ws, bank_names, ws.spectrumInfo().l1() - l1_chop)
        comp_info = ws.componentInfo()
        t0 = get_t0_parameters(comp_info, comp_info.indexOfAny(CHOPPER))[0]
        self.slit_cycle_fractions = slit_offsets / cycle_time - t0  # slit positions as a fraction of the cycle
        self.initial_difc = get_tof_d1Ang(ws, get_tths_and_l2s(ws, self.live_spectra)[1], l1_chop, self.live_spectra)
        self.flux_wavelengths, self.flux = _get_flux_arrays(ws, *LAMBDA_RANGE, n_points=500)
        self.flux_slopes = np.diff(self.flux) / np.diff(self.flux_wavelengths)  # dflux/dlambda on each table segment


class FoldedData(WeightedCounts):
    """Counts on one folded chopper cycle of cycle_time us."""

    def __init__(self, counts: np.ndarray[float], cycle_time: float):
        super().__init__(counts)
        self.cycle_time = cycle_time


class TimingModel:
    """Maps the timing parameters to one tz per dataset: tz = params_to_tz @ params."""

    def __init__(self, kind: str, cycle_times: Sequence[float], speeds: Sequence[float], t0: float, t0_const: float):
        cycle_times = np.asarray(cycle_times, dtype=float)
        self.kind = kind
        if kind == "Linear":
            self.params_to_tz = np.column_stack([cycle_times, np.ones_like(cycle_times)])
            self.initial_params = np.array([t0, t0_const])
            self.param_names = ["t0", "t0_const"]
        else:  # PerChopperSpeed: datasets of the same speed share tz
            self.chopper_speeds = sorted(set(speeds))
            self.params_to_tz = np.array([[float(s == u) for u in self.chopper_speeds] for s in speeds])
            self.initial_params = np.array([t0 * cycle_times[list(speeds).index(u)] + t0_const for u in self.chopper_speeds])
            self.param_names = [f"tz({u:g} rpm)" for u in self.chopper_speeds]

    def calibration_table_rows(self, params: np.ndarray[float]) -> list[tuple[str, float, float]]:
        """(parameter, value, chopper speed or 0 for every speed) rows of the chopper in the calibration table."""
        if self.kind == "Linear":
            return [("t0", params[0], 0.0), ("t0_const", params[1], 0.0)]
        return [("t0", 0.0, 0.0)] + [("t0_const", value, speed) for speed, value in zip(self.chopper_speeds, params)]


def _sum_into_bins(flat_bins: np.ndarray[int], values: np.ndarray[float], n_flat: int) -> np.ndarray[float]:
    """Add each value into its flat (spectrum, bin) slot of a length-n_flat array. Values aimed at the same slot add up,
    which `out[flat_bins] += values` would not do."""
    return np.bincount(flat_bins.ravel(), values.ravel(), minlength=n_flat)


def model_counts(
    geom: PixelGeometry,
    data: FoldedData,
    c: np.ndarray[float],
    sin_t: np.ndarray[float],
    tz: float,
    kappa: float,
    srel: float,
    A: np.ndarray[float],
    d_hkl: np.ndarray[float],
    deriv: bool = False,
    nsig: float = 5.0,
) -> np.ndarray[float] | tuple[np.ndarray[float], ...]:
    """Predicted counts m (nspec, nbins). With deriv, also dm/dc (nspec, nbins), dm/dsin_t (nspec, nbins), dm/dtz,
    dm/dkappa, dm/dsrel and dm/dA (K, nspec, nbins).

    c and sin_t are per spectrum, A and d_hkl per reflection. In the shapes below, n is the number of pixels a reflection
    reaches, S the number of chopper slits and W the number of bins evaluated around each peak."""
    nspec, n_bins = data.counts.shape
    T = data.cycle_time  # cycle time (us)
    bin_width = T / n_bins
    n_flat = nspec * n_bins  # m and its derivatives are built flat over (spectrum, bin), then reshaped
    live = np.flatnonzero(geom.live_spectra)  # spectrum indices of the unmasked pixels
    m = np.zeros(n_flat)  # predicted counts
    if deriv:
        # c_i and sin_t_i only affect spectrum i, so dm/dc and dm/dsin_t need one value per (spectrum, bin), like m
        dm_dc, dm_dsin, dm_dtz, dm_dkappa, dm_dsrel = (np.zeros(n_flat) for _ in range(5))
        dm_dA = np.zeros((d_hkl.size, n_flat))
    sig_chopper = kappa * T  # chopper-opening part of the peak width, the same for every pixel
    for k, d_k in enumerate(d_hkl):
        # phi: incident flux at the wavelength each live pixel sees reflection k at, lambda = 2 d_k sin(theta_i) by
        # Bragg's law; zero outside the flux table
        wavelengths = 2.0 * d_k * sin_t[live]
        phi = np.interp(wavelengths, geom.flux_wavelengths, geom.flux, left=0.0, right=0.0)
        reached = phi > 0
        if not reached.any():
            continue
        pix = live[reached]  # spectrum index of each pixel the reflection reaches, (n,)
        c_i, phi_i = c[pix], phi[reached]  # (n,)
        sig = np.sqrt(sig_chopper**2 + (srel * c_i * d_k) ** 2)  # peak width (us), (n,)
        # arrival time via each slit (TOF + timing offset + slit opening time), folded onto one cycle, (n, S)
        centre = np.mod(c_i[:, None] * d_k + tz + geom.slit_cycle_fractions[None, :] * T, T)
        # bins within nsig of each centre, sized for the widest peak so every window has W bins, (n, S, W).
        # Left unwrapped: near the cycle ends they run below 0 or past n_bins and still measure the true distance to the centre
        # and capped at half a cycle, so a wild trial step on a width parameter cannot allocate huge arrays
        half_window = min(int(np.ceil(nsig * sig.max() / bin_width)) + 1, n_bins // 2)
        bins = np.floor(centre / bin_width).astype(int)[..., None] + np.arange(-half_window, half_window + 1)
        # left and right bin edges, in standard deviations from the peak centre
        u_lo = (bins * bin_width - centre[..., None]) / sig[:, None, None]
        u_hi = u_lo + bin_width / sig[:, None, None]
        # G: fraction of the unit-area Gaussian falling in each bin, Phi(u_hi) - Phi(u_lo), (n, S, W)
        G = 0.5 * (erf(u_hi / np.sqrt(2.0)) - erf(u_lo / np.sqrt(2.0)))
        flat_bins = pix[:, None, None] * n_bins + np.mod(bins, n_bins)  # wraps the tails round the cycle
        peak_area = (A[k] * phi_i)[:, None, None]  # integrated counts of each pixel's peak
        m += _sum_into_bins(flat_bins, peak_area * G, n_flat)
        if deriv:
            # standard normal density at the bin edges, since dPhi/du = pdf(u)
            pdf_lo, pdf_hi = np.exp(-0.5 * u_lo**2) / np.sqrt(2 * np.pi), np.exp(-0.5 * u_hi**2) / np.sqrt(2 * np.pi)
            # u = (edge - centre) / sig, so du/dcentre = -1/sig and du/dsig = -u/sig
            dG_dcentre = -(pdf_hi - pdf_lo) / sig[:, None, None]
            dG_dsig = -(u_hi * pdf_hi - u_lo * pdf_lo) / sig[:, None, None]
            # differentiating sig^2 = (kappa T)^2 + (srel c d)^2
            dsig_dc = (srel**2 * c_i * d_k**2 / sig)[:, None, None]
            dsig_dsrel = (srel * (c_i * d_k) ** 2 / sig)[:, None, None]
            dsig_dkappa = (kappa * T**2 / sig)[:, None, None]
            dm_dA[k] += _sum_into_bins(flat_bins, phi_i[:, None, None] * G, n_flat)  # m is linear in A_k
            dm_dtz += _sum_into_bins(flat_bins, peak_area * dG_dcentre, n_flat)  # dcentre/dtz = 1
            # c both moves the peak (dcentre/dc = d_k) and widens it
            dm_dc += _sum_into_bins(flat_bins, peak_area * (dG_dcentre * d_k + dG_dsig * dsig_dc), n_flat)
            # sin_t sets the wavelength the pixel sees the reflection at, and so its flux: dphi/dsin_t = 2 d_k dphi/dlambda
            segment = np.maximum(np.searchsorted(geom.flux_wavelengths, wavelengths[reached]) - 1, 0)
            dm_dsin += _sum_into_bins(flat_bins, (A[k] * 2.0 * d_k * geom.flux_slopes[segment])[:, None, None] * G, n_flat)
            # the width parameters act only through sig
            dm_dsrel += _sum_into_bins(flat_bins, peak_area * dG_dsig * dsig_dsrel, n_flat)
            dm_dkappa += _sum_into_bins(flat_bins, peak_area * dG_dsig * dsig_dkappa, n_flat)
    shape = (nspec, n_bins)
    if not deriv:
        return m.reshape(shape)
    # kappa before srel, the order CalibrationFit._projected_jacobian takes them in
    return (*(v.reshape(shape) for v in (m, dm_dc, dm_dsin, dm_dtz, dm_dkappa, dm_dsrel)), dm_dA.reshape((d_hkl.size, *shape)))


class CalibrationFit(ScaleBackgroundFit):
    """Levenberg-Marquardt fit of the forward model to folded counts.

    Parameters, in order: geometry q | timing parameters | kappa, sigma_rel | reflection intensities A.
    `start`, a previous fit of the same data, seeds everything after the geometry."""

    def __init__(
        self,
        geometry: PixelGeometry,
        datasets: Sequence[FoldedData],
        geometry_model: RigidBanks,
        d_hkl: np.ndarray[float],
        timing_model: TimingModel,
        kappa: float = 1e-3,
        srel: float = 1.5e-3,
        start: CalibrationFit | None = None,
    ):
        self.geometry, self.geometry_model, self.timing_model = geometry, geometry_model, timing_model
        # copies whose masked spectra have zero weight, so they do not contribute even when the mask didn't zero their counts
        self.datasets = [copy.copy(ds) for ds in datasets]
        for ds in self.datasets:
            ds.weights = np.where(geometry.live_spectra[:, None], ds.weights, 0.0)
        # drop reflections no live pixel can reach: their intensity column would be all zero
        _, sin_t = geometry.difc_and_sin_theta(geometry.initial_bank_params)
        lam = 2 * np.outer(d_hkl, sin_t[geometry.live_spectra])
        reachable = ((lam >= geometry.flux_wavelengths[0]) & (lam <= geometry.flux_wavelengths[-1])).any(axis=1)
        self.d_hkl = np.asarray(d_hkl)[reachable]
        if not self.d_hkl.size:
            raise ValueError("None of the reflections reaches a live pixel inside the flux wavelength range.")
        self.n_geometry_params, self.n_timing_params = geometry_model.initial_params.size, timing_model.initial_params.size
        self.params = np.concatenate([geometry_model.initial_params, timing_model.initial_params, [kappa, srel], np.ones(self.d_hkl.size)])
        self.param_names = (
            list(geometry_model.param_names) + timing_model.param_names + ["kappa", "sigma_rel"] + [f"I(d={d:.4f})" for d in self.d_hkl]
        )
        self.is_free = np.ones(self.params.size, bool)
        if start is None:
            self._estimate_initial_intensities()
        else:
            n_geom = self.n_geometry_params
            if start.param_names[start.n_geometry_params :] != self.param_names[n_geom:]:
                raise ValueError("start must be a fit of the same timing parameters and reflections.")
            self.params[n_geom:], self.is_free[n_geom:] = start.params[start.n_geometry_params :], start.is_free[start.n_geometry_params :]

    def split_params(self, x: np.ndarray[float]) -> tuple[np.ndarray[float], np.ndarray[float], float, float, np.ndarray[float]]:
        nq, ntz = self.n_geometry_params, self.n_timing_params
        return x[:nq], x[nq : nq + ntz], x[nq + ntz], x[nq + ntz + 1], x[nq + ntz + 2 :]

    @property
    def timing_params(self) -> np.ndarray[float]:
        return self.split_params(self.params)[1]

    @property
    def bank_params(self) -> np.ndarray[float]:
        return self.geometry_model.split_bank_and_block_params(self.split_params(self.params)[0])[0]

    @property
    def block_params(self) -> np.ndarray[float] | None:
        return self.geometry_model.split_bank_and_block_params(self.split_params(self.params)[0])[1]

    def _estimate_initial_intensities(self) -> None:
        """Linear least-squares estimate of A (with per-spectrum backgrounds and unit scales). The strongest reflection
        is then fixed: the overall scale lives in the per-spectrum scales."""
        q, x_tz, kappa, srel, _ = self.split_params(self.params)
        c, sin_t = self.geometry.difc_and_sin_theta(*self.geometry_model.split_bank_and_block_params(q))
        K = self.d_hkl.size
        normal, rhs = np.zeros((K, K)), np.zeros(K)
        for ds, tz in zip(self.datasets, self.timing_model.params_to_tz @ x_tz):
            # dm/dA_k is reflection k's model at unit intensity
            dA = np.stack([model_counts(self.geometry, ds, c, sin_t, tz, kappa, srel, np.ones(1), self.d_hkl[k : k + 1]) for k in range(K)])
            w2 = ds.weights**2
            w_sum = np.maximum(w2.sum(1, keepdims=True), 1e-300)
            y_centred = ds.counts - (w2 * ds.counts).sum(1, keepdims=True) / w_sum
            B = ((dA - (w2 * dA).sum(2, keepdims=True) / w_sum) * ds.weights).reshape(K, -1)
            normal += B @ B.T
            rhs += B @ (ds.weights * y_centred).ravel()
        A = np.linalg.lstsq(normal, rhs, rcond=None)[0]
        if not A.any():  # only when every spectrum is flat; otherwise the clip below keeps A positive
            raise ValueError("The counts have no peaks to estimate the reflection intensities from.")
        A = np.clip(A, 1e-3 * np.abs(A).max(), None)
        first_intensity = self.n_geometry_params + self.n_timing_params + 2
        self.params[first_intensity:] = A / A.max()
        self.is_free[first_intensity + int(np.argmax(A))] = False

    def evaluate_cost(self, x: np.ndarray[float]) -> float:
        """Cost 0.5 |r|^2 over every dataset, r being the weighted residuals at each spectrum's best scale and
        background."""
        q, x_tz, kappa, srel, A = self.split_params(x)
        c, sin_t = self.geometry.difc_and_sin_theta(*self.geometry_model.split_bank_and_block_params(q))
        cost = 0.0
        for ds, tz in zip(self.datasets, self.timing_model.params_to_tz @ x_tz):
            m = model_counts(self.geometry, ds, c, sin_t, tz, kappa, srel, A, self.d_hkl)
            cost += 0.5 * np.sum(weighted_residuals(ds, m)[0] ** 2)
        return cost

    def evaluate_cost_and_derivatives(self, x: np.ndarray[float]) -> tuple[float, np.ndarray[float], np.ndarray[float]]:
        """Cost as evaluate_cost, with the Gauss-Newton normal matrix J^T J (approximating the Hessian) and gradient
        J^T r over all of x. Also keeps the residuals at x, for the reduced chi-squared."""
        q, x_tz, kappa, srel, A = self.split_params(x)
        # the geometry acts on the counts only through each pixel's c and sin_t (see NormalEquationSums)
        c, sin_t, dpixel_dq = self.geometry_model.difc_and_derivatives(q)
        sums = NormalEquationSums(c.size, x.size - self.n_geometry_params)
        self.residuals = []
        for j, (ds, tz) in enumerate(zip(self.datasets, self.timing_model.params_to_tz @ x_tz)):
            m, *dm_dparams = model_counts(self.geometry, ds, c, sin_t, tz, kappa, srel, A, self.d_hkl, deriv=True)
            r, scale = weighted_residuals(ds, m)
            self.residuals.append(r)
            sums.add(*self._projected_jacobian(j, ds, m, scale, *dm_dparams), r)
        cost = sum(0.5 * np.sum(r**2) for r in self.residuals)
        return cost, *sums.assemble(dpixel_dq)

    def _projected_jacobian(
        self,
        j: int,
        data: FoldedData,
        m: np.ndarray[float],
        scale: np.ndarray[float],
        dm_dc: np.ndarray[float],
        dm_dsin: np.ndarray[float],
        dm_dtz: np.ndarray[float],
        dm_dkappa: np.ndarray[float],
        dm_dsrel: np.ndarray[float],
        dm_dA: np.ndarray[float],
    ) -> tuple[np.ndarray[float], np.ndarray[float]]:
        """Jacobian dr/dparameter for dataset j: J_pixel (2, nspec, nbins), the compact columns for each pixel's c and
        sin_t (see NormalEquationSums), and J_other (n_other, nspec, nbins) for the timing parameters, kappa, sigma_rel
        and the intensities, in the order of x."""

        def project(dm):
            # r = w (y - s m - b), so dr = -s w dm, less the part a re-fitted s and b would cancel (see project_out)
            return -scale[:, None] * project_out(dm, m, data.weights)

        J_tz = project(dm_dtz)
        # dataset j's tz = params_to_tz[j] @ timing parameters, so each timing column is dr/dtz times its entry
        timing_cols = [J_tz * self.timing_model.params_to_tz[j, a] for a in range(self.n_timing_params)]
        J_other = np.stack(timing_cols + [project(dm_dkappa), project(dm_dsrel)] + [project(dA) for dA in dm_dA])
        return np.stack([project(dm_dc), project(dm_dsin)]), J_other
