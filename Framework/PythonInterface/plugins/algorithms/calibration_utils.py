# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Instrument-agnostic parts of a forward-model calibration: bank and block geometry in numpy, a per-spectrum scale and
flat background removed by variable projection, and the Levenberg-Marquardt fit. poldi_calibration_utils builds POLDI's
model on these."""

from __future__ import annotations

from collections.abc import Callable, Sequence
from typing import TYPE_CHECKING

import numpy as np

from mantid.kernel import Quat, V3D
from plugins.algorithms.component_info_utils import resolve_component_index
from plugins.algorithms.diffraction_utils import get_live_spectra

if TYPE_CHECKING:
    from mantid.api import MatrixWorkspace


def rot_y(deg: float) -> np.ndarray[float]:
    a = np.deg2rad(deg)
    return np.array([[np.cos(a), 0.0, np.sin(a)], [0.0, 1.0, 0.0], [-np.sin(a), 0.0, np.cos(a)]])


def _angle_axis(q: Quat) -> tuple[float, np.ndarray[float]]:
    """Angle (deg) and unit axis of q. Not Quat.getAngleAxis, which rounds rotations under ~0.5 deg to none."""
    v = np.array([q.imagI(), q.imagJ(), q.imagK()])
    norm = np.linalg.norm(v)
    return np.rad2deg(2 * np.arctan2(norm, q.real())), v / norm if norm > 0 else np.array([0.0, 1.0, 0.0])


class BankGeometry:
    """Pixel positions in the frame of their bank, so bank and block moves can be applied in numpy.

    Pixels outside `bank_names` keep their starting position and DIFC. c_i is the starting initial_difc scaled by the
    geometric factor g = (L_p + L2) sin(theta), L_p being the primary_flight_path the DIFC covers (source to sample on a
    conventional TOF instrument), so the absolute constant is always Mantid's. Subclasses set initial_difc."""

    def __init__(self, ws: MatrixWorkspace, bank_names: Sequence[str], primary_flight_path: float):
        comp_info, det_info = ws.componentInfo(), ws.detectorInfo()
        self.live_spectra = get_live_spectra(ws)
        self.primary_flight_path = primary_flight_path
        self.sample_position = np.array(comp_info.samplePosition())
        beam = self.sample_position - np.array(comp_info.sourcePosition())
        self.beam_direction = beam / np.linalg.norm(beam)
        self.bank_names = list(bank_names)
        self.detector_indices = np.array([det_info.indexOf(ws.getSpectrum(i).getDetectorIDs()[0]) for i in range(ws.getNumberHistograms())])
        bank_idx = [resolve_component_index(bank, comp_info) for bank in self.bank_names]
        self.bank_of_spectrum = np.full(self.detector_indices.size, -1)
        for ib, b in enumerate(bank_idx):
            self.bank_of_spectrum[np.isin(self.detector_indices, comp_info.detectorsInSubtree(b))] = ib
        # banks are rotated about Y only, so (x, z, roty) with y fixed describes them
        self.bank_y_positions = np.array([comp_info.position(b)[1] for b in bank_idx])
        self.initial_bank_params = np.concatenate(
            [[comp_info.position(b)[0], comp_info.position(b)[2], comp_info.rotation(b).getEulerAngles("YZX")[0]] for b in bank_idx]
        )
        self.initial_positions = np.array([comp_info.position(int(d)) for d in self.detector_indices])
        self.positions_in_bank = np.zeros_like(self.initial_positions)
        for ib, b in enumerate(bank_idx):
            sel = self.bank_of_spectrum == ib
            rel = self.initial_positions[sel] - np.array(comp_info.position(b))
            self.positions_in_bank[sel] = rel @ rot_y(self.initial_bank_params[3 * ib + 2])  # R^T rel, row-wise
        # detector blocks (modules) holding live pixels of the refined banks, as each such pixel's parent component.
        # The data cannot constrain a block with no live pixel, so it is not refined
        live_in_bank = self.live_spectra & (self.bank_of_spectrum >= 0)
        parents = [comp_info.parent(int(d)) if live else -1 for d, live in zip(self.detector_indices, live_in_bank)]
        block_idx = sorted({p for p in parents if p >= 0})
        self.block_of_spectrum = np.array([block_idx.index(p) if p >= 0 else -1 for p in parents])
        self.block_names = [comp_info.fullName(b).partition("/")[2] for b in block_idx]  # path below the instrument root
        self.bank_of_block = [int(self.bank_of_spectrum[self.block_of_spectrum == m][0]) for m in range(len(block_idx))]
        # in the bank frame; the block component's own position, which is what RotateInstrumentComponent rotates about
        self.block_centres = np.array(
            [
                (np.array(comp_info.position(b)) - comp_info.position(bank_idx[ib])) @ rot_y(self.initial_bank_params[3 * ib + 2])
                for b, ib in zip(block_idx, self.bank_of_block)
            ]
        )
        # (angle, axis) of each block's fixed rotation within its bank, so apply_geometry can set block rotations absolutely
        self.block_rotations_in_bank = [
            _angle_axis(Quat(-self.initial_bank_params[3 * ib + 2], V3D(0, 1, 0)) * comp_info.rotation(b))
            for b, ib in zip(block_idx, self.bank_of_block)
        ]
        self.initial_geometric_factor = self._geometric_factor_and_sin_theta(self.pixel_positions(self.initial_bank_params))[0]

    def pixel_positions(self, bank_params: np.ndarray[float], block_params: np.ndarray[float] | None = None) -> np.ndarray[float]:
        """Absolute pixel positions after bank moves (x, z, roty per bank) and optional block moves (shift along the
        bank normal, tilt about the vertical through the block centre)."""
        local = self.positions_in_bank if block_params is None else self._positions_in_bank_after_block_moves(block_params)
        out = self.initial_positions.copy()  # pixels outside the refined banks stay where they are
        for ib in range(len(self.bank_names)):
            sel = self.bank_of_spectrum == ib
            x, z, roty = bank_params[3 * ib : 3 * ib + 3]
            out[sel] = local[sel] @ rot_y(roty).T + [x, self.bank_y_positions[ib], z]
        return out

    def _positions_in_bank_after_block_moves(self, block_params: np.ndarray[float]) -> np.ndarray[float]:
        local = self.positions_in_bank.copy()
        for m, centre in enumerate(self.block_centres):
            sel = self.block_of_spectrum == m
            normal, tilt = block_params[2 * m : 2 * m + 2]
            local[sel] = (local[sel] - centre) @ rot_y(tilt).T + centre + [0.0, 0.0, normal]
        return local

    def _geometric_factor_and_sin_theta(self, positions: np.ndarray[float]) -> tuple[np.ndarray[float], np.ndarray[float]]:
        rel = positions - self.sample_position
        l2 = np.linalg.norm(rel, axis=1)
        sin_t = np.sqrt(0.5 * (1.0 - rel @ self.beam_direction / np.where(l2 > 0, l2, 1.0)))
        return (self.primary_flight_path + l2) * sin_t, sin_t

    def difc_and_sin_theta(
        self, bank_params: np.ndarray[float], block_params: np.ndarray[float] | None = None
    ) -> tuple[np.ndarray[float], np.ndarray[float]]:
        """DIFC c_i (us/A) and sin(theta_i) of every spectrum."""
        g, sin_t = self._geometric_factor_and_sin_theta(self.pixel_positions(bank_params, block_params))
        in_bank = self.bank_of_spectrum >= 0
        return np.where(in_bank, self.initial_difc * g / np.where(in_bank, self.initial_geometric_factor, 1.0), self.initial_difc), sin_t


