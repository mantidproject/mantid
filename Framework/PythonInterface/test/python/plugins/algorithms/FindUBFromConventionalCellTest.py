# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# ruff: noqa: E741  # Ambiguous variable name (h, k, l are Miller indices)
import unittest
import numpy as np

from mantid.simpleapi import CreatePeaksWorkspace, SetUB
from mantid.kernel import V3D
from mantid.api import AlgorithmManager

import plugins.algorithms.FindUBFromConventionalCell as find_ub_module


def rotation_matrix_from_axis_angle(axis, angle_rad):
    axis = np.asarray(axis, dtype=float)
    axis = axis / np.linalg.norm(axis)
    x, y, z = axis
    c = np.cos(angle_rad)
    s = np.sin(angle_rad)
    C = 1.0 - c

    return np.array(
        [
            [c + x * x * C, x * y * C - z * s, x * z * C + y * s],
            [y * x * C + z * s, c + y * y * C, y * z * C - x * s],
            [z * x * C - y * s, z * y * C + x * s, c + z * z * C],
        ]
    )


def direct_basis_from_lattice(a, b, c, alpha, beta, gamma):
    ca, cb, cg = np.cos(alpha), np.cos(beta), np.cos(gamma)
    sg = np.sin(gamma)
    volume_factor = 1.0 + 2.0 * ca * cb * cg - ca**2 - cb**2 - cg**2
    return np.array(
        [
            [a, b * cg, c * cb],
            [0.0, b * sg, c * (ca - cb * cg) / sg],
            [0.0, 0.0, c * np.sqrt(volume_factor) / sg],
        ],
        dtype=float,
    )


def reciprocal_basis_from_lattice(a, b, c, alpha, beta, gamma):
    A = direct_basis_from_lattice(a, b, c, alpha, beta, gamma)
    return np.linalg.inv(A).T


