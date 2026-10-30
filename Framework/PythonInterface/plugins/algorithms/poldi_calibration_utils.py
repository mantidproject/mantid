# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Forward model of POLDI folded counts and its fit, used by PoldiCalibrateForwardModel (whose docs give the model)."""

from __future__ import annotations

from collections.abc import Callable, Sequence
from typing import TYPE_CHECKING

import numpy as np
from scipy.special import erf

from plugins.algorithms.poldi_utils import (
    _get_flux_arrays,
    get_instrument_settings_from_log,
    get_live_spectra,
    get_t0_parameters,
    get_tof_d1Ang,
    get_tths_and_l2s,
)

if TYPE_CHECKING:
    from mantid.api import MatrixWorkspace
    from mantid.geometry import ComponentInfo
    from mantid.kernel import Quat

CHOPPER = "chopper"
LAMBDA_RANGE = (1.0, 5.99)  # Angstrom, inside the IDF's WavelengthDistribution table (0.986 - 5.994)


def rot_y(deg: float) -> np.ndarray[float]:
    a = np.deg2rad(deg)
    return np.array([[np.cos(a), 0.0, np.sin(a)], [0.0, 1.0, 0.0], [-np.sin(a), 0.0, np.cos(a)]])


def _is_ancestor(comp_info: ComponentInfo, ancestor: int, idx: int) -> bool:
    while comp_info.hasParent(idx):
        idx = comp_info.parent(idx)
        if idx == ancestor:
            return True
    return False


def _path_below_root(comp_info: ComponentInfo, idx: int) -> str:
    """'/' path from the child of the instrument root down to idx, e.g. 'SouthBank/reference-sphere-1/block1'."""
    names = []
    while comp_info.hasParent(idx):
        names.append(comp_info.name(idx))
        idx = comp_info.parent(idx)
    return "/".join(reversed(names))


def _rotation_matrix(q: Quat) -> np.ndarray[float]:
    w, x, y, z = q.real(), q.imagI(), q.imagJ(), q.imagK()
    return np.array(
        [
            [1 - 2 * (y * y + z * z), 2 * (x * y - w * z), 2 * (x * z + w * y)],
            [2 * (x * y + w * z), 1 - 2 * (x * x + z * z), 2 * (y * z - w * x)],
            [2 * (x * z - w * y), 2 * (y * z + w * x), 1 - 2 * (x * x + y * y)],
        ]
    )


def component_index(comp_info: ComponentInfo, path: str) -> int:
    """Component index from a name or '/' path (block names repeat in both banks: 'NorthBank/detector-bank/reference-sphere-1/block1')."""
    first, *rest = path.split("/")
    idx = comp_info.indexOfAny(first)
    for name in rest:
        idx = next(int(c) for c in comp_info.children(idx) if comp_info.name(int(c)) == name)
    return idx