class RigidBanks:
    """Geometry parameters (x, z, roty) per bank, mapped to (c, sin_t) and their derivatives for the fit."""

    BANK_FINITE_DIFF_STEPS = (1e-5, 1e-5, 1e-4)  # m, m, deg

    def __init__(self, geometry: BankGeometry):
        self.geometry = geometry
        self.initial_params = geometry.initial_bank_params.copy()
        self.param_names = [f"{bank}.{par}" for bank in geometry.bank_names for par in ("x", "z", "roty")]
        self.finite_diff_steps = np.tile(self.BANK_FINITE_DIFF_STEPS, len(geometry.bank_names))

    def difc_and_derivatives(self, q: np.ndarray[float]) -> tuple[np.ndarray[float], np.ndarray[float], np.ndarray[float]]:
        """c and sin_t of every spectrum, and d(c, sin_t)/dq (nspec, 2, nq) by central differences."""
        c, sin_t = self.geometry.difc_and_sin_theta(*self.split_bank_and_block_params(q))
        dpixel_dq = np.empty((c.size, 2, q.size))
        for a, step in enumerate(self.finite_diff_steps):
            dq = np.zeros(q.size)
            dq[a] = step
            plus = np.array(self.geometry.difc_and_sin_theta(*self.split_bank_and_block_params(q + dq)))  # (2, nspec)
            minus = np.array(self.geometry.difc_and_sin_theta(*self.split_bank_and_block_params(q - dq)))
            dpixel_dq[:, :, a] = ((plus - minus) / (2 * step)).T
        return c, sin_t, dpixel_dq

    def split_bank_and_block_params(self, q: np.ndarray[float]) -> tuple[np.ndarray[float], np.ndarray[float] | None]:
        return q, None


