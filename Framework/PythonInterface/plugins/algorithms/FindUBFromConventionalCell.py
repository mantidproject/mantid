# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#
# This algorithm was ported into Mantid from the neutrons/garnet-tools project
# (https://github.com/neutrons/garnet-tools, src/garnet/reduction/search.py,
# branch main). Relicensed under GPL-3.0+ with permission for inclusion in
# Mantid.
#
# E741 is suppressed because h, k, l are Miller indices, not ambiguous names.
# ruff: noqa: E741
from mantid.api import (
    PythonAlgorithm,
    AlgorithmFactory,
    IPeaksWorkspaceProperty,
    WorkspaceProperty,
)
from mantid.kernel import (
    Direction,
    StringListValidator,
    FloatBoundedValidator,
    IntBoundedValidator,
)
from mantid.dataobjects import TableWorkspaceProperty
from mantid.simpleapi import CreateEmptyTableWorkspace, CreateWorkspace, SetUB
from mantid.geometry import UnitCell

import numpy as np


def normalize(v):
    """
    Normalize a vector to unit length.

    Parameters
    ----------
    v : ndarray
        Vector to normalize.

    Returns
    -------
    v_hat : ndarray
        Unit vector in the direction of `v`.
    """
    v = np.asarray(v, dtype=float)
    n = np.linalg.norm(v)
    if n < 1e-15:
        raise ValueError("Cannot normalize near-zero vector.")
    return v / n


def orthonormal_frame(n):
    """
    Build an orthonormal basis (u, v) spanning the plane perpendicular to `n`.

    Parameters
    ----------
    n : ndarray of shape (3,)
        Reference direction.

    Returns
    -------
    u, v : ndarray of shape (3,)
        Orthonormal vectors perpendicular to `n` and to each other.
    """
    n = normalize(n)
    ref = np.array([0.0, 0.0, 1.0])
    if abs(np.dot(n, ref)) > 0.9:
        ref = np.array([1.0, 0.0, 0.0])

    u = ref - np.dot(ref, n) * n
    u = normalize(u)
    v = normalize(np.cross(n, u))
    return u, v


def angular_distance_antipodal(u, v):
    """
    Angle between the two straight lines spanned by `u` and `v`.

    The angle between `u` and `v` is the same as the angle between `-u` and `v`, since `u` and `-u` span the same line.

    Parameters
    ----------
    u, v : ndarray of shape (3,)
        Directions to compare.

    Returns
    -------
    angle : float
        Angle in radians, in [0, pi / 2].
    """
    return np.arccos(np.clip(abs(np.dot(normalize(u), normalize(v))), -1.0, 1.0))


def direct_basis_from_lattice(a, b, c, alpha, beta, gamma):
    """
    Build the direct-lattice basis matrix from lattice parameters.

    This matrix stores direct-lattice vectors as columns; reciprocal-lattice vectors are stored as columns.

    Parameters
    ----------
    a, b, c : float
        Lattice lengths.
    alpha, beta, gamma : float
        Lattice angles, in radians.

    Returns
    -------
    A : ndarray of shape (3, 3)
        Direct-lattice basis with a, b, c as columns.
    """
    ca, cb, cg = np.cos(alpha), np.cos(beta), np.cos(gamma)
    sg = np.sin(gamma)

    if abs(sg) < 1e-14:
        raise ValueError("gamma is too close to 0 or pi.")

    volume_factor = 1.0 + 2.0 * ca * cb * cg - ca**2 - cb**2 - cg**2
    if volume_factor <= 0.0:
        raise ValueError("Invalid lattice parameters.")

    return np.array(
        [
            [a, b * cg, c * cb],
            [0.0, b * sg, c * (ca - cb * cg) / sg],
            [0.0, 0.0, c * np.sqrt(volume_factor) / sg],
        ],
        dtype=float,
    )


def lattice_from_direct_basis(A):
    """
    Recover lattice parameters from a direct-lattice basis matrix.

    Parameters
    ----------
    A : ndarray of shape (3, 3)
        Direct-lattice basis with a, b, c as columns.

    Returns
    -------
    a, b, c : float
        Lattice lengths.
    alpha, beta, gamma : float
        Lattice angles, in radians.
    """
    a_vec, b_vec, c_vec = A[:, 0], A[:, 1], A[:, 2]

    a = np.linalg.norm(a_vec)
    b = np.linalg.norm(b_vec)
    c = np.linalg.norm(c_vec)

    alpha = np.arccos(np.clip(np.dot(b_vec, c_vec) / (b * c), -1.0, 1.0))
    beta = np.arccos(np.clip(np.dot(a_vec, c_vec) / (a * c), -1.0, 1.0))
    gamma = np.arccos(np.clip(np.dot(a_vec, b_vec) / (a * b), -1.0, 1.0))

    return a, b, c, alpha, beta, gamma


def reciprocal_basis_from_direct_basis(A):
    """
    Compute the reciprocal-lattice basis conjugate to a direct-lattice basis.

    The inverse transpose is used because direct-lattice vectors and reciprocal vectors are stored as columns.

    Parameters
    ----------
    A : ndarray of shape (3, 3)
        Direct-lattice basis with a, b, c as columns.

    Returns
    -------
    B : ndarray of shape (3, 3)
        Reciprocal-lattice basis with a*, b*, c* as columns.
    """
    return np.linalg.inv(A).T


def direct_basis_from_axes(a, b, c, a_vec, b_vec, c_vec):
    """
    Build a direct-lattice basis from lattice lengths and axis directions.

    Parameters
    ----------
    a, b, c : float
        Lattice lengths.
    a_vec, b_vec, c_vec : ndarray of shape (3,)
        Directions of the a, b, c axes (not required to be unit length).

    Returns
    -------
    A : ndarray of shape (3, 3)
        Direct-lattice basis with a, b, c as columns.
    """
    return np.column_stack(
        [
            a * normalize(a_vec),
            b * normalize(b_vec),
            c * normalize(c_vec),
        ]
    )