class PixelGeometry:
    """Pixel positions in the frame of their bank, so bank and block moves can be applied in numpy.

    Pixels outside `banks` keep their starting DIFC. c_i is the starting Mantid value scaled by the geometric factor
    g = (L_cs + L2) sin(theta), so the absolute constant is always Mantid's."""

    def __init__(self, ws: MatrixWorkspace, banks: Sequence[str]):
        comp_info, spec_info, det_info = ws.componentInfo(), ws.spectrumInfo(), ws.detectorInfo()
        self.live = get_live_spectra(ws)
        _, l2s = get_tths_and_l2s(ws, self.live)
        cycle_time, slit_offsets, _, l1_chop = get_instrument_settings_from_log(ws)
        t0 = get_t0_parameters(comp_info, comp_info.indexOfAny(CHOPPER))[0]
        self.slit_x = slit_offsets / cycle_time - t0  # slit positions as a fraction of the cycle
        self.l_cs = spec_info.l1() - l1_chop
        self.c0 = get_tof_d1Ang(ws, l2s, l1_chop, self.live)
        self.lam, self.flux = _get_flux_arrays(ws, *LAMBDA_RANGE, n_points=500)
        self.sample = np.array(comp_info.samplePosition())
        beam = self.sample - np.array(comp_info.sourcePosition())
        self.beam = beam / np.linalg.norm(beam)
        self.banks = list(banks)
        self.det = np.array([det_info.indexOf(ws.getSpectrum(i).getDetectorIDs()[0]) for i in range(ws.getNumberHistograms())])
        bank_idx = [component_index(comp_info, bank) for bank in self.banks]
        self.bank_of = np.array([next((ib for ib, b in enumerate(bank_idx) if _is_ancestor(comp_info, b, int(d))), -1) for d in self.det])
        # banks are rotated about Y only, so (x, z, roty) with y fixed describes them
        self.bank_y = np.array([comp_info.position(b)[1] for b in bank_idx])
        self.q0 = np.concatenate(
            [[comp_info.position(b)[0], comp_info.position(b)[2], comp_info.rotation(b).getEulerAngles("YZX")[0]] for b in bank_idx]
        )
        self.local = np.zeros((self.det.size, 3))
        for ib, b in enumerate(bank_idx):
            sel = self.bank_of == ib
            rel = np.array([comp_info.position(int(d)) for d in self.det[sel]]) - np.array(comp_info.position(b))
            self.local[sel] = rel @ rot_y(self.q0[3 * ib + 2])  # R^T rel, row-wise
        # detector blocks (modules) of the refined banks: each pixel's parent component
        parents = [comp_info.parent(int(d)) if b >= 0 else -1 for d, b in zip(self.det, self.bank_of)]
        block_idx = sorted({p for p in parents if p >= 0})
        self.block_of = np.array([block_idx.index(p) if p >= 0 else -1 for p in parents])
        self.blocks = [_path_below_root(comp_info, b) for b in block_idx]
        self.block_bank = [int(self.bank_of[self.block_of == m][0]) for m in range(len(block_idx))]
        # in the bank frame; the block component's own position, which is what RotateInstrumentComponent rotates about
        self.block_centre = np.array(
            [
                (np.array(comp_info.position(b)) - comp_info.position(bank_idx[ib])) @ rot_y(self.q0[3 * ib + 2])
                for b, ib in zip(block_idx, self.block_bank)
            ]
        )
        self.g0 = self._g(self.positions(self.q0))[0]

    def positions(self, q_banks: np.ndarray[float], q_blocks: np.ndarray[float] | None = None) -> np.ndarray[float]:
        """Absolute pixel positions after bank moves (x, z, roty per bank) and optional block moves (shift along the
        bank normal, tilt about the vertical through the block centre)."""
        local = self.local if q_blocks is None else self._move_blocks(q_blocks)
        out = np.zeros_like(local)
        for ib in range(len(self.banks)):
            sel = self.bank_of == ib
            x, z, roty = q_banks[3 * ib : 3 * ib + 3]
            out[sel] = local[sel] @ rot_y(roty).T + [x, self.bank_y[ib], z]
        return out

    def _move_blocks(self, q_blocks: np.ndarray[float]) -> np.ndarray[float]:
        local = self.local.copy()
        for m, centre in enumerate(self.block_centre):
            sel = self.block_of == m
            normal, tilt = q_blocks[2 * m : 2 * m + 2]
            local[sel] = (local[sel] - centre) @ rot_y(tilt).T + centre + [0.0, 0.0, normal]
        return local

    def _g(self, positions: np.ndarray[float]) -> tuple[np.ndarray[float], np.ndarray[float]]:
        rel = positions - self.sample
        l2 = np.linalg.norm(rel, axis=1)
        sin_t = np.sqrt(0.5 * (1.0 - rel @ self.beam / np.where(l2 > 0, l2, 1.0)))
        return (self.l_cs + l2) * sin_t, sin_t

    def difc(self, q_banks: np.ndarray[float], q_blocks: np.ndarray[float] | None = None) -> tuple[np.ndarray[float], np.ndarray[float]]:
        """Chopper-path DIFC c_i (us/A) and sin(theta_i) of every spectrum."""
        g, sin_t = self._g(self.positions(q_banks, q_blocks))
        in_bank = self.bank_of >= 0
        return np.where(in_bank, self.c0 * g / np.where(in_bank, self.g0, 1.0), self.c0), sin_t


class RigidBanks:
    """Geometry parameters (x, z, roty) per bank, mapped to (c, sin_t, dc/dq) for the fit."""

    BANK_STEPS = (1e-5, 1e-5, 1e-4)  # m, m, deg

    def __init__(self, geom: PixelGeometry):
        self.geom = geom
        self.q0 = geom.q0.copy()
        self.labels = [f"{bank}.{par}" for bank in geom.banks for par in ("x", "z", "roty")]
        self.steps = np.tile(self.BANK_STEPS, len(geom.banks))

    def __call__(self, q: np.ndarray[float]) -> tuple[np.ndarray[float], np.ndarray[float], np.ndarray[float]]:
        c, sin_t = self.geom.difc(*self.split(q))
        dc_dq = np.empty((c.size, q.size))
        for a, step in enumerate(self.steps):
            dq = np.zeros(q.size)
            dq[a] = step
            dc_dq[:, a] = (self.geom.difc(*self.split(q + dq))[0] - self.geom.difc(*self.split(q - dq))[0]) / (2 * step)
        return c, sin_t, dc_dq

    def split(self, q: np.ndarray[float]) -> tuple[np.ndarray[float], np.ndarray[float] | None]:
        return q, None  # bank parameters, block parameters