class FindUBFromConventionalCellTest(unittest.TestCase):
    def _make_synthetic_peaks_workspace(self, UB, hkls, dhkl=0.05):
        ws = CreatePeaksWorkspace(NumberOfPeaks=0, OutputType="LeanElasticPeak")

        SetUB(Workspace=ws, UB=UB)

        np.random.seed(26)

        for h, k, l in hkls:
            rand = 2 * np.random.random(3) - 1
            hkl = np.array([h, k, l], dtype=float) + dhkl * rand
            peak = ws.createPeakHKL(V3D(hkl[0], hkl[1], hkl[2]))
            peak.setHKL(h, k, l)
            ws.addPeak(peak)

        return ws

    def _assert_indexing_and_centering(self, alg, UB_true, hkls):
        ws_out = alg.getProperty("PeaksWorkspace").value
        UB_est = ws_out.sample().getOrientedLattice().getUB().copy()

        q_vectors = np.array([UB_true @ np.array(hkl, dtype=float) for hkl in hkls])
        hkl_est = np.linalg.solve(UB_est, q_vectors.T).T
        err = hkl_est - np.rint(hkl_est)
        rms = np.sqrt(np.mean(err**2))
        self.assertLess(rms, 0.3)

        diagnostics = alg.getProperty("DiagnosticTable").value
        metrics = {diagnostics.cell(row, 0): diagnostics.cell(row, 1) for row in range(diagnostics.rowCount())}
        self.assertAlmostEqual(metrics["centering_score"], 1.0)
        self.assertEqual(metrics["n_near_integer"], float(len(hkls)))
        self.assertEqual(metrics["n_centering_ok"], float(len(hkls)))

    def _validate_peaks(self, hkls):
        ws = CreatePeaksWorkspace(NumberOfPeaks=0, OutputType="LeanElasticPeak")
        SetUB(Workspace=ws, UB=np.eye(3))
        for hkl in hkls:
            ws.addPeak(ws.createPeakHKL(V3D(*hkl)))

        alg = AlgorithmManager.create("FindUBFromConventionalCell")
        alg.initialize()
        alg.setProperty("PeaksWorkspace", ws)
        return alg.validateInputs()

    def test_validation_rejects_zero_q_peak(self):
        issues = self._validate_peaks([[0, 0, 0], [1, 0, 0], [0, 1, 0]])

        self.assertEqual(issues["PeaksWorkspace"], "All peaks must have non-zero Q vectors.")

    def test_validation_reports_distinct_q_direction_requirement(self):
        issues = self._validate_peaks([[1, 0, 0], [2, 0, 0], [3, 0, 0]])

        self.assertEqual(issues["PeaksWorkspace"], "At least 3 peaks with distinct Q directions are required.")

    def test_resolvability_metric_matches_scalar_angular_distance(self):
        directions = np.array(
            [
                [1.0, 0.0, 0.0],
                [0.0, 1.0, 0.0],
                [-1.0, 0.0, 0.0],
                [1.0, 1.0, 0.0],
            ]
        )
        best_dir = np.array([1.0, 0.0, 0.0])
        scores = np.array([0.9, 0.2, 0.8, 0.4])
        exclude = np.deg2rad(10.0)
        scalar_angles = np.array([find_ub_module.angular_distance_antipodal(direction, best_dir) for direction in directions])
        mask = scalar_angles > exclude
        smax = scores.max()
        s2 = scores[mask].max()
        expected = (smax - s2) / (np.median(scores) + 1e-15)

        rho, actual_smax, actual_s2 = find_ub_module.resolvability_metric(scores, directions, best_dir)

        self.assertAlmostEqual(rho, expected)
        self.assertEqual(actual_smax, smax)
        self.assertEqual(actual_s2, s2)

    def test_assign_ub_without_existing_lattice(self):
        ws = CreatePeaksWorkspace(NumberOfPeaks=0, OutputType="LeanElasticPeak")
        alg = find_ub_module.FindUBFromConventionalCell()
        alg.initialize()
        ub = np.diag([0.1, 0.2, 0.3])

        alg._assign_ub_to_workspace(ws, ub)

        np.testing.assert_allclose(ws.sample().getOrientedLattice().getUB(), ub)

    def test_recovers_primitive_hexagonal_like_case(self):
        a, b, c = 6.0, 6.0, 8.0
        alpha_deg, beta_deg, gamma_deg = 90.0, 90.0, 120.0
        alpha = np.deg2rad(alpha_deg)
        beta = np.deg2rad(beta_deg)
        gamma = np.deg2rad(gamma_deg)

        B = reciprocal_basis_from_lattice(a, b, c, alpha, beta, gamma)
        U = rotation_matrix_from_axis_angle([1.0, 2.0, 3.0], np.deg2rad(37.0))
        UB_true = U @ B

        hkls = [
            [1, 0, 0],
            [0, 1, 0],
            [1, 1, 0],
            [0, 0, 1],
            [1, 0, 1],
            [2, 0, 0],
            [0, 2, 0],
            [1, 1, 1],
            [2, 1, 0],
            [2, 1, 1],
            [4, 0, 0],
            [0, 0, 6],
        ]

        ws = self._make_synthetic_peaks_workspace(UB_true, hkls)

        alg = AlgorithmManager.create("FindUBFromConventionalCell")
        alg.initialize()
        alg.setProperty("PeaksWorkspace", ws)
        alg.setProperty("a", a)
        alg.setProperty("b", b)
        alg.setProperty("c", c)
        alg.setProperty("alpha", alpha_deg)
        alg.setProperty("beta", beta_deg)
        alg.setProperty("gamma", gamma_deg)
        alg.setProperty("Centering", "P")
        alg.setProperty("NumAzimuth", 240)
        alg.setProperty("NumPolar", 120)
        alg.setProperty("CapAngleDeg", 10.0)
        alg.setProperty("CapSamples", 1000)
        alg.setProperty("NumPsi", 720)
        alg.setProperty("RandomSeed", 1234)
        alg.execute()

        ws_out = alg.getProperty("PeaksWorkspace").value
        ol = ws_out.sample().getOrientedLattice()

        UB_est = ol.getUB().copy()

        q_vectors = np.array([UB_true @ np.array(hkl, dtype=float) for hkl in hkls])
        hkl_est = np.linalg.solve(UB_est, q_vectors.T).T
        err = hkl_est - np.rint(hkl_est)
        rms = np.sqrt(np.mean(err**2))

        self.assertLess(rms, 0.3)

    def test_accepts_primitive_rhombohedral_as_P(self):
        a = b = c = 5.5
        alpha_deg = beta_deg = gamma_deg = 55.0
        alpha = beta = gamma = np.deg2rad(55.0)

        B = reciprocal_basis_from_lattice(a, b, c, alpha, beta, gamma)
        U = rotation_matrix_from_axis_angle([1.0, -1.0, 1.0], np.deg2rad(42.0))
        UB_true = U @ B

        hkls = [
            [1, 0, 0],
            [0, 1, 0],
            [0, 0, 1],
            [1, 1, 0],
            [1, 0, 1],
            [0, 1, 1],
            [1, 1, 1],
            [2, 0, 0],
            [0, 2, 0],
            [0, 0, 2],
        ]

        ws = self._make_synthetic_peaks_workspace(UB_true, hkls)

        alg = AlgorithmManager.create("FindUBFromConventionalCell")
        alg.initialize()
        alg.setProperty("PeaksWorkspace", ws)
        alg.setProperty("a", a)
        alg.setProperty("b", b)
        alg.setProperty("c", c)
        alg.setProperty("alpha", alpha_deg)
        alg.setProperty("beta", beta_deg)
        alg.setProperty("gamma", gamma_deg)
        alg.setProperty("Centering", "P")
        alg.execute()

        ws_out = alg.getProperty("PeaksWorkspace").value
        ol = ws_out.sample().getOrientedLattice()

        UB_est = ol.getUB().copy()

        q_vectors = np.array([UB_true @ np.array(hkl, dtype=float) for hkl in hkls])
        hkl_est = np.linalg.solve(UB_est, q_vectors.T).T
        err = hkl_est - np.rint(hkl_est)
        rms = np.sqrt(np.mean(err**2))

        self.assertLess(rms, 0.3)

    def test_face_centered_cubic_with_few_peaks(self):
        a = b = c = 8
        alpha_deg = beta_deg = gamma_deg = 90
        alpha = beta = gamma = np.deg2rad(90.0)

        B = reciprocal_basis_from_lattice(a, b, c, alpha, beta, gamma)
        U = rotation_matrix_from_axis_angle([3.0, 1.0, 2.0], np.deg2rad(6.0))
        UB_true = U @ B

        hkls = [
            [2, 0, 0],
            [2, 2, 0],
            [1, 1, 1],
        ]

        ws = self._make_synthetic_peaks_workspace(UB_true, hkls, 0.05)

        alg = AlgorithmManager.create("FindUBFromConventionalCell")
        alg.initialize()
        alg.setProperty("PeaksWorkspace", ws)
        alg.setProperty("a", a)
        alg.setProperty("b", b)
        alg.setProperty("c", c)
        alg.setProperty("alpha", alpha_deg)
        alg.setProperty("beta", beta_deg)
        alg.setProperty("gamma", gamma_deg)
        alg.setProperty("Centering", "F")
        alg.execute()

        ws_out = alg.getProperty("PeaksWorkspace").value
        ol = ws_out.sample().getOrientedLattice()

        UB_est = ol.getUB().copy()

        q_vectors = np.array([UB_true @ np.array(hkl, dtype=float) for hkl in hkls])
        hkl_est = np.linalg.solve(UB_est, q_vectors.T).T
        err = hkl_est - np.rint(hkl_est)
        rms = np.sqrt(np.mean(err**2))

        self.assertLess(rms, 0.3)

    def test_base_centered_monoclinic_with_few_peaks(self):
        (
            a,
            b,
            c,
        ) = (
            6,
            10,
            6,
        )
        alpha_deg, beta_deg, gamma_deg = 90, 108, 90
        alpha, beta, gamma = np.deg2rad(90), np.deg2rad(108), np.deg2rad(90)

        B = reciprocal_basis_from_lattice(a, b, c, alpha, beta, gamma)
        U = rotation_matrix_from_axis_angle([-2.0, 1.0, 0.0], np.deg2rad(31.0))
        UB_true = U @ B

        hkls = [
            [2, 0, 1],
            [2, 2, 0],
            [1, 1, 1],
        ]

        ws = self._make_synthetic_peaks_workspace(UB_true, hkls, 0.05)

        alg = AlgorithmManager.create("FindUBFromConventionalCell")
        alg.initialize()
        alg.setProperty("PeaksWorkspace", ws)
        alg.setProperty("a", a)
        alg.setProperty("b", b)
        alg.setProperty("c", c)
        alg.setProperty("alpha", alpha_deg)
        alg.setProperty("beta", beta_deg)
        alg.setProperty("gamma", gamma_deg)
        alg.setProperty("Centering", "C")
        alg.execute()

        ws_out = alg.getProperty("PeaksWorkspace").value
        ol = ws_out.sample().getOrientedLattice()

        UB_est = ol.getUB().copy()

        q_vectors = np.array([UB_true @ np.array(hkl, dtype=float) for hkl in hkls])
        hkl_est = np.linalg.solve(UB_est, q_vectors.T).T
        err = hkl_est - np.rint(hkl_est)
        rms = np.sqrt(np.mean(err**2))

        self.assertLess(rms, 0.3)

    def test_a_centered_orthorhombic(self):
        a, b, c = 6.0, 7.0, 8.0
        alpha_deg = beta_deg = gamma_deg = 90.0
        alpha = np.deg2rad(alpha_deg)
        beta = np.deg2rad(beta_deg)
        gamma = np.deg2rad(gamma_deg)

        B = reciprocal_basis_from_lattice(a, b, c, alpha, beta, gamma)
        U = rotation_matrix_from_axis_angle([1.0, 2.0, -3.0], np.deg2rad(17.0))
        UB_true = U @ B

        # A centering requires k + l to be even.
        hkls = [
            [1, 0, 2],
            [0, 1, 1],
            [2, 1, 1],
            [1, 2, 0],
            [3, 0, 2],
            [2, 2, 2],
            [0, 2, 0],
            [2, 0, 0],
        ]

        ws = self._make_synthetic_peaks_workspace(UB_true, hkls)

        alg = AlgorithmManager.create("FindUBFromConventionalCell")
        alg.initialize()
        alg.setProperty("PeaksWorkspace", ws)
        alg.setProperty("a", a)
        alg.setProperty("b", b)
        alg.setProperty("c", c)
        alg.setProperty("alpha", alpha_deg)
        alg.setProperty("beta", beta_deg)
        alg.setProperty("gamma", gamma_deg)
        alg.setProperty("Centering", "A")
        alg.setProperty("NumAzimuth", 240)
        alg.setProperty("NumPolar", 120)
        alg.setProperty("CapAngleDeg", 10.0)
        alg.setProperty("CapSamples", 1000)
        alg.setProperty("NumPsi", 720)
        alg.setProperty("RandomSeed", 1234)
        alg.execute()

        self._assert_indexing_and_centering(alg, UB_true, hkls)

    def test_b_centered_orthorhombic(self):
        a, b, c = 6.0, 7.0, 8.0
        alpha_deg = beta_deg = gamma_deg = 90.0
        alpha = np.deg2rad(alpha_deg)
        beta = np.deg2rad(beta_deg)
        gamma = np.deg2rad(gamma_deg)

        B = reciprocal_basis_from_lattice(a, b, c, alpha, beta, gamma)
        U = rotation_matrix_from_axis_angle([1.0, 2.0, -3.0], np.deg2rad(17.0))
        UB_true = U @ B

        # B centering requires h + l to be even.
        hkls = [
            [1, 0, 1],
            [0, 1, 0],
            [1, 1, 1],
            [2, 1, 0],
            [1, 2, 1],
            [0, 2, 0],
            [2, 0, 2],
            [3, 1, 1],
        ]

        ws = self._make_synthetic_peaks_workspace(UB_true, hkls)

        alg = AlgorithmManager.create("FindUBFromConventionalCell")
        alg.initialize()
        alg.setProperty("PeaksWorkspace", ws)
        alg.setProperty("a", a)
        alg.setProperty("b", b)
        alg.setProperty("c", c)
        alg.setProperty("alpha", alpha_deg)
        alg.setProperty("beta", beta_deg)
        alg.setProperty("gamma", gamma_deg)
        alg.setProperty("Centering", "B")
        alg.setProperty("NumAzimuth", 240)
        alg.setProperty("NumPolar", 120)
        alg.setProperty("CapAngleDeg", 10.0)
        alg.setProperty("CapSamples", 1000)
        alg.setProperty("NumPsi", 720)
        alg.setProperty("RandomSeed", 1234)
        alg.execute()

        self._assert_indexing_and_centering(alg, UB_true, hkls)

    def test_r_centered_conventional_hexagonal(self):
        a = b = 6.0
        c = 10.0
        alpha_deg = beta_deg = 90.0
        gamma_deg = 120.0
        alpha = np.deg2rad(alpha_deg)
        beta = np.deg2rad(beta_deg)
        gamma = np.deg2rad(gamma_deg)

        B = reciprocal_basis_from_lattice(a, b, c, alpha, beta, gamma)
        U = rotation_matrix_from_axis_angle([1.0, 2.0, -3.0], np.deg2rad(17.0))
        UB_true = U @ B

        # R centering in the conventional hexagonal setting requires
        # -h + k + l to be divisible by 3.
        hkls = [
            [1, 0, 1],
            [1, 1, 0],
            [0, 1, 2],
            [2, 1, 1],
            [0, 2, 1],
            [2, 0, 2],
            [1, 2, 2],
            [2, 2, 0],
        ]

        ws = self._make_synthetic_peaks_workspace(UB_true, hkls)

        alg = AlgorithmManager.create("FindUBFromConventionalCell")
        alg.initialize()
        alg.setProperty("PeaksWorkspace", ws)
        alg.setProperty("a", a)
        alg.setProperty("b", b)
        alg.setProperty("c", c)
        alg.setProperty("alpha", alpha_deg)
        alg.setProperty("beta", beta_deg)
        alg.setProperty("gamma", gamma_deg)
        alg.setProperty("Centering", "R")
        alg.setProperty("NumAzimuth", 240)
        alg.setProperty("NumPolar", 120)
        alg.setProperty("CapAngleDeg", 10.0)
        alg.setProperty("CapSamples", 1000)
        alg.setProperty("NumPsi", 720)
        alg.setProperty("RandomSeed", 1234)
        alg.execute()

        self._assert_indexing_and_centering(alg, UB_true, hkls)

    def test_body_centered_tetragonal(self):
        a, b, c = 6.0, 6.0, 8.0
        alpha_deg = beta_deg = gamma_deg = 90.0
        alpha = np.deg2rad(alpha_deg)
        beta = np.deg2rad(beta_deg)
        gamma = np.deg2rad(gamma_deg)

        B = reciprocal_basis_from_lattice(a, b, c, alpha, beta, gamma)
        U = rotation_matrix_from_axis_angle([1.0, -2.0, 3.0], np.deg2rad(11.0))
        UB_true = U @ B

        hkls = [
            [2, 2, 0],
            [4, 2, 0],
            [2, 4, 2],
            [1, 3, 1],
            [1, 0, 1],
            [2, 0, 2],
        ]

        ws = self._make_synthetic_peaks_workspace(UB_true, hkls)

        alg = AlgorithmManager.create("FindUBFromConventionalCell")
        alg.initialize()
        alg.setProperty("PeaksWorkspace", ws)
        alg.setProperty("a", a)
        alg.setProperty("b", b)
        alg.setProperty("c", c)
        alg.setProperty("alpha", alpha_deg)
        alg.setProperty("beta", beta_deg)
        alg.setProperty("gamma", gamma_deg)
        alg.setProperty("Centering", "I")
        alg.setProperty("NumAzimuth", 240)
        alg.setProperty("NumPolar", 120)
        alg.setProperty("CapAngleDeg", 10.0)
        alg.setProperty("CapSamples", 1000)
        alg.setProperty("NumPsi", 720)
        alg.setProperty("RandomSeed", 1234)
        alg.execute()

        ws_out = alg.getProperty("PeaksWorkspace").value
        ol = ws_out.sample().getOrientedLattice()

        UB_est = ol.getUB().copy()

        q_vectors = np.array([UB_true @ np.array(hkl, dtype=float) for hkl in hkls])
        hkl_est = np.linalg.solve(UB_est, q_vectors.T).T
        err = hkl_est - np.rint(hkl_est)
        rms = np.sqrt(np.mean(err**2))

        self.assertLess(rms, 0.3)

    def test_face_centered_orthorhombic(self):
        a, b, c = 6.0, 6.1, 8.0
        alpha_deg = beta_deg = gamma_deg = 90.0
        alpha = np.deg2rad(alpha_deg)
        beta = np.deg2rad(beta_deg)
        gamma = np.deg2rad(gamma_deg)

        B = reciprocal_basis_from_lattice(a, b, c, alpha, beta, gamma)
        U = rotation_matrix_from_axis_angle([1.0, 2.0, -3.0], np.deg2rad(6.0))
        UB_true = U @ B

        hkls = [
            [1, 1, 1],
            [4, 2, 4],
            [2, 4, 2],
            [1, 3, 1],
            [3, 1, 1],
            [4, 2, 2],
        ]

        ws = self._make_synthetic_peaks_workspace(UB_true, hkls)

        alg = AlgorithmManager.create("FindUBFromConventionalCell")
        alg.initialize()
        alg.setProperty("PeaksWorkspace", ws)
        alg.setProperty("a", a)
        alg.setProperty("b", b)
        alg.setProperty("c", c)
        alg.setProperty("alpha", alpha_deg)
        alg.setProperty("beta", beta_deg)
        alg.setProperty("gamma", gamma_deg)
        alg.setProperty("Centering", "F")
        alg.setProperty("NumAzimuth", 240)
        alg.setProperty("NumPolar", 120)
        alg.setProperty("CapAngleDeg", 10.0)
        alg.setProperty("CapSamples", 1000)
        alg.setProperty("NumPsi", 720)
        alg.setProperty("RandomSeed", 1234)
        alg.execute()

        ws_out = alg.getProperty("PeaksWorkspace").value
        ol = ws_out.sample().getOrientedLattice()

        UB_est = ol.getUB().copy()

        q_vectors = np.array([UB_true @ np.array(hkl, dtype=float) for hkl in hkls])
        hkl_est = np.linalg.solve(UB_est, q_vectors.T).T
        err = hkl_est - np.rint(hkl_est)
        rms = np.sqrt(np.mean(err**2))

        self.assertLess(rms, 0.3)