def centering_transform_to_primitive(centering):
    """
    Look up the transform from a conventional cell to its primitive cell.

    Parameters
    ----------
    centering : str
        Centering symbol: "P", "A", "B", "C", "I", "F", or "R".

    Returns
    -------
    T_cp : ndarray of shape (3, 3)
        Transform such that the primitive direct-lattice basis is `A_c @ T_cp`.
    """
    centering = centering.upper()

    # In every entry below the *columns* are the primitive vectors expressed in the conventional
    # basis, matching the `A_c @ T_cp` use in `conventional_to_primitive_lattice`. For A, B and C
    # the row and column readings differ only by a sign or an axis relabel within the same lattice,
    # and I and F are symmetric, so only R is sensitive to getting this convention wrong.
    transforms = {
        "P": np.eye(3),
        "A": np.array([[1.0, 0.0, 0.0], [0.0, 0.5, 0.5], [0.0, -0.5, 0.5]], dtype=float),
        "B": np.array([[0.5, 0.0, 0.5], [0.0, 1.0, 0.0], [-0.5, 0.0, 0.5]], dtype=float),
        "C": np.array([[0.5, 0.5, 0.0], [-0.5, 0.5, 0.0], [0.0, 0.0, 1.0]], dtype=float),
        "I": 0.5 * np.array([[-1.0, 1.0, 1.0], [1.0, -1.0, 1.0], [1.0, 1.0, -1.0]], dtype=float),
        "F": 0.5 * np.array([[0.0, 1.0, 1.0], [1.0, 0.0, 1.0], [1.0, 1.0, 0.0]], dtype=float),
        # Obverse setting: the columns are the rhombohedral vectors
        # a_R = (2a + b + c)/3, b_R = (-a + b + c)/3, c_R = (-a - 2b + c)/3
        # expressed in the conventional hexagonal basis.
        "R": np.array(
            [
                [2.0 / 3.0, -1.0 / 3.0, -1.0 / 3.0],
                [1.0 / 3.0, 1.0 / 3.0, -2.0 / 3.0],
                [1.0 / 3.0, 1.0 / 3.0, 1.0 / 3.0],
            ],
            dtype=float,
        ),
    }

    if centering not in transforms:
        raise ValueError("Unknown centering '{}'.".format(centering))
    return transforms[centering]


def conventional_to_primitive_lattice(a, b, c, alpha, beta, gamma, centering):
    """
    Convert conventional lattice parameters to a primitive cell.

    Parameters
    ----------
    a, b, c : float
        Conventional lattice lengths.
    alpha, beta, gamma : float
        Conventional lattice angles, in radians.
    centering : str
        Centering symbol: "P", "A", "B", "C", "I", "F", or "R".

    Returns
    -------
    lattice_p : tuple
        Primitive lattice parameters `(a, b, c, alpha, beta, gamma)`, with angles in radians.
    T_cp : ndarray of shape (3, 3)
        Transform from the conventional to the primitive direct-lattice basis.
    """
    A_c = direct_basis_from_lattice(a, b, c, alpha, beta, gamma)
    T_cp = centering_transform_to_primitive(centering)
    A_p = A_c @ T_cp
    lattice_p = lattice_from_direct_basis(A_p)
    return lattice_p, T_cp


def primitive_ub_to_conventional_ub(UB_p, T_cp):
    """
    Convert a primitive-cell UB matrix to the conventional-cell UB matrix.

    Miller indices reindex as `hkl_p = T_cp.T @ hkl_c`, so `UB_c = UB_p @ T_cp.T`.

    This is not generally the same as `UB_p @ inv(T_cp)` for centered cells.

    Parameters
    ----------
    UB_p : ndarray of shape (3, 3)
        UB matrix indexed on the primitive cell.
    T_cp : ndarray of shape (3, 3)
        Transform from the conventional to the primitive direct-lattice basis.

    Returns
    -------
    UB_c : ndarray of shape (3, 3)
        UB matrix indexed on the conventional cell.
    """
    return UB_p @ T_cp.T


def sample_hemisphere_grid(n_az=160, n_pol=160):
    """
    Sample directions on a hemisphere on a regular azimuth/polar grid.

    Only a hemisphere is sampled since `periodic_alignment_scores` cannot distinguish antipodal directions.

    Parameters
    ----------
    n_az : int, optional
        Number of azimuthal samples.
    n_pol : int, optional
        Number of polar samples, uniform in cosine of the polar angle.

    Returns
    -------
    directions : ndarray of shape (n_az * n_pol, 3)
        Unit vectors covering the hemisphere.
    """
    az, u = np.meshgrid(
        np.linspace(0.0, 2.0 * np.pi, n_az, endpoint=False),
        np.linspace(0.0, 0.5, n_pol),
        indexing="ij",
    )

    az = az.ravel()
    cos_pol = 2.0 * u.ravel() - 1.0
    sin_pol = np.sqrt(np.maximum(0.0, 1.0 - cos_pol**2))

    return np.column_stack([sin_pol * np.cos(az), sin_pol * np.sin(az), cos_pol])


def sample_spherical_cap(n, theta_max_deg, n_samples, rng=None):
    """
    Sample random directions within a cap around a central direction.

    Parameters
    ----------
    n : ndarray of shape (3,)
        Central direction of the cap.
    theta_max_deg : float
        Cap half-angle, in degrees.
    n_samples : int
        Number of directions to sample.
    rng : numpy.random.Generator, optional
        Random number generator. A default generator is created if omitted.

    Returns
    -------
    directions : ndarray of shape (n_samples, 3)
        Unit vectors within the cap, or `n` itself (reshaped to shape (1, 3)) if `theta_max_deg` is zero or negative.
    """
    if rng is None:
        rng = np.random.default_rng()

    n = normalize(n)
    theta_max = np.deg2rad(theta_max_deg)
    if theta_max <= 0:
        return n[None, :]

    u, v = orthonormal_frame(n)

    phi = rng.uniform(0.0, 2.0 * np.pi, size=n_samples)
    cos_theta = rng.uniform(np.cos(theta_max), 1.0, size=n_samples)
    sin_theta = np.sqrt(np.maximum(0.0, 1.0 - cos_theta**2))

    return cos_theta[:, None] * n[None, :] + sin_theta[:, None] * (np.cos(phi)[:, None] * u[None, :] + np.sin(phi)[:, None] * v[None, :])


def project_reflections_onto_directions(q_vectors, directions):
    """
    Project reflection Q-vectors onto a set of candidate directions.

    Parameters
    ----------
    q_vectors : ndarray of shape (n_peaks, 3)
        Peak positions in the Q-sample frame, in inverse angstroms.
    directions : ndarray of shape (3,) or (n_directions, 3)
        Candidate directions, assumed to be normalized.

    Returns
    -------
    s : ndarray of shape (n_peaks, n_directions)
        Dot product of each Q-vector with each direction.
    """
    directions = np.atleast_2d(directions)
    return q_vectors @ directions.T