class BankModules(RigidBanks):
    """Rigid banks plus, per detector block, a shift along the bank normal and a tilt about the vertical."""

    BLOCK_STEPS = (1e-5, 1e-3)  # m, deg

    def __init__(self, geom: PixelGeometry, q_banks: np.ndarray[float]):
        super().__init__(geom)
        self.n_bank_params = len(q_banks)
        self.q0 = np.concatenate([q_banks, np.zeros(2 * len(geom.blocks))])
        self.labels += [f"{block}.{par}" for block in geom.blocks for par in ("normal", "tilt")]
        self.steps = np.concatenate([self.steps, np.tile(self.BLOCK_STEPS, len(geom.blocks))])

    def split(self, q: np.ndarray[float]) -> tuple[np.ndarray[float], np.ndarray[float] | None]:
        return q[: self.n_bank_params], q[self.n_bank_params :]


class FoldedData:
    """Counts (nspec, nbins) on one folded chopper cycle of T us, with Poisson weights.

    Not a dataclass: Mantid loads plugin modules outside sys.modules, which dataclasses cannot handle."""

    def __init__(self, y: np.ndarray[float], T: float):
        self.y, self.T = y, T
        self.w = 1.0 / np.sqrt(y + 1.0)


class TimingModel:
    """Maps the timing parameters to one tz per dataset."""

    def __init__(self, kind: str, cycle_times: Sequence[float], speeds: Sequence[float], t0: float, t0_const: float):
        cycle_times = np.asarray(cycle_times, dtype=float)
        self.kind = kind
        if kind == "Linear":
            self.matrix = np.column_stack([cycle_times, np.ones_like(cycle_times)])
            self.x0 = np.array([t0, t0_const])
            self.labels = ["t0", "t0_const"]
        else:  # PerChopperSpeed: datasets of the same speed share tz
            self.speeds = sorted(set(speeds))
            self.matrix = np.array([[float(s == u) for u in self.speeds] for s in speeds])
            self.x0 = np.array([t0 * cycle_times[list(speeds).index(u)] + t0_const for u in self.speeds])
            self.labels = [f"tz({u:g} rpm)" for u in self.speeds]

    def table_rows(self, x: np.ndarray[float]) -> list[tuple[str, float, float]]:
        """(parameter, value, chopper speed or 0 for every speed) rows of the chopper in the calibration table."""
        if self.kind == "Linear":
            return [("t0", x[0], 0.0), ("t0_const", x[1], 0.0)]
        return [("t0", 0.0, 0.0)] + [("t0_const", value, speed) for speed, value in zip(self.speeds, x)]


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
    """Predicted counts m (nspec, nbins). With deriv, also dm/dc (nspec, nbins), dm/dtz, dm/dkappa, dm/dsrel and
    dm/dA (K, nspec, nbins).

    c and sin_t are per spectrum, A and d_hkl per reflection. In the shapes below, n is the number of pixels a reflection
    reaches, S the number of chopper slits and W the number of bins evaluated around each peak."""
    nspec, n_bins = data.y.shape
    T = data.T  # cycle time (us)
    bin_width = T / n_bins
    n_flat = nspec * n_bins  # m and its derivatives are built flat over (spectrum, bin), then reshaped
    live = np.flatnonzero(geom.live)  # spectrum indices of the unmasked pixels
    m = np.zeros(n_flat)  # predicted counts
    if deriv:
        # c_i only affects spectrum i, so dm/dc needs one value per (spectrum, bin), like m
        dm_dc, dm_dtz, dm_dkappa, dm_dsrel = (np.zeros(n_flat) for _ in range(4))
        dm_dA = np.zeros((d_hkl.size, n_flat))
    sig_chopper = kappa * T  # chopper-opening part of the peak width, the same for every pixel
    for k, d_k in enumerate(d_hkl):
        # phi: incident flux at the wavelength each live pixel sees reflection k at, lambda = 2 d_k sin(theta_i) by
        # Bragg's law; zero outside the flux table
        phi = np.interp(2.0 * d_k * sin_t[live], geom.lam, geom.flux, left=0.0, right=0.0)
        reached = phi > 0
        if not reached.any():
            continue
        pix = live[reached]  # spectrum index of each pixel the reflection reaches, (n,)
        c_i, phi_i = c[pix], phi[reached]  # (n,)
        sig = np.sqrt(sig_chopper**2 + (srel * c_i * d_k) ** 2)  # peak width (us), (n,)
        # arrival time via each slit (TOF + timing offset + slit opening time), folded onto one cycle, (n, S)
        centre = np.mod(c_i[:, None] * d_k + tz + geom.slit_x[None, :] * T, T)
        # bins within nsig of each centre, sized for the widest peak so every window has W bins, (n, S, W).
        # Left unwrapped: near the cycle ends they run below 0 or past n_bins and still measure the true distance to the centre
        half_window = int(np.ceil(nsig * sig.max() / bin_width)) + 1
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
            # the width parameters act only through sig
            dm_dsrel += _sum_into_bins(flat_bins, peak_area * dG_dsig * dsig_dsrel, n_flat)
            dm_dkappa += _sum_into_bins(flat_bins, peak_area * dG_dsig * dsig_dkappa, n_flat)
    shape = (nspec, n_bins)
    if not deriv:
        return m.reshape(shape)
    # kappa before srel, the order CalibrationFit.evaluate unpacks them in
    return (*(v.reshape(shape) for v in (m, dm_dc, dm_dtz, dm_dkappa, dm_dsrel)), dm_dA.reshape((d_hkl.size, *shape)))


