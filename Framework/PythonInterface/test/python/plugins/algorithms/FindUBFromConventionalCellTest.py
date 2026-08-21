# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# ruff: noqa: E741  # Ambiguous variable name (h, k, l are Miller indices)
import os
import unittest
import numpy as np

from mantid.simpleapi import CreatePeaksWorkspace, Load, SetUB
from mantid.kernel import V3D
from mantid.api import AlgorithmManager

# The real-data test file is not yet contributed to Mantid's ExternalData.
# Until it is, the test below is guarded with `@skipUnless` and looks for the
# file locally at `<this dir>/data/macro.nxs`. Once the file is added to
# ExternalData (Testing/Data/UnitTest/macro.nxs.md5), this path handling and
# the skip guard can be replaced with a bare-filename Load.
MACRO_NXS_PATH = os.path.join(os.path.dirname(__file__), "data", "macro.nxs")


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


@unittest.skipUnless(
    os.path.exists(MACRO_NXS_PATH),
    "tests/data/macro.nxs not available locally",
)
class FindUBFromConventionalCellRealDataTest(unittest.TestCase):
    def test_macromolecular_orthorhombic_p(self):
        ws = Load(Filename=MACRO_NXS_PATH, OutputWorkspace="macro")

        alg = AlgorithmManager.create("FindUBFromConventionalCell")
        alg.initialize()
        alg.setProperty("PeaksWorkspace", ws)
        alg.setProperty("a", 85.2)
        alg.setProperty("b", 89.6)
        alg.setProperty("c", 110.9)
        alg.setProperty("alpha", 90.0)
        alg.setProperty("beta", 90.0)
        alg.setProperty("gamma", 90.0)
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

        self.assertAlmostEqual(ol.a(), 85.2, delta=0.5)
        self.assertAlmostEqual(ol.b(), 89.6, delta=0.5)
        self.assertAlmostEqual(ol.c(), 110.9, delta=0.5)
        self.assertAlmostEqual(ol.alpha(), 90.0, delta=1.0)
        self.assertAlmostEqual(ol.beta(), 90.0, delta=1.0)
        self.assertAlmostEqual(ol.gamma(), 90.0, delta=1.0)

        UB_est = ol.getUB().copy()
        q_vectors = np.array(
            [
                [
                    ws_out.getPeak(i).getQSampleFrame().X(),
                    ws_out.getPeak(i).getQSampleFrame().Y(),
                    ws_out.getPeak(i).getQSampleFrame().Z(),
                ]
                for i in range(ws_out.getNumberPeaks())
            ]
        )
        hkl_est = np.linalg.solve(UB_est, q_vectors.T).T / (2.0 * np.pi)
        err = np.abs(hkl_est - np.rint(hkl_est))
        fail_frac = np.mean(np.any(err > 0.3, axis=1))

        self.assertLess(fail_frac, 0.2)


if __name__ == "__main__":
    unittest.main()