def periodic_alignment_scores(s, spacing):
    """
    Score how closely values cluster near integers, via circular statistics.

    A true axis gives `s * spacing` near integer Miller indices, producing a score close to 1.

    Unrelated directions have nearly random phases and score close to 0.

    Parameters
    ----------
    s : ndarray
        Projections to test, with shape (n_peaks, n_directions), as returned by `project_reflections_onto_directions`.
    spacing : float
        Candidate real-space lattice length along the projection direction.

    Returns
    -------
    score : ndarray
        Resultant phase length for each direction, in [0, 1], averaged over all peaks.
    """
    x = s * spacing
    return np.abs(np.exp(2j * np.pi * x).mean(axis=0))


def best_direction_from_candidates(q_vectors, directions, spacing):
    """
    Pick the candidate direction with the highest periodicity score.

    Parameters
    ----------
    q_vectors : ndarray of shape (n_peaks, 3)
        Peak positions in the Q-sample frame, in inverse angstroms.
    directions : ndarray of shape (n_directions, 3)
        Candidate unit directions.
    spacing : float
        Candidate real-space lattice length along `directions`.

    Returns
    -------
    best_direction : ndarray of shape (3,)
        Candidate direction with the highest score.
    best_score : float
        Score of `best_direction`.
    scores : ndarray of shape (n_directions,)
        Score of every candidate direction.
    """
    s = project_reflections_onto_directions(q_vectors, directions)
    scores = periodic_alignment_scores(s, spacing)
    i_best = np.argmax(scores)
    return directions[i_best], scores[i_best], scores


def resolvability_metric(scores, directions, best_dir, exclude_deg=10.0):
    """
    Measure how distinctly the best-scoring direction stands out.

    Parameters
    ----------
    scores : ndarray of shape (n_directions,)
        Scores from `best_direction_from_candidates`.
    directions : ndarray of shape (n_directions, 3)
        Candidate directions corresponding to `scores`.
    best_dir : ndarray of shape (3,)
        Highest-scoring direction.
    exclude_deg : float, optional
        Angular radius, in degrees, around `best_dir` and its antipode for excluding nearby directions.

    Returns
    -------
    rho : float
        Ratio of the score gap outside the excluded region to the median score.
    smax : float
        Score of the best direction.
    s2 : float
        Best score outside the excluded region, or 0 if every direction falls within it.
    """
    exclude = np.deg2rad(exclude_deg)
    directions = np.atleast_2d(directions)
    direction_norms = np.linalg.norm(directions, axis=1)
    if np.any(direction_norms < 1e-15):
        raise ValueError("Cannot normalize near-zero vector.")
    directions_hat = directions / direction_norms[:, None]
    best_dir_hat = normalize(best_dir)
    cos_ang = np.abs(directions_hat @ best_dir_hat)
    ang = np.arccos(np.clip(cos_ang, -1.0, 1.0))
    mask = ang > exclude

    smax = scores.max()
    s2 = scores[mask].max() if np.any(mask) else 0.0
    smed = np.median(scores)
    rho = (smax - s2) / (smed + 1e-15)
    return rho, smax, s2


def choose_best_anchor(q_vectors, coarse_dirs, lengths, exclude_deg=10.0, length_resolution=0.0001):
    """
    Pick which lattice axis gives the most distinctly resolvable direction.

    Parameters
    ----------
    q_vectors : ndarray of shape (n_peaks, 3)
        Peak positions in the Q-sample frame, in inverse angstroms.
    coarse_dirs : ndarray of shape (n_directions, 3)
        Candidate unit directions to search over.
    lengths : dict
        Mapping of axis names ("a", "b", "c") to real-space lattice lengths.
    exclude_deg : float, optional
        Passed to `resolvability_metric`.
    length_resolution : float, optional
        Lattice-length resolution, in angstroms; closer lengths are scored once.

    Returns
    -------
    best_name : str
        Name of the axis with the highest resolvability.
    results : dict
        Mapping of axis name to a dict with keys "dir", "score", "scores", "resolvability", "smax", and "s2".
    """
    results = {}
    cache = []
    for name, d in lengths.items():
        cached = next((item for item in cache if abs(d - item[0]) < length_resolution), None)
        if cached is None:
            best_dir, best_score, scores = best_direction_from_candidates(q_vectors, coarse_dirs, d)
            rho, smax, s2 = resolvability_metric(scores, coarse_dirs, best_dir, exclude_deg)
            cache.append((d, best_dir, best_score, scores, rho, smax, s2))
        else:
            _, best_dir, best_score, scores, rho, smax, s2 = cached
        results[name] = {
            "dir": normalize(best_dir),
            "score": best_score,
            "scores": scores,
            "resolvability": rho,
            "smax": smax,
            "s2": s2,
        }

    best_name = max(results, key=lambda k: results[k]["resolvability"])
    return best_name, results


def cone_pair_from_anchor(anchor_hat, ang1, ang2, mutual_ang, psi):
    """
    Sample directions on two cones around an anchor direction.

    For each azimuth in `psi`, generates candidate directions for the two non-anchor axes at the requested angles.

    Both azimuthal offset directions (`+delta` and `-delta`) are returned because angles alone do not set the sign.

    Parameters
    ----------
    anchor_hat : ndarray of shape (3,)
        Anchor axis direction.
    ang1 : float
        Angle between the anchor and the first candidate axis, in radians.
    ang2 : float
        Angle between the anchor and the second candidate axis, in radians.
    mutual_ang : float
        Angle between the first and second candidate axes, in radians.
    psi : ndarray
        Azimuthal angles, in radians, around the anchor at which to sample.

    Returns
    -------
    x_dirs : ndarray of shape (len(psi), 3)
        Candidate directions for the first axis, at azimuth `psi`.
    y_dirs_plus : ndarray of shape (len(psi), 3)
        Candidate directions for the second axis, at azimuth `psi + delta`.
    y_dirs_minus : ndarray of shape (len(psi), 3)
        Candidate directions for the second axis, at azimuth `psi - delta`.
    """
    anchor_hat = normalize(anchor_hat)
    u, v = orthonormal_frame(anchor_hat)

    denom = np.sin(ang1) * np.sin(ang2)
    if abs(denom) < 1e-14:
        raise ValueError("Degenerate cone geometry.")

    rhs = (np.cos(mutual_ang) - np.cos(ang1) * np.cos(ang2)) / denom
    rhs = np.clip(rhs, -1.0, 1.0)
    delta = np.arccos(rhs)

    ring1 = np.cos(psi)[:, None] * u[None, :] + np.sin(psi)[:, None] * v[None, :]
    ring2_plus = np.cos(psi + delta)[:, None] * u[None, :] + np.sin(psi + delta)[:, None] * v[None, :]
    ring2_minus = np.cos(psi - delta)[:, None] * u[None, :] + np.sin(psi - delta)[:, None] * v[None, :]

    x_dirs = np.cos(ang1) * anchor_hat[None, :] + np.sin(ang1) * ring1
    y_dirs_plus = np.cos(ang2) * anchor_hat[None, :] + np.sin(ang2) * ring2_plus
    y_dirs_minus = np.cos(ang2) * anchor_hat[None, :] + np.sin(ang2) * ring2_minus

    return x_dirs, y_dirs_plus, y_dirs_minus