def scale_bg(y: np.ndarray[float], m: np.ndarray[float], w: np.ndarray[float]) -> tuple[np.ndarray[float], np.ndarray[float]]:
    """Weighted closed-form per-spectrum scale s and flat background b minimising |w (y - s m - b)|^2."""
    w2 = w**2
    s_w, s_x, s_y = w2.sum(1), (w2 * m).sum(1), (w2 * y).sum(1)
    s_xx, s_xy = (w2 * m**2).sum(1), (w2 * m * y).sum(1)
    det = s_w * s_xx - s_x**2
    ok = det > 1e-12 * np.maximum(s_w * s_xx, 1e-300)
    s = np.where(ok, (s_w * s_xy - s_x * s_y) / np.where(ok, det, 1.0), 0.0)
    b = np.where(ok, (s_y - s * s_x) / np.where(s_w > 0, s_w, 1.0), 0.0)
    return s, b


def project_out(J: np.ndarray[float], m: np.ndarray[float], w: np.ndarray[float]) -> np.ndarray[float]:
    """Per spectrum, remove from the weighted columns w J the part in span{w m, w} (Kaufman variable projection)."""
    q2 = w / np.maximum(np.sqrt((w**2).sum(-1, keepdims=True)), 1e-300)
    a1 = w * m
    a1 = a1 - (a1 * q2).sum(-1, keepdims=True) * q2
    q1 = a1 / np.maximum(np.sqrt((a1**2).sum(-1, keepdims=True)), 1e-300)
    wJ = w * J
    return wJ - (wJ * q1).sum(-1, keepdims=True) * q1 - (wJ * q2).sum(-1, keepdims=True) * q2