class BankModules(RigidBanks):
    """Rigid banks plus, per detector block, a shift along the bank normal and a tilt about the vertical."""

    BLOCK_FINITE_DIFF_STEPS = (1e-5, 1e-3)  # m, deg

    def __init__(self, geometry: BankGeometry, bank_params: np.ndarray[float]):
        super().__init__(geometry)
        self.n_bank_params = len(bank_params)
        self.initial_params = np.concatenate([bank_params, np.zeros(2 * len(geometry.block_names))])
        self.param_names += [f"{block}.{par}" for block in geometry.block_names for par in ("normal", "tilt")]
        self.finite_diff_steps = np.concatenate([self.finite_diff_steps, np.tile(self.BLOCK_FINITE_DIFF_STEPS, len(geometry.block_names))])

    def split_bank_and_block_params(self, q: np.ndarray[float]) -> tuple[np.ndarray[float], np.ndarray[float] | None]:
        return q[: self.n_bank_params], q[self.n_bank_params :]


class WeightedCounts:
    """Counts (nspec, nbins) with Poisson weights.

    Not a dataclass: Mantid loads plugin modules outside sys.modules, which dataclasses cannot handle."""

    def __init__(self, counts: np.ndarray[float]):
        self.counts = counts
        self.weights = 1.0 / np.sqrt(counts + 1.0)


def scale_bg(y: np.ndarray[float], m: np.ndarray[float], w: np.ndarray[float]) -> tuple[np.ndarray[float], np.ndarray[float]]:
    """Weighted closed-form per-spectrum scale s and flat background b minimising |w (y - s m - b)|^2."""
    w2 = w**2
    s_w, s_x, s_y = w2.sum(1), (w2 * m).sum(1), (w2 * y).sum(1)
    s_xx, s_xy = (w2 * m**2).sum(1), (w2 * m * y).sum(1)
    det = s_w * s_xx - s_x**2
    ok = det > 1e-12 * np.maximum(s_w * s_xx, 1e-300)
    s = np.where(ok, (s_w * s_xy - s_x * s_y) / np.where(ok, det, 1.0), 0.0)
    b = (s_y - s * s_x) / np.where(s_w > 0, s_w, 1.0)  # the weighted mean of y where s = 0
    return s, b


def weighted_residuals(data: WeightedCounts, m: np.ndarray[float]) -> tuple[np.ndarray[float], np.ndarray[float]]:
    """Residuals r = w (y - s m - b) (nspec, nbins) at each spectrum's best scale s and background b, and s."""
    scale, bg = scale_bg(data.counts, m, data.weights)
    return data.weights * (data.counts - scale[:, None] * m - bg[:, None]), scale


def project_out(derivative: np.ndarray[float], model: np.ndarray[float], weights: np.ndarray[float]) -> np.ndarray[float]:
    """Per spectrum, the part of weights * derivative that the spectrum's scale s and flat background b cannot cancel
    (Kaufman variable projection).

    scale_bg re-picks s and b whenever the fit parameters change, so it cancels any change to a spectrum along
    weights * model (rescaling it) or along weights (lifting it). Treating a spectrum's bins as one vector, those two
    directions span a plane, and only the part of the derivative perpendicular to that plane moves the residual:

            returned    weights * derivative
                ^      ^
                |     /
                |    /
                |   /
                |  /
                | /
              ==o======+===========  plane of the changes s and b can make
                <-   ->
                removed

    NOTE: weights * model and weights are not perpendicular, so removing the part along each in turn would remove
    some of the derivative twice. The same plane is instead described by two perpendicular unit directions, which can
    be removed one at a time. background_dir stays parallel to weights, so only weights * model needs replacing:
    scale_dir is what is left of it once its part along background_dir is removed.

           scale_dir    weights * model
                ^      ^
                |     /
                |    /
                |   /
                |  /
                | /  )  <- not perpendicular
                o-------------> background_dir AND weights
                <-  ->
                removed

    Masked spectra have zero weights, so every vector of theirs is zero and stays zero.
    """

    def dot(a, b):  # one dot product per spectrum, over its bins
        return (a * b).sum(-1, keepdims=True)

    def unit(a):
        return a / np.maximum(np.sqrt(dot(a, a)), 1e-300)

    background_dir = unit(weights)
    scale_dir = unit(weights * model - dot(weights * model, background_dir) * background_dir)
    weighted_derivative = weights * derivative
    in_plane = dot(weighted_derivative, scale_dir) * scale_dir + dot(weighted_derivative, background_dir) * background_dir
    return weighted_derivative - in_plane