def right_handed_dirs2(anchor_hat, name_anchor, name1, name2, dirs1, dirs2_plus, dirs2_minus):
    """
    Pick the `dirs2_plus` or `dirs2_minus` branch that gives a right-handed (a, b, c) frame.

    The `+delta` and `-delta` branches are mirror images, so both satisfy the requested inter-axial angles equally well.
    Only handedness differs, and varying `psi` cannot change chirality, so handedness only needs checking once.

    Parameters
    ----------
    anchor_hat : ndarray of shape (3,)
        Anchor axis direction.
    name_anchor, name1, name2 : str
        Names ("a", "b", or "c") of the anchor and two candidate axes.
    dirs1 : ndarray of shape (n_psi, 3)
        Candidate directions for `name1`.
    dirs2_plus, dirs2_minus : ndarray of shape (n_psi, 3)
        Candidate directions for `name2`, for each azimuthal-offset branch.

    Returns
    -------
    dirs2 : ndarray of shape (n_psi, 3)
        Whichever of `dirs2_plus`/`dirs2_minus` is right-handed.
    branch : int
        +1 if `dirs2_plus` was right-handed, -1 if `dirs2_minus` was.
    """
    hats = {name_anchor: anchor_hat, name1: dirs1[0], name2: dirs2_plus[0]}
    M = np.column_stack([hats["a"], hats["b"], hats["c"]])

    if np.linalg.det(M) > 0:
        return dirs2_plus, +1
    return dirs2_minus, -1


def joint_pair_search_from_anchor(q_vectors, anchor_hat, name_anchor, a, b, c, alpha, beta, gamma, n_psi=720):
    """
    Find the pair of non-anchor axes best matching the observed reflections.

    Scans the azimuthal angle around the anchor axis over the full circle and, at each value, scores candidate
    directions for the two remaining lattice axes using `cone_pair_from_anchor`. Of its two azimuthal-offset branches
    (`+delta` and `-delta`), only the one that keeps (a, b, c) right-handed is searched -- see `right_handed_dirs2`.

    Parameters
    ----------
    q_vectors : ndarray of shape (n_peaks, 3)
        Peak positions in the Q-sample frame, in inverse angstroms.
    anchor_hat : ndarray of shape (3,)
        Anchor axis direction.
    name_anchor : str
        Name of the anchor axis: "a", "b", or "c".
    a, b, c : float
        Primitive lattice lengths.
    alpha, beta, gamma : float
        Primitive lattice angles, in radians.
    n_psi : int, optional
        Number of azimuthal samples over the full circle.

    Returns
    -------
    name1, name2 : str
        Names of the two non-anchor axes.
    vec1, vec2 : ndarray of shape (3,)
        Best-scoring directions for `name1` and `name2`.
    joint_score : float
        Combined score of `vec1` and `vec2`.
    branch : int
        +1 if the `+delta` azimuthal offset was the right-handed (and hence searched) branch, -1 if `-delta` was.
    """
    psi = np.linspace(0.0, 2.0 * np.pi, n_psi, endpoint=False)

    if name_anchor == "a":
        name1, d1, ang1 = "b", b, gamma
        name2, d2, ang2 = "c", c, beta
        mutual = alpha
    elif name_anchor == "b":
        name1, d1, ang1 = "a", a, gamma
        name2, d2, ang2 = "c", c, alpha
        mutual = beta
    else:
        name1, d1, ang1 = "a", a, beta
        name2, d2, ang2 = "b", b, alpha
        mutual = gamma

    dirs1, dirs2_plus, dirs2_minus = cone_pair_from_anchor(anchor_hat, ang1, ang2, mutual, psi)

    dirs2, branch = right_handed_dirs2(anchor_hat, name_anchor, name1, name2, dirs1, dirs2_plus, dirs2_minus)

    R1 = periodic_alignment_scores(project_reflections_onto_directions(q_vectors, dirs1), d1)
    R2 = periodic_alignment_scores(project_reflections_onto_directions(q_vectors, dirs2), d2)

    joint = R1 * R2
    i_best = np.argmax(joint)

    return name1, name2, dirs1[i_best], dirs2[i_best], joint[i_best], branch


def refine_pair_from_anchor_local(
    q_vectors,
    anchor_hat,
    name_anchor,
    a,
    b,
    c,
    alpha,
    beta,
    gamma,
    psi_center,
    psi_half_width_deg=3.0,
    n_psi=1440,
):
    """
    Refine the coarse azimuthal placement of the two non-anchor axes around the fixed anchor.

    The joint search first finds the coarse azimuth around the anchor that best scores the two non-anchor axes.
    Only the azimuthal-offset branch that keeps (a, b, c) right-handed is searched.

    Parameters
    ----------
    q_vectors : ndarray of shape (n_peaks, 3)
        Peak positions in the Q-sample frame, in inverse angstroms.
    anchor_hat : ndarray of shape (3,)
        Anchor axis direction.
    name_anchor : str
        Name of the anchor axis: "a", "b", or "c".
    a, b, c : float
        Primitive lattice lengths.
    alpha, beta, gamma : float
        Primitive lattice angles, in radians.
    psi_center : float
        Azimuthal angle, in radians, to center the search window on.
    psi_half_width_deg : float, optional
        Half-width of the search window, in degrees.
    n_psi : int, optional
        Number of azimuthal samples within the window.

    Returns
    -------
    name1, name2 : str
        Names of the two non-anchor axes.
    vec1, vec2 : ndarray of shape (3,)
        Best-scoring directions for `name1` and `name2`.
    joint_score : float
        Combined score of `vec1` and `vec2`.
    psi_best : float
        Azimuthal angle, in radians, that gave `joint_score`.
    branch : int
        +1 if the `+delta` azimuthal offset was the right-handed (and hence searched) branch, -1 if `-delta` was.
    """
    psi_half_width = np.deg2rad(psi_half_width_deg)
    psi = np.linspace(
        psi_center - psi_half_width,
        psi_center + psi_half_width,
        n_psi,
        endpoint=True,
    )

    if name_anchor == "a":
        name1, d1, ang1 = "b", b, gamma
        name2, d2, ang2 = "c", c, beta
        mutual = alpha
    elif name_anchor == "b":
        name1, d1, ang1 = "a", a, gamma
        name2, d2, ang2 = "c", c, alpha
        mutual = beta
    else:
        name1, d1, ang1 = "a", a, beta
        name2, d2, ang2 = "b", b, alpha
        mutual = gamma

    dirs1, dirs2_plus, dirs2_minus = cone_pair_from_anchor(anchor_hat, ang1, ang2, mutual, psi)

    dirs2, branch = right_handed_dirs2(anchor_hat, name_anchor, name1, name2, dirs1, dirs2_plus, dirs2_minus)

    R1 = periodic_alignment_scores(project_reflections_onto_directions(q_vectors, dirs1), d1)
    R2 = periodic_alignment_scores(project_reflections_onto_directions(q_vectors, dirs2), d2)

    joint = R1 * R2
    i_best = np.argmax(joint)

    return (
        name1,
        name2,
        dirs1[i_best],
        dirs2[i_best],
        joint[i_best],
        psi[i_best],
        branch,
    )