class CalibrationFit:
    """Levenberg-Marquardt fit of the forward model to folded counts.

    Parameters, in order: geometry q | timing parameters | kappa, sigma_rel | reflection intensities A.
    `start`, a previous fit of the same data, seeds everything after the geometry."""

    def __init__(
        self,
        geom: PixelGeometry,
        datasets: Sequence[FoldedData],
        geometry_model: RigidBanks,
        d_hkl: np.ndarray[float],
        timing: TimingModel,
        kappa: float = 1e-3,
        srel: float = 1.5e-3,
        start: CalibrationFit | None = None,
    ):
        self.geom, self.datasets, self.gm, self.timing = geom, datasets, geometry_model, timing
        for ds in datasets:  # masked spectra must not contribute, even when the mask didn't zero their counts
            ds.w[~geom.live] = 0.0
        _, sin_t = geom.difc(geom.q0)  # drop reflections no live pixel can reach: their intensity column would be all zero
        lam = 2 * np.outer(d_hkl, sin_t[geom.live])
        self.d_hkl = np.asarray(d_hkl)[((lam >= geom.lam[0]) & (lam <= geom.lam[-1])).any(axis=1)]
        self.nq, self.ntz = geometry_model.q0.size, timing.x0.size
        self.x = np.concatenate([geometry_model.q0, timing.x0, [kappa, srel], np.ones(self.d_hkl.size)])
        self.labels = list(geometry_model.labels) + timing.labels + ["kappa", "sigma_rel"] + [f"I(d={d:.4f})" for d in self.d_hkl]
        self.free = np.ones(self.x.size, bool)
        if start is None:
            self._init_intensities()
        else:
            self.x[self.nq :], self.free[self.nq :] = start.x[start.nq :], start.free[start.nq :]

    def split(self, x: np.ndarray[float]) -> tuple[np.ndarray[float], np.ndarray[float], float, float, np.ndarray[float]]:
        nq, ntz = self.nq, self.ntz
        return x[:nq], x[nq : nq + ntz], x[nq + ntz], x[nq + ntz + 1], x[nq + ntz + 2 :]

    @property
    def x_tz(self) -> np.ndarray[float]:
        return self.split(self.x)[1]

    @property
    def q_banks(self) -> np.ndarray[float]:
        return self.gm.split(self.split(self.x)[0])[0]

    @property
    def q_blocks(self) -> np.ndarray[float] | None:
        return self.gm.split(self.split(self.x)[0])[1]

    def _init_intensities(self) -> None:
        """Linear least-squares estimate of A (with per-spectrum backgrounds and unit scales). The strongest reflection
        is then fixed: the overall scale lives in the per-spectrum scales."""
        q, x_tz, kappa, srel, _ = self.split(self.x)
        c, sin_t, _ = self.gm(q)
        K = self.d_hkl.size
        normal, rhs = np.zeros((K, K)), np.zeros(K)
        for ds, tz in zip(self.datasets, self.timing.matrix @ x_tz):
            dA = model_counts(self.geom, ds, c, sin_t, tz, kappa, srel, np.ones(K), self.d_hkl, deriv=True)[-1]
            w2 = ds.w**2
            w_sum = np.maximum(w2.sum(1, keepdims=True), 1e-300)
            y_centred = ds.y - (w2 * ds.y).sum(1, keepdims=True) / w_sum
            B = ((dA - (w2 * dA).sum(2, keepdims=True) / w_sum) * ds.w).reshape(K, -1)
            normal += B @ B.T
            rhs += B @ (ds.w * y_centred).ravel()
        A = np.linalg.lstsq(normal, rhs, rcond=None)[0]
        A = np.clip(A, 1e-3 * np.abs(A).max(), None)
        self.x[self.nq + self.ntz + 2 :] = A / A.max()
        self.free[self.nq + self.ntz + 2 + int(np.argmax(A))] = False

    def evaluate(self, x: np.ndarray[float], deriv: bool = False) -> float | tuple[float, np.ndarray[float], np.ndarray[float]]:
        """Cost 0.5 |r|^2, and with deriv the Gauss-Newton normal matrix and gradient over all of x."""
        q, x_tz, kappa, srel, A = self.split(x)
        c, sin_t, dc_dq = self.gm(q)
        n_other = x.size - self.nq
        cost = 0.0
        if deriv:
            Dc, gc = np.zeros(c.size), np.zeros(c.size)
            C, G_other, g_other = np.zeros((c.size, n_other)), np.zeros((n_other, n_other)), np.zeros(n_other)
            residuals = []
        for j, (ds, tz) in enumerate(zip(self.datasets, self.timing.matrix @ x_tz)):
            out = model_counts(self.geom, ds, c, sin_t, tz, kappa, srel, A, self.d_hkl, deriv=deriv)
            m = out[0] if deriv else out
            scale, bg = scale_bg(ds.y, m, ds.w)
            r = ds.w * (ds.y - scale[:, None] * m - bg[:, None])
            cost += 0.5 * np.sum(r**2)
            if not deriv:
                continue
            residuals.append(r)
            _, dm_dc, dm_dtz, dm_dka, dm_dsr, dm_dA = out

            def project(J):
                return -scale[:, None] * project_out(J, m, ds.w)

            Jc = project(dm_dc)
            J_tz = project(dm_dtz)
            cols = [J_tz * self.timing.matrix[j, a] for a in range(self.ntz)]
            cols += [project(dm_dka), project(dm_dsr)] + [project(dA) for dA in dm_dA]
            J_other = np.stack(cols).reshape(len(cols), -1)
            Dc += np.sum(Jc**2, axis=1)
            gc += np.sum(Jc * r, axis=1)
            C += (J_other.reshape(len(cols), *Jc.shape) * Jc).sum(2).T
            G_other += J_other @ J_other.T
            g_other += J_other @ r.ravel()
        if not deriv:
            return cost
        self.residuals = residuals  # at x, for the reduced chi-squared
        normal = np.zeros((x.size, x.size))
        normal[: self.nq, : self.nq] = dc_dq.T @ (Dc[:, None] * dc_dq)
        normal[: self.nq, self.nq :] = dc_dq.T @ C
        normal[self.nq :, : self.nq] = normal[: self.nq, self.nq :].T
        normal[self.nq :, self.nq :] = G_other
        return cost, normal, np.concatenate([dc_dq.T @ gc, g_other])

    def fit(self, max_iter: int = 50, tol: float = 1e-9, step_tol: float = 1e-6, lam: float = 1e-3) -> CalibrationFit:
        cost, normal, grad = self.evaluate(self.x, deriv=True)
        f = self.free
        for _ in range(max_iter):
            Nf, gf = normal[np.ix_(f, f)], grad[f]
            scale = np.sqrt(np.maximum(np.diag(Nf), 1e-300))
            while lam <= 1e8:
                step = np.zeros_like(self.x)
                step[f] = -np.linalg.solve(Nf / np.outer(scale, scale) + lam * np.eye(f.sum()), gf / scale) / scale
                trial_cost = self.evaluate(self.x + step)
                if trial_cost < cost:
                    lam = max(lam / 5, 1e-7)
                    break
                lam *= 5
            else:
                break  # no downhill step: converged to within numerical precision
            rel = (cost - trial_cost) / cost
            self.x = self.x + step
            cost, normal, grad = self.evaluate(self.x, deriv=True)
            # steps far below the statistical uncertainties: stops noise-free fits, whose cost falls without plateauing
            if rel < tol or np.abs(step[f] * scale).max() < step_tol:
                break
        self.cost = cost
        n_data = sum(int(self.geom.live.sum()) * ds.y.shape[1] for ds in self.datasets)
        n_fitted = int(f.sum()) + 2 * int(self.geom.live.sum()) * len(self.datasets)  # + per-spectrum scale and background
        self.red_chi2 = sum(np.sum(r**2) for r in self.residuals) / (n_data - n_fitted)
        cov = np.full((self.x.size, self.x.size), np.nan)
        cov[np.ix_(f, f)] = np.linalg.pinv(normal[np.ix_(f, f)]) * self.red_chi2
        self.err = np.sqrt(np.diag(cov))
        return self