class NormalEquationSums:
    """Running sums over datasets of the Gauss-Newton normal matrix J^T J and gradient J^T r.

    The geometry q reaches the counts only through each pixel's DIFC c_i and sin(theta_i), which only affect spectrum
    i. So the Jacobian columns for (c, sin_t) are stored compactly as J_pixel (2, nspec, nbins), row i being the only
    nonzero part of pixel i's columns, and the sums involving them are kept per pixel as small blocks. `assemble`
    converts those to q once, by the chain rule through d(c, sin_t)/dq. The other parameters, such as timing, peak
    widths and intensities, affect every spectrum, so their columns J_other are dense."""

    def __init__(self, n_pixels: int, n_other: int):
        self.pixel_pixel = np.zeros((n_pixels, 2, 2))  # J_pixel^T J_pixel, block diagonal as no two pixels' columns share a row
        self.pixel_other = np.zeros((n_pixels, 2, n_other))  # J_pixel^T J_other
        self.other_other = np.zeros((n_other, n_other))  # J_other^T J_other
        self.pixel_gradient = np.zeros((n_pixels, 2))  # J_pixel^T r
        self.other_gradient = np.zeros(n_other)  # J_other^T r

    def add(self, J_pixel: np.ndarray[float], J_other: np.ndarray[float], r: np.ndarray[float]) -> None:
        """Add one dataset's contribution: J_pixel is (2, nspec, nbins), r (nspec, nbins), J_other (n_other, nspec, nbins)."""
        # indices: i pixel, b bin, u and v the pixel's (c, sin_t), o the other parameters
        self.pixel_pixel += np.einsum("uib,vib->iuv", J_pixel, J_pixel)
        self.pixel_other += np.einsum("uib,oib->iuo", J_pixel, J_other)
        self.pixel_gradient += np.einsum("uib,ib->iu", J_pixel, r)
        J_other_flat = J_other.reshape(len(J_other), -1)
        self.other_other += J_other_flat @ J_other_flat.T
        self.other_gradient += J_other_flat @ r.ravel()

    def assemble(self, dpixel_dq: np.ndarray[float]) -> tuple[np.ndarray[float], np.ndarray[float]]:
        """Normal matrix and gradient over (q, other parameters), with dpixel_dq = d(c, sin_t)/dq (nspec, 2, nq), summed
        over pixels:

                  q                                             other
        q      [  dpixel_dq^T pixel_pixel dpixel_dq             dpixel_dq^T pixel_other  ]
        other  [  (transpose)                                   other_other              ]
        """
        nq = dpixel_dq.shape[2]
        normal = np.zeros((nq + self.other_gradient.size,) * 2)
        normal[:nq, :nq] = np.einsum("iuq,iuv,ivp->qp", dpixel_dq, self.pixel_pixel, dpixel_dq, optimize=True)
        normal[:nq, nq:] = np.einsum("iuq,iuo->qo", dpixel_dq, self.pixel_other)
        normal[nq:, :nq] = normal[:nq, nq:].T
        normal[nq:, nq:] = self.other_other
        return normal, np.concatenate([np.einsum("iuq,iu->q", dpixel_dq, self.pixel_gradient), self.other_gradient])