def estimate_frame_from_q(
    q_vectors,
    a,
    b,
    c,
    alpha,
    beta,
    gamma,
    coarse_dirs,
    cap_angle_deg=3.0,
    cap_samples=400,
    n_psi=720,
    rng=None,
):
    """
    Estimate lab-frame directions for the primitive a, b, c axes from a set of peak positions in the sample frame.

    Chooses the most resolvable axis as an anchor, refines it, then finds the remaining axes by azimuthal search.

    Parameters
    ----------
    q_vectors : ndarray of shape (n_peaks, 3)
        Peak positions in the Q-sample frame, in inverse angstroms.
    a, b, c : float
        Primitive lattice lengths.
    alpha, beta, gamma : float
        Primitive lattice angles, in radians.
    coarse_dirs : ndarray of shape (n_directions, 3)
        Candidate unit directions for the initial anchor search.
    cap_angle_deg : float, optional
        Half-angle, in degrees, of the local refinement cap around the anchor and coarse azimuthal estimate.
    cap_samples : int, optional
        Number of directions sampled when refining the anchor.
    n_psi : int, optional
        Number of azimuthal samples used in the coarse and local searches.
    rng : numpy.random.Generator, optional
        Random number generator used to sample the anchor refinement cap.

    Returns
    -------
    a_hat, b_hat, c_hat : ndarray of shape (3,)
        Estimated lab-frame directions of the primitive a, b, c axes.
    info : dict
        Diagnostics with keys "anchor_name", "anchor_results", "joint_score", "psi_best", and "branch".
    """
    if rng is None:
        rng = np.random.default_rng(1234)

    lengths = {"a": a, "b": b, "c": c}
    anchor_name, anchor_results = choose_best_anchor(q_vectors, coarse_dirs, lengths)

    anchor_hat0 = anchor_results[anchor_name]["dir"]
    anchor_refine = sample_spherical_cap(anchor_hat0, cap_angle_deg, cap_samples, rng=rng)
    anchor_hat, _, _ = best_direction_from_candidates(q_vectors, anchor_refine, lengths[anchor_name])
    anchor_hat = normalize(anchor_hat)

    name1, name2, vec1, vec2, joint_score, branch = joint_pair_search_from_anchor(
        q_vectors, anchor_hat, anchor_name, a, b, c, alpha, beta, gamma, n_psi=n_psi
    )

    u, v = orthonormal_frame(anchor_hat)
    tangential = normalize(vec1 - np.dot(vec1, anchor_hat) * anchor_hat)
    psi_center = np.arctan2(np.dot(tangential, v), np.dot(tangential, u))

    name1, name2, vec1, vec2, joint_score, psi_best, branch = refine_pair_from_anchor_local(
        q_vectors, anchor_hat, anchor_name, a, b, c, alpha, beta, gamma, psi_center, psi_half_width_deg=cap_angle_deg, n_psi=n_psi
    )

    axes = {
        anchor_name: anchor_hat,
        name1: normalize(vec1),
        name2: normalize(vec2),
    }

    return (
        axes["a"],
        axes["b"],
        axes["c"],
        {
            "anchor_name": anchor_name,
            "anchor_results": anchor_results,
            "joint_score": joint_score,
            "psi_best": psi_best,
            "branch": branch,
        },
    )


def resolve_axis_length_degeneracy(q_vectors, a, b, c, a_hat, b_hat, c_hat, T_cp, tol=0.05):
    """
    Disambiguate near-degenerate axis lengths by comparing global fit quality.

    The per-axis periodicity score loses resolving power when two axis lengths are close.

    This tries swapping near-degenerate fitted directions and keeps the assignment with the lowest global hkl RMS.

    Parameters
    ----------
    q_vectors : ndarray of shape (n_peaks, 3)
        Peak positions in the Q-sample frame, in inverse angstroms.
    a, b, c : float
        Primitive lattice lengths.
    a_hat, b_hat, c_hat : ndarray of shape (3,)
        Estimated lab-frame directions of the primitive a, b, c axes, as returned by `estimate_frame_from_q`.
    T_cp : ndarray of shape (3, 3)
        Transform from the conventional to the primitive direct-lattice basis.
    tol : float, optional
        Relative length difference for treating two axes as degenerate.

    Returns
    -------
    a_hat, b_hat, c_hat : ndarray of shape (3,)
        Directions for the axes, possibly swapped from the input.
    UB_conv : ndarray of shape (3, 3)
        UB matrix indexed on the conventional cell, for the chosen assignment.
    hkl_conv : ndarray of shape (n_peaks, 3)
        Fractional Miller indices for the chosen assignment.
    rms_hkl : float
        RMS deviation of `hkl_conv` from the nearest integers.
    swap_applied : str
        Which pair of input axes was swapped: "none", "ab", "ac", or "bc".
    """
    lengths = {"a": a, "b": b, "c": c}
    hats = {"a": a_hat, "b": b_hat, "c": c_hat}

    def _evaluate(cand_hats):
        A_p_est = direct_basis_from_axes(a, b, c, cand_hats["a"], cand_hats["b"], cand_hats["c"])
        UB_p = reciprocal_basis_from_direct_basis(A_p_est)
        UB_conv = primitive_ub_to_conventional_ub(UB_p, T_cp)
        hkl_conv = np.linalg.solve(UB_conv, q_vectors.T).T
        err = hkl_conv - np.rint(hkl_conv)
        rms = np.sqrt(np.mean(err**2))
        return UB_conv, hkl_conv, rms

    candidates = [("none", hats)]
    for x, y in (("a", "b"), ("a", "c"), ("b", "c")):
        if abs(lengths[x] - lengths[y]) <= tol * max(lengths[x], lengths[y]):
            (z,) = [ax for ax in ("a", "b", "c") if ax not in (x, y)]
            swapped = dict(hats)
            swapped[x], swapped[y] = hats[y], hats[x]
            # Swapping two axes alone is an odd permutation (flips the
            # frame left-handed); negating the third axis compensates
            # -- exact for the orthorhombic-or-higher-symmetry case this
            # degeneracy check mainly targets, since 90 degree angles are
            # unchanged by negating one axis (self-complementary under
            # angle -> 180 - angle).
            swapped[z] = -hats[z]
            candidates.append((x + y, swapped))

    best = None
    for swap_applied, cand_hats in candidates:
        UB_conv, hkl_conv, rms = _evaluate(cand_hats)
        if best is None or rms < best[-1]:
            best = (cand_hats, UB_conv, hkl_conv, swap_applied, rms)

    cand_hats, UB_conv, hkl_conv, swap_applied, rms_hkl = best
    return (
        cand_hats["a"],
        cand_hats["b"],
        cand_hats["c"],
        UB_conv,
        hkl_conv,
        rms_hkl,
        swap_applied,
    )


