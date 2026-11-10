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

import numpy as np

from mantid.kernel import Quat


def rot_y(deg: float) -> np.ndarray[float]:
    a = np.deg2rad(deg)
    return np.array([[np.cos(a), 0.0, np.sin(a)], [0.0, 1.0, 0.0], [-np.sin(a), 0.0, np.cos(a)]])


def _angle_axis(q: Quat) -> tuple[float, np.ndarray[float]]:
    """Angle (deg) and unit axis of q. Not Quat.getAngleAxis, which rounds rotations under ~0.5 deg to none."""
    v = np.array([q.imagI(), q.imagJ(), q.imagK()])
    norm = np.linalg.norm(v)
    return np.rad2deg(2 * np.arctan2(norm, q.real())), v / norm if norm > 0 else np.array([0.0, 1.0, 0.0])


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