def apply_geometry(
    ws: MatrixWorkspace,
    geom: PixelGeometry,
    q_banks: np.ndarray[float],
    exec_child: Callable[..., object],
    q_blocks: np.ndarray[float] | None = None,
) -> None:
    """Apply absolute bank moves (x, z, roty) to ws, then block moves (normal shift, tilt about the block centre)."""
    for ib, bank in enumerate(geom.banks):
        x, z, roty = q_banks[3 * ib : 3 * ib + 3]
        exec_child("RotateInstrumentComponent", Workspace=ws, ComponentName=bank, X=0, Y=1, Z=0, Angle=roty, RelativeRotation=False)
        exec_child("MoveInstrumentComponent", Workspace=ws, ComponentName=bank, X=x, Y=geom.bank_y[ib], Z=z, RelativePosition=False)
    if q_blocks is None:
        return
    comp_info = ws.componentInfo()
    for m, block in enumerate(geom.blocks):
        normal, tilt = q_blocks[2 * m : 2 * m + 2]
        nx, _, nz = rot_y(q_banks[3 * geom.block_bank[m] + 2]) @ [0.0, 0.0, 1.0]  # bank normal, global frame
        exec_child("MoveInstrumentComponent", Workspace=ws, ComponentName=block, X=normal * nx, Y=0, Z=normal * nz, RelativePosition=True)
        # a relative rotation turns about an axis in the component's own frame: NorthBank's blocks sit under a 180 deg
        # rotation about z, so global Y is -Y there
        axis = _rotation_matrix(comp_info.rotation(component_index(comp_info, block))).T @ [0.0, 1.0, 0.0]
        exec_child(
            "RotateInstrumentComponent",
            Workspace=ws,
            ComponentName=block,
            X=axis[0],
            Y=axis[1],
            Z=axis[2],
            Angle=tilt,
            RelativeRotation=True,
        )