def centering_mask(h, k, l, centering):
    """
    Test integer Miller indices against a centering's reflection condition.

    In a centered lattice, not every integer (h, k, l) reflection is allowed.
    The centering introduces systematic absences. This function returns a boolean mask marking
    which `(h, k, l)` values satisfy the requested centering and which are systematic absences.

    Parameters
    ----------
    h : ndarray
        Integer Miller h indices.
    k : ndarray
        Integer Miller k indices.
    l : ndarray
        Integer Miller l indices.
    centering : str
        Centering symbol: "P", "A", "B", "C", "I", "F", or "R".

    Returns
    -------
    allowed : ndarray of bool
        True where `(h, k, l)` satisfies the centering condition.
    """
    centering = centering.upper()
    if centering == "P":
        return np.ones_like(h, dtype=bool)
    if centering == "I":
        return (h + k + l) % 2 == 0
    if centering == "F":
        return ((h % 2) == (k % 2)) & ((k % 2) == (l % 2))
    if centering == "A":
        return (k + l) % 2 == 0
    if centering == "B":
        return (h + l) % 2 == 0
    if centering == "C":
        return (h + k) % 2 == 0
    if centering == "R":
        return (-h + k + l) % 3 == 0
    raise ValueError("Unknown centering '{}'.".format(centering))


def score_centering_condition(hkl_frac, centering, tol=0.2):
    """
    Fraction of near-integer reflections consistent with a centering.

    Parameters
    ----------
    hkl_frac : ndarray of shape (n_peaks, 3)
        Fractional Miller indices, as computed from a candidate UB matrix.
    centering : str
        Centering symbol: "P", "A", "B", "C", "I", "F", or "R".
    tol : float, optional
        Maximum deviation from an integer, per index, to count a reflection as near-integer.

    Returns
    -------
    fraction_ok : float
        Fraction of near-integer reflections that also satisfy the centering condition; 0 if none are near-integer.
    info : dict
        Counts with keys "n_total", "n_near_integer", and "n_centering_ok".
    """
    hkl_frac = np.asarray(hkl_frac, dtype=float)
    hkl_round = np.rint(hkl_frac).astype(int)
    residual = np.abs(hkl_frac - hkl_round)

    near_integer = np.all(residual < tol, axis=1)
    if not np.any(near_integer):
        return 0.0, {
            "n_total": len(hkl_frac),
            "n_near_integer": 0,
            "n_centering_ok": 0,
        }

    h = hkl_round[near_integer, 0]
    k = hkl_round[near_integer, 1]
    l = hkl_round[near_integer, 2]
    ok = centering_mask(h, k, l, centering)

    return float(np.mean(ok)), {
        "n_total": len(hkl_frac),
        "n_near_integer": int(np.sum(near_integer)),
        "n_centering_ok": int(np.sum(ok)),
    }