class ScaleBackgroundFit:
    """Levenberg-Marquardt fit of a model with a per-spectrum scale and flat background, both removed by variable
    projection (see project_out).

    Subclasses set params, is_free, geometry (a BankGeometry), datasets (WeightedCounts) and provide evaluate_cost(x) and
    evaluate_cost_and_derivatives(x) -> (cost, J^T J, J^T r), the latter also keeping its residuals."""

    def fit(self, max_iter: int = 50, tol: float = 1e-9, step_tol: float = 1e-6, lam: float = 1e-3) -> ScaleBackgroundFit:
        cost, normal, grad = self.evaluate_cost_and_derivatives(self.params)
        f = self.is_free
        for _ in range(max_iter):
            Nf, gf = normal[np.ix_(f, f)], grad[f]
            scale = np.sqrt(np.maximum(np.diag(Nf), 1e-300))
            while lam <= 1e8:
                step = np.zeros_like(self.params)
                step[f] = -np.linalg.solve(Nf / np.outer(scale, scale) + lam * np.eye(f.sum()), gf / scale) / scale
                trial_cost = self.evaluate_cost(self.params + step)
                if trial_cost < cost:
                    lam = max(lam / 5, 1e-7)
                    break
                lam *= 5
            else:
                break  # no downhill step: converged to within numerical precision
            rel = (cost - trial_cost) / cost
            self.params = self.params + step
            cost, normal, grad = self.evaluate_cost_and_derivatives(self.params)
            # steps far below the statistical uncertainties: stops noise-free fits, whose cost falls without plateauing
            if rel < tol or np.abs(step[f] * scale).max() < step_tol:
                break
        self.cost = cost
        n_live = int(self.geometry.live_spectra.sum())
        n_data = sum(n_live * ds.counts.shape[1] for ds in self.datasets)
        n_fitted = int(f.sum()) + 2 * n_live * len(self.datasets)  # + per-spectrum scale and background
        self.reduced_chi_squared = sum(np.sum(r**2) for r in self.residuals) / (n_data - n_fitted)
        cov = np.full((self.params.size, self.params.size), np.nan)
        cov[np.ix_(f, f)] = np.linalg.pinv(normal[np.ix_(f, f)]) * self.reduced_chi_squared
        self.errors = np.sqrt(np.diag(cov))
        return self


def apply_geometry(
    ws: MatrixWorkspace,
    geom: BankGeometry,
    bank_params: np.ndarray[float],
    exec_child: Callable[..., object],
    block_params: np.ndarray[float] | None = None,
) -> None:
    """Move the banks of ws to (x, z, roty), then the blocks to their normal shift and tilt about the block centre from
    the state geom was built from. All moves are absolute, so they do not depend on what was applied to ws before.
    Without block_params the blocks are not moved, so they keep any earlier block moves relative to their bank."""
    for ib, bank in enumerate(geom.bank_names):
        x, z, roty = bank_params[3 * ib : 3 * ib + 3]
        exec_child("RotateInstrumentComponent", Workspace=ws, ComponentName=bank, X=0, Y=1, Z=0, Angle=roty, RelativeRotation=False)
        exec_child(
            "MoveInstrumentComponent", Workspace=ws, ComponentName=bank, X=x, Y=geom.bank_y_positions[ib], Z=z, RelativePosition=False
        )
    if block_params is None:
        return
    for m, block in enumerate(geom.block_names):
        ib = geom.bank_of_block[m]
        x, z, roty = bank_params[3 * ib : 3 * ib + 3]
        normal, tilt = block_params[2 * m : 2 * m + 2]
        x_block, y_block, z_block = rot_y(roty) @ (geom.block_centres[m] + [0.0, 0.0, normal]) + [x, geom.bank_y_positions[ib], z]
        exec_child("MoveInstrumentComponent", Workspace=ws, ComponentName=block, X=x_block, Y=y_block, Z=z_block, RelativePosition=False)
        # the bank's rotation plus the tilt about Y, then the block's fixed rotation within its bank on top, as a relative
        # rotation is current * Quat(angle, axis). Turning the fitted angle about Y alone also keeps clear of
        # RotateInstrumentComponent rejecting any axis with X + Y + Z = 0
        exec_child("RotateInstrumentComponent", Workspace=ws, ComponentName=block, X=0, Y=1, Z=0, Angle=roty + tilt, RelativeRotation=False)
        angle, (ax, ay, az) = geom.block_rotations_in_bank[m]
        exec_child("RotateInstrumentComponent", Workspace=ws, ComponentName=block, X=ax, Y=ay, Z=az, Angle=angle, RelativeRotation=True)