class CenteringTransformTest(unittest.TestCase):
    """
    Unit tests for `centering_transform_to_primitive` as a pure function.

    The algorithm encodes each centering twice: once as a direct-basis transform and once as a
    reflection condition. The two are used at opposite ends of the calculation and nothing else ties
    them together, so these tests assert that they describe the same lattice.
    """

    CENTERINGS = ["P", "A", "B", "C", "I", "F", "R"]

    # Number of lattice points per conventional cell, i.e. 1 / det(T_cp).
    MULTIPLICITY = {"P": 1, "A": 2, "B": 2, "C": 2, "I": 2, "F": 4, "R": 3}

    @staticmethod
    def _all_hkl(index_max):
        """Every integer (h, k, l) with each index in [-index_max, index_max], excluding (0, 0, 0)."""
        span = np.arange(-index_max, index_max + 1)
        grid = np.stack(np.meshgrid(span, span, span, indexing="ij"), axis=-1).reshape(-1, 3)
        return grid[np.any(grid != 0, axis=1)]

    def test_transform_agrees_with_reflection_condition(self):
        """
        A reflection exists on the primitive cell exactly when its primitive indices are integers.

        The transform and the reflection condition must therefore agree on every (h, k, l): allowed
        reflections must reindex to integers, and forbidden ones must not. The second direction is
        what distinguishes the obverse R setting from any other mod-3 rule.
        """
        hkl_conv = self._all_hkl(4)
        h, k, l = hkl_conv[:, 0], hkl_conv[:, 1], hkl_conv[:, 2]

        for centering in self.CENTERINGS:
            with self.subTest(centering=centering):
                T_cp = find_ub_module.centering_transform_to_primitive(centering)

                hkl_prim = hkl_conv @ T_cp
                is_integral = np.all(np.abs(hkl_prim - np.rint(hkl_prim)) < 1e-9, axis=1)
                allowed = find_ub_module.centering_mask(h, k, l, centering)

                np.testing.assert_array_equal(is_integral, allowed)

    def test_transform_has_expected_multiplicity(self):
        """The conventional cell holds `MULTIPLICITY` primitive cells, so det(T_cp) = 1 / that."""
        for centering in self.CENTERINGS:
            with self.subTest(centering=centering):
                T_cp = find_ub_module.centering_transform_to_primitive(centering)
                self.assertAlmostEqual(abs(np.linalg.det(T_cp)), 1.0 / self.MULTIPLICITY[centering])

    def test_r_transform_gives_rhombohedral_primitive_cell(self):
        """
        The primitive cell of an R lattice is rhombohedral, by symmetry.

        This is a stronger check than index integrality: it fails for any transform whose columns
        are not genuine R lattice vectors, whatever mod-3 rule those columns happen to satisfy.
        """
        a = 6.0
        c = 10.0

        A_conv = find_ub_module.direct_basis_from_lattice(a, a, c, np.pi / 2.0, np.pi / 2.0, np.deg2rad(120.0))
        T_cp = find_ub_module.centering_transform_to_primitive("R")
        a_p, b_p, c_p, alpha_p, beta_p, gamma_p = find_ub_module.lattice_from_direct_basis(A_conv @ T_cp)

        self.assertAlmostEqual(a_p, b_p)
        self.assertAlmostEqual(b_p, c_p)
        self.assertAlmostEqual(alpha_p, beta_p)
        self.assertAlmostEqual(beta_p, gamma_p)

        # Closed forms relating the hexagonal and rhombohedral descriptions of an R lattice.
        a_expected = np.sqrt(a**2 / 3.0 + c**2 / 9.0)
        alpha_expected = 2.0 * np.arcsin(3.0 * a / (2.0 * np.sqrt(3.0 * a**2 + c**2)))

        self.assertAlmostEqual(a_p, a_expected)
        self.assertAlmostEqual(alpha_p, alpha_expected)


if __name__ == "__main__":
    unittest.main()