class FindUBFromConventionalCell(PythonAlgorithm):
    """
    Determine a UB matrix from unindexed peaks and known conventional-cell lattice parameters.

    Reduces the conventional cell to its primitive setting, searches for the lab-frame directions of the primitive a, b,
    c axes by scoring how well peak Q-vectors align with periodic projections along candidate directions, then converts
    the resulting primitive UB back to the conventional cell.
    """

    def category(self):
        return "Crystal\\UBMatrix"

    def name(self):
        return "FindUBFromConventionalCell"

    def summary(self):
        return "Determine UB from a PeaksWorkspace and conventional-cell lattice parameters."

    def seeAlso(self):
        return [
            "FindUBUsingLatticeParameters",
            "FindUBUsingFFT",
            "FindUBFromScatteringPlane",
            "IndexPeaks",
        ]

    def PyInit(self):
        """
        Declare the algorithm's input and output properties.
        """
        self.declareProperty(
            IPeaksWorkspaceProperty("PeaksWorkspace", "", Direction.InOut),
            doc="Peaks workspace with unindexed peaks; the found UB is set on its oriented lattice.",
        )

        positive = FloatBoundedValidator(lower=0.1)
        self.declareProperty("a", 10.0, positive, doc="Conventional-cell lattice length a (angstrom).")
        self.declareProperty("b", 10.0, positive, doc="Conventional-cell lattice length b (angstrom).")
        self.declareProperty("c", 10.0, positive, doc="Conventional-cell lattice length c (angstrom).")

        self.declareProperty("alpha", 90.0, doc="Conventional-cell lattice angle alpha (degrees).")
        self.declareProperty("beta", 90.0, doc="Conventional-cell lattice angle beta (degrees).")
        self.declareProperty("gamma", 90.0, doc="Conventional-cell lattice angle gamma (degrees).")

        self.declareProperty(
            "Centering",
            "P",
            StringListValidator(["P", "A", "B", "C", "I", "F", "R"]),
            doc="Centering of the conventional cell.",
        )

        self.declareProperty(
            "NumAzimuth",
            240,
            IntBoundedValidator(lower=8),
            doc="Number of azimuthal samples in the coarse direction grid.",
        )
        self.declareProperty(
            "NumPolar",
            120,
            IntBoundedValidator(lower=8),
            doc="Number of polar samples in the coarse direction grid.",
        )
        self.declareProperty("CapAngleDeg", 10.0, doc="Half-angle (degrees) of the local refinement cap.")
        self.declareProperty(
            "CapSamples",
            10000,
            IntBoundedValidator(lower=10),
            doc="Number of random directions sampled when refining the anchor axis.",
        )
        self.declareProperty(
            "NumPsi",
            1440,
            IntBoundedValidator(lower=30),
            doc="Number of azimuthal samples used in the axis-pair search.",
        )
        self.declareProperty("RandomSeed", 1234, doc="Seed for the random-number generator used in the cap refinement.")
        self.declareProperty(
            "Tolerance",
            0.2,
            doc="Maximum per-index deviation from an integer to count a reflection as indexed.",
        )
        self.declareProperty(
            "AxisDegeneracyTolerance",
            0.05,
            doc="Relative length difference below which two axes are treated as degenerate.",
        )

        self.declareProperty(
            TableWorkspaceProperty("DiagnosticTable", "diagnostics_table", Direction.Output),
            doc="Table of fit diagnostics (RMS, scores, primitive cell, axis swap, centering counts).",
        )
        self.declareProperty(
            WorkspaceProperty(
                "ProjectionHistograms",
                "projection_histograms",
                Direction.Output,
            ),
            doc="Histograms of the peak Q-vectors projected onto the fitted a, b, c axes.",
        )

    def validateInputs(self):
        """
        Check property values and peak geometry before execution.

        Returns
        -------
        issues : dict
            Mapping of property name to an error message, for properties that fail validation.
        """
        issues = {}

        peaks = self.getProperty("PeaksWorkspace").value
        if peaks is None:
            issues["PeaksWorkspace"] = "A PeaksWorkspace is required."
            return issues

        if peaks.getNumberPeaks() < 3:
            issues["PeaksWorkspace"] = "At least 3 peaks are required."
        else:
            q_norms = [np.linalg.norm(peaks.getPeak(i).getQSampleFrame()) for i in range(peaks.getNumberPeaks())]
            if any(norm < 1e-10 for norm in q_norms):
                issues["PeaksWorkspace"] = "All peaks must have non-zero Q vectors."
            else:
                non_coincident_pairs = 0
                for i in range(peaks.getNumberPeaks() - 1):
                    q0 = peaks.getPeak(i).getQSampleFrame()
                    q1 = peaks.getPeak(i + 1).getQSampleFrame()
                    cos_theta = np.clip(np.dot(q0, q1) / (q_norms[i] * q_norms[i + 1]), -1.0, 1.0)
                    if np.rad2deg(np.arccos(cos_theta)) > 10:
                        non_coincident_pairs += 1
                if non_coincident_pairs < 2:
                    issues["PeaksWorkspace"] = "At least 3 peaks with distinct Q directions are required."

        for name in ["alpha", "beta", "gamma"]:
            ang = self.getProperty(name).value
            if not (10.0 < ang < 170.0):
                issues[name] = "Angle must satisfy 10 < angle < 170 degrees."

        centering = self.getProperty("Centering").value.upper()
        a = self.getProperty("a").value
        b = self.getProperty("b").value
        alpha = self.getProperty("alpha").value
        beta = self.getProperty("beta").value
        gamma = self.getProperty("gamma").value

        if centering == "R":
            r_issues = []
            if not np.isclose(a, b, atol=1e-6):
                r_issues.append("R centering expects conventional hexagonal input with a=b.")
            if not np.isclose(alpha, 90.0, atol=1e-6) or not np.isclose(beta, 90.0, atol=1e-6):
                r_issues.append("R centering expects alpha=beta=90.")
            if not np.isclose(gamma, 120.0, atol=1e-6):
                r_issues.append("R centering expects gamma=120.")
            if r_issues:
                issues["Centering"] = " ".join(r_issues)

        return issues

    def _extract_q_vectors(self, peaks_ws):
        """
        Read peak positions from a workspace into an array.

        Mantid's `getQSampleFrame` follows `Q = 2 * pi * UB @ hkl`; this module divides out `2 * pi`.

        Parameters
        ----------
        peaks_ws : PeaksWorkspace
            Workspace containing the peaks to index.

        Returns
        -------
        q_vectors : ndarray of shape (n_peaks, 3)
            Peak positions in the Q-sample frame, in inverse angstroms.
        """
        q_vectors = []
        for i in range(peaks_ws.getNumberPeaks()):
            pk = peaks_ws.getPeak(i)
            q = pk.getQSampleFrame()
            q_vectors.append([q.X(), q.Y(), q.Z()])
        return np.asarray(q_vectors, dtype=float) / (2.0 * np.pi)

    def _assign_ub_to_workspace(self, peaks_ws, UB_conv):
        """
        Attach a UB matrix to a peaks workspace's oriented lattice.

        `SetUB` and `OrientedLattice.setUB` use this module's no-`2 * pi` UB convention, so `UB_conv` is stored as-is.

        Parameters
        ----------
        peaks_ws : PeaksWorkspace
            Workspace to update.
        UB_conv : ndarray of shape (3, 3)
            UB matrix indexed on the conventional cell.
        """
        sample = peaks_ws.mutableSample()
        try:
            ol = sample.getOrientedLattice()
        except RuntimeError:
            SetUB(Workspace=peaks_ws, UB=UB_conv)
            return

        ol = sample.getOrientedLattice()
        ol.setUB(UB_conv)

    def _make_diagnostic_table(
        self,
        info,
        rms_hkl,
        centering_score,
        centering_info,
        lattice_p,
        swap_applied,
    ):
        """
        Build a table workspace summarizing the fit diagnostics.

        Parameters
        ----------
        info : dict
            Diagnostics returned by `estimate_frame_from_q`.
        rms_hkl : float
            RMS deviation of fitted Miller indices from their nearest integers.
        centering_score : float
            Fraction of near-integer reflections consistent with the requested centering.
        centering_info : dict
            Counts returned by `score_centering_condition`.
        lattice_p : tuple
            Primitive lattice parameters `(a, b, c, alpha, beta, gamma)`, with angles in radians.
        swap_applied : str
            Axis pair swapped by `resolve_axis_length_degeneracy`: "none", "ab", "ac", or "bc".

        Returns
        -------
        table : TableWorkspace
            Table with one "Metric"/"Value" row per diagnostic.
        """
        table = CreateEmptyTableWorkspace()
        table.addColumn("str", "Metric")
        table.addColumn("double", "Value")

        swap_code = {"none": 0.0, "ab": 1.0, "ac": 2.0, "bc": 3.0}[swap_applied]

        rows = [
            ("rms_hkl", float(rms_hkl)),
            ("centering_score", float(centering_score)),
            ("joint_score", float(info["joint_score"])),
            ("psi_best_deg", float(np.rad2deg(info["psi_best"]))),
            ("branch", float(info["branch"])),
            ("axis_swap_applied", swap_code),
            ("primitive_a", float(lattice_p[0])),
            ("primitive_b", float(lattice_p[1])),
            ("primitive_c", float(lattice_p[2])),
            ("primitive_alpha_deg", float(np.rad2deg(lattice_p[3]))),
            ("primitive_beta_deg", float(np.rad2deg(lattice_p[4]))),
            ("primitive_gamma_deg", float(np.rad2deg(lattice_p[5]))),
            ("n_total", float(centering_info["n_total"])),
            ("n_near_integer", float(centering_info["n_near_integer"])),
            ("n_centering_ok", float(centering_info["n_centering_ok"])),
        ]

        for metric, value in rows:
            table.addRow([metric, value])

        return table

    def _make_projection_histograms(self, q_vectors, a_hat, b_hat, c_hat, a, b, c):
        """
        Build histograms of Q-vectors projected onto the fitted a, b, c axes.

        Parameters
        ----------
        q_vectors : ndarray of shape (n_peaks, 3)
            Peak positions in the Q-sample frame, in inverse angstroms.
        a_hat, b_hat, c_hat : ndarray of shape (3,)
            Estimated lab-frame directions of the primitive a, b, c axes.
        a, b, c : float
            Primitive lattice lengths.

        Returns
        -------
        ws : Workspace2D
            Workspace with 3 spectra ("h", "k", "l") of projected reflection counts binned over a common axis.
        """
        h_proj = project_reflections_onto_directions(q_vectors, a_hat).ravel() * a
        k_proj = project_reflections_onto_directions(q_vectors, b_hat).ravel() * b
        l_proj = project_reflections_onto_directions(q_vectors, c_hat).ravel() * c

        all_vals = np.concatenate([h_proj, k_proj, l_proj])
        lo = np.floor(all_vals.min()) - 0.5
        hi = np.ceil(all_vals.max()) + 0.5
        bins = np.linspace(lo, hi, 301)

        h_hist, edges = np.histogram(h_proj, bins=bins)
        k_hist, _ = np.histogram(k_proj, bins=bins)
        l_hist, _ = np.histogram(l_proj, bins=bins)

        x = 0.5 * (edges[:-1] + edges[1:])

        ws = CreateWorkspace(
            DataX=np.concatenate([x, x, x]),
            DataY=np.concatenate([h_hist, k_hist, l_hist]),
            NSpec=3,
            UnitX="Label",
            VerticalAxisUnit="Text",
            VerticalAxisValues=["h", "k", "l"],
        )
        return ws

    def PyExec(self):
        """
        Estimate the primitive-cell orientation and assign the conventional UB.
        """
        peaks_ws = self.getProperty("PeaksWorkspace").value

        a = self.getProperty("a").value
        b = self.getProperty("b").value
        c = self.getProperty("c").value
        alpha_deg = self.getProperty("alpha").value
        beta_deg = self.getProperty("beta").value
        gamma_deg = self.getProperty("gamma").value
        centering = self.getProperty("Centering").value.upper()

        alpha = np.deg2rad(alpha_deg)
        beta = np.deg2rad(beta_deg)
        gamma = np.deg2rad(gamma_deg)

        n_az = self.getProperty("NumAzimuth").value
        n_pol = self.getProperty("NumPolar").value
        cap_angle_deg = self.getProperty("CapAngleDeg").value
        cap_samples = self.getProperty("CapSamples").value
        n_psi = self.getProperty("NumPsi").value
        seed = self.getProperty("RandomSeed").value
        integer_tol = self.getProperty("Tolerance").value
        degeneracy_tol = self.getProperty("AxisDegeneracyTolerance").value

        # Mantid's "Inelastic" Q convention is Q = 2 * pi * UB @ hkl;
        # this module's formulas assume Q = UB @ hkl.
        q_vectors = self._extract_q_vectors(peaks_ws)

        uc_conv = UnitCell(a, b, c, alpha_deg, beta_deg, gamma_deg)
        self.log().information(
            "Input conventional cell: "
            f"a={uc_conv.a():.6f}, b={uc_conv.b():.6f}, c={uc_conv.c():.6f}, "
            f"alpha={uc_conv.alpha():.6f}, beta={uc_conv.beta():.6f}, gamma={uc_conv.gamma():.6f}, "
            f"centering={centering}"
        )

        if centering == "P":
            lattice_solve = (a, b, c, alpha, beta, gamma)
            T_cp = np.eye(3)
        else:
            lattice_solve, T_cp = conventional_to_primitive_lattice(a, b, c, alpha, beta, gamma, centering)

        a_p, b_p, c_p, alpha_p, beta_p, gamma_p = lattice_solve

        coarse_dirs = sample_hemisphere_grid(n_az=n_az, n_pol=n_pol)

        a_hat, b_hat, c_hat, info = estimate_frame_from_q(
            q_vectors=q_vectors,
            a=a_p,
            b=b_p,
            c=c_p,
            alpha=alpha_p,
            beta=beta_p,
            gamma=gamma_p,
            coarse_dirs=coarse_dirs,
            cap_angle_deg=cap_angle_deg,
            cap_samples=cap_samples,
            n_psi=n_psi,
            rng=np.random.default_rng(seed),
        )

        a_hat, b_hat, c_hat, UB_conv, hkl_conv, rms_hkl, swap_applied = resolve_axis_length_degeneracy(
            q_vectors, a_p, b_p, c_p, a_hat, b_hat, c_hat, T_cp, tol=degeneracy_tol
        )
        if swap_applied != "none":
            self.log().notice(
                "Primitive axis lengths nearly degenerate "
                f"(tol={degeneracy_tol}): swapped the '{swap_applied[0]}' "
                f"and '{swap_applied[1]}' directions based on the "
                "global hkl fit."
            )

        centering_score, centering_info = score_centering_condition(hkl_conv, centering, tol=integer_tol)

        self._assign_ub_to_workspace(peaks_ws, UB_conv)

        diag_table = self._make_diagnostic_table(
            info,
            rms_hkl,
            centering_score,
            centering_info,
            lattice_solve,
            swap_applied,
        )
        self.setProperty("DiagnosticTable", diag_table)

        proj_ws = self._make_projection_histograms(q_vectors, a_hat, b_hat, c_hat, a_p, b_p, c_p)
        self.setProperty("ProjectionHistograms", proj_ws)

        self.setProperty("PeaksWorkspace", peaks_ws)


AlgorithmFactory.subscribe(FindUBFromConventionalCell)
