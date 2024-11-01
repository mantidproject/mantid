# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from numpy import array, nan, where, vstack
from numpy.testing import assert_almost_equal, assert_array_equal
from mantid.utils.dgs import qangle, ErrorCodes
from mantid.geometry import OrientedLattice


class DgsUtilsTest(unittest.TestCase):
    # The original tests 1-9 were taken from known good values which were hand calculated in Excel by Barry Winn
    # ConvertToMD can also be used to generate similar data in Mantid. Take a run at a given angle, find the peak
    # Use the H, K, L values as input to check if the angles are the same as you see with instrument viewer

    def run_test(
        self,
        ol,
        expected_Qx,
        expected_Qy,
        expected_Qz,
        expected_in_plane_Q_angle,
        expected_out_plane_Q_angle,
        expected_in_plane_kf_angle,
        expected_out_plane_kf_angle,
        expected_omega,
        expected_error_code,
        **pars,
    ):
        result = qangle(lattice=ol, **pars)
        good_results = where(result.error_code == ErrorCodes.CORRECT)
        assert_almost_equal(expected_Qx[good_results], result.Q_lab_x[good_results], 2)
        assert_almost_equal(expected_Qy[good_results], result.Q_lab_y[good_results], 2)
        assert_almost_equal(expected_Qz[good_results], result.Q_lab_z[good_results], 2)
        assert_almost_equal(expected_in_plane_Q_angle[good_results], result.in_plane_Q_angle[good_results], 1)
        assert_almost_equal(expected_out_plane_Q_angle[good_results], result.out_plane_Q_angle[good_results], 1)
        assert_almost_equal(expected_in_plane_kf_angle[good_results], result.in_plane_kf_angle[good_results], 1)
        assert_almost_equal(expected_out_plane_kf_angle[good_results], result.out_plane_kf_angle[good_results], 1)
        assert_almost_equal(expected_omega[good_results], result.omega[good_results], 1)
        assert_array_equal(expected_error_code, result.error_code)

    def test_qangle_NoGC_NoOC(self):
        # No geometry constraints, no omega constraints, all parameters are arrays
        test_case1 = array(
            [
                [10, 0, 1, 0, 0, 1, -0.72, 0, 0.12, 19, 0, -80.5, 0, -80.5, 0],
                [10, 0, 0, 1, 0, 1, -0.72, 0, 0.12, 19, 0, -80.5, 0, -140.5, 0],
                [10, 0, -1, 0, 0, 1, -0.72, 0, 0.12, 19, 0, -80.5, 0, 99.5, 0],
                [10, 0, 0, -1, 0, 1, -0.72, 0, 0.12, 19, 0, -80.5, 0, 39.5, 0],
                [10, 0, 1, 1, 0, 1, -1.2, 0, 0.36, 33.2, 0, -73.4, 0, -103.4, 0],
                [10, 0, 0.2, 0, 0, 1, -0.14, 0, 0, 3.8, 0, -88.1, 0, -88.1, 0],
                [10, 0, 0.02, 0, 0, 1, -0.015, 0, 5e-05, 0.4, 0, -89.8, 0, -89.8, 0],
                [10, 5, 1, 0, 0, 1, -0.28, 0, 0.67, 10.4, 0, -22.8, 0, -22.7, 0],
                [10, -5, 1, 0, 0, 1, -0.59, 0, -0.43, 12.6, 0, -126.3, 0, -126.3, 0],
                [10, 0, 5, 0, 0, 1, -2.05, 0, 3, 111.3, 0, -34.4, 0, -34.3, 0],
                [10, 0, 1, -0.01, -0.18, 1, -0.71, -0.11, 0.12, 19, 3, -80.3, -9, -79.7, 0],
                [10, 0, 1, 0, -0.6, 1, -0.72, -0.38, 0.15, 19.13, 10, -77.8, -27.5, -77.9, 0],
                [10, 0, 1, 0, 0, -1, 0.72, 0, 0.12, -19, 0, 80.5, 0, 80.5, 0],
                [10, 0, 0, 1, 0, -1, 0.72, 0, 0.12, -19, 0, 80.5, 0, 20.5, 0],
                [10, 0, -1, 0, 0, -1, 0.72, 0, 0.12, -19, 0, 80.5, 0, -99.5, 0],
                [10, 0, 0, -1, 0, -1, 0.72, 0, 0.12, -19, 0, 80.5, 0, -159.5, 0],
                [10, 0, 1, 1, 0, -1, 1.2, 0, 0.36, -33.2, 0, 73.4, 0, 43.4, 0],
                [10, 0, 0.2, 0, 0, -1, 0.14, 0, 0, -3.8, 0, 88.1, 0, 88.1, 0],
                [10, 0, 0.02, 0, 0, -1, 0.015, 0, 5e-05, -0.4, 0, 89.8, 0, 89.8, 0],
                [10, 5, 1, 0, 0, -1, 0.28, 0, 0.67, -10.4, 0, 22.8, 0, 22.8, 0],
                [10, -5, 1, 0, 0, -1, 0.59, 0, -0.43, -12.6, 0, 126.3, 0, 126.3, 0],
                [10, 0, 5, 0, 0, -1, 2.05, 0, 3, -111.3, 0, 34.4, 0, 34.3, 0],
                [20, 0, 1, 0, 0, 1, -0.72, 0, 0.08, 13.4, 0, -83.3, 0, -83.3, 0],
                [10, 0, 8, 0, 0, 1, nan, nan, nan, nan, nan, nan, nan, nan, 2],
                [10, 20, 1, 0, 0, 1, nan, nan, nan, nan, nan, nan, nan, nan, 2],
                [-10, 0, 1, 0, 0, 1, nan, nan, nan, nan, nan, nan, nan, nan, 1],
                [10, 0, 1, 0, nan, 1, nan, nan, nan, nan, nan, nan, nan, nan, 1],
                [10, 0, -1.3, 3.4, -3.5, -1, nan, nan, nan, nan, nan, nan, nan, nan, 3],
                [10, 0, -1.3, 3.4, 3.5, -1, nan, nan, nan, nan, nan, nan, nan, nan, 3],
            ]
        )

        a = 10
        b = 10
        c = 10
        alpha = 90
        beta = 90
        gamma = 120
        u = [1, 0, 0]
        v = [0, 1, 0]
        ol = OrientedLattice(a, b, c, alpha, beta, gamma)
        ol.setUFromVectors(u, v)

        (
            Ei,
            DE,
            H,
            K,
            L,
            sign,
            expected_Qx,
            expected_Qy,
            expected_Qz,
            expected_in_plane_kf_angle,
            expected_out_plane_kf_angle,
            expected_in_plane_Q_angle,
            expected_out_plane_Q_angle,
            expected_omega,
            expected_error_code,
        ) = test_case1.T

        HKL = vstack([H, K, L]).T
        pars = {"Ei": Ei, "hkl": HKL, "DeltaE": DE, "sign": sign}
        expected_error_code = [ErrorCodes(ec) for ec in expected_error_code]
        self.run_test(
            ol,
            expected_Qx,
            expected_Qy,
            expected_Qz,
            expected_in_plane_Q_angle,
            expected_out_plane_Q_angle,
            expected_in_plane_kf_angle,
            expected_out_plane_kf_angle,
            expected_omega,
            expected_error_code,
            **pars,
        )

    def test_qangle_NoGC_NoOC_Ei10(self):
        # No geometry constraints, no omega constraints, all parameters are arrays,
        # except for Ei=10
        test_case2 = array(
            [
                [0, 0, 1, 0, 1, -0.72, 0, 0.12, 19, 0, -80.5, 0, -140.5, 0],
                [0, -1, 0, 0, 1, -0.72, 0, 0.12, 19, 0, -80.5, 0, 99.5, 0],
                [0, 0, -1, 0, 1, -0.72, 0, 0.12, 19, 0, -80.5, 0, 39.5, 0],
                [0, 1, 1, 0, 1, -1.2, 0, 0.36, 33.2, 0, -73.4, 0, -103.4, 0],
                [0, 0.2, 0, 0, 1, -0.14, 0, 0, 3.8, 0, -88.1, 0, -88.1, 0],
                [0, 0.02, 0, 0, 1, -0.015, 0, 5e-05, 0.4, 0, -89.8, 0, -89.8, 0],
                [5, 1, 0, 0, 1, -0.28, 0, 0.67, 10.4, 0, -22.8, 0, -22.7, 0],
                [-5, 1, 0, 0, 1, -0.59, 0, -0.43, 12.6, 0, -126.3, 0, -126.3, 0],
                [0, 5, 0, 0, 1, -2.05, 0, 3, 111.3, 0, -34.4, 0, -34.3, 0],
                [0, 1, -0.01, -0.18, 1, -0.71, -0.11, 0.12, 19, 3, -80.3, -9, -79.7, 0],
                [0, 1, 0, -0.6, 1, -0.72, -0.38, 0.15, 19.13, 10, -77.8, -27.5, -77.9, 0],
                [0, 1, 0, 0, -1, 0.72, 0, 0.12, -19, 0, 80.5, 0, 80.5, 0],
                [0, 0, 1, 0, -1, 0.72, 0, 0.12, -19, 0, 80.5, 0, 20.5, 0],
                [0, -1, 0, 0, -1, 0.72, 0, 0.12, -19, 0, 80.5, 0, -99.5, 0],
                [0, 0, -1, 0, -1, 0.72, 0, 0.12, -19, 0, 80.5, 0, -159.5, 0],
                [0, 1, 1, 0, -1, 1.2, 0, 0.36, -33.2, 0, 73.4, 0, 43.4, 0],
                [0, 0.2, 0, 0, -1, 0.14, 0, 0, -3.8, 0, 88.1, 0, 88.1, 0],
                [0, 0.02, 0, 0, -1, 0.015, 0, 5e-05, -0.4, 0, 89.8, 0, 89.8, 0],
                [5, 1, 0, 0, -1, 0.28, 0, 0.67, -10.4, 0, 22.8, 0, 22.8, 0],
                [-5, 1, 0, 0, -1, 0.59, 0, -0.43, -12.6, 0, 126.3, 0, 126.3, 0],
                [0, 5, 0, 0, -1, 2.05, 0, 3, -111.3, 0, 34.4, 0, 34.3, 0],
                [0, 8, 0, 0, 1, nan, nan, nan, nan, nan, nan, nan, nan, 2],
                [20, 1, 0, 0, 1, nan, nan, nan, nan, nan, nan, nan, nan, 2],
            ]
        )
        a = 10
        b = 10
        c = 10
        alpha = 90
        beta = 90
        gamma = 120
        u = [1, 0, 0]
        v = [0, 1, 0]
        ol = OrientedLattice(a, b, c, alpha, beta, gamma)
        ol.setUFromVectors(u, v)

        Ei = 10.0
        (
            DE,
            H,
            K,
            L,
            sign,
            expected_Qx,
            expected_Qy,
            expected_Qz,
            expected_in_plane_kf_angle,
            expected_out_plane_kf_angle,
            expected_in_plane_Q_angle,
            expected_out_plane_Q_angle,
            expected_omega,
            expected_error_code,
        ) = test_case2.T

        HKL = vstack([H, K, L]).T
        pars = {"Ei": Ei, "hkl": HKL, "DeltaE": DE, "sign": sign}
        expected_error_code = [ErrorCodes(ec) for ec in expected_error_code]
        self.run_test(
            ol,
            expected_Qx,
            expected_Qy,
            expected_Qz,
            expected_in_plane_Q_angle,
            expected_out_plane_Q_angle,
            expected_in_plane_kf_angle,
            expected_out_plane_kf_angle,
            expected_omega,
            expected_error_code,
            **pars,
        )

    def test_qangle_NoGC_NoOC_DE0(self):
        # No geometry constraints, no omega constraints, all parameters are arrays,
        # except for DeltaE=0
        test_case3 = array(
            [
                [10, 1, 0, 0, 1, -0.72, 0, 0.12, 19, 0, -80.5, 0, -80.5, 0],
                [10, 0, 1, 0, 1, -0.72, 0, 0.12, 19, 0, -80.5, 0, -140.5, 0],
                [10, -1, 0, 0, 1, -0.72, 0, 0.12, 19, 0, -80.5, 0, 99.5, 0],
                [10, 0, -1, 0, 1, -0.72, 0, 0.12, 19, 0, -80.5, 0, 39.5, 0],
                [10, 1, 1, 0, 1, -1.2, 0, 0.36, 33.2, 0, -73.4, 0, -103.4, 0],
                [10, 0.2, 0, 0, 1, -0.14, 0, 0, 3.8, 0, -88.1, 0, -88.1, 0],
                [10, 0.02, 0, 0, 1, -0.015, 0, 5e-05, 0.4, 0, -89.8, 0, -89.8, 0],
                [10, 5, 0, 0, 1, -2.05, 0, 3, 111.3, 0, -34.4, 0, -34.3, 0],
                [10, 1, -0.01, -0.18, 1, -0.71, -0.11, 0.12, 19, 3, -80.3, -9, -79.7, 0],
                [10, 1, 0, -0.6, 1, -0.72, -0.38, 0.15, 19.13, 10, -77.8, -27.5, -77.9, 0],
                [10, 1, 0, 0, -1, 0.72, 0, 0.12, -19, 0, 80.5, 0, 80.5, 0],
                [10, 0, 1, 0, -1, 0.72, 0, 0.12, -19, 0, 80.5, 0, 20.5, 0],
                [10, -1, 0, 0, -1, 0.72, 0, 0.12, -19, 0, 80.5, 0, -99.5, 0],
                [10, 0, -1, 0, -1, 0.72, 0, 0.12, -19, 0, 80.5, 0, -159.5, 0],
                [10, 1, 1, 0, -1, 1.2, 0, 0.36, -33.2, 0, 73.4, 0, 43.4, 0],
                [10, 0.2, 0, 0, -1, 0.14, 0, 0, -3.8, 0, 88.1, 0, 88.1, 0],
                [10, 0.02, 0, 0, -1, 0.015, 0, 5e-05, -0.4, 0, 89.8, 0, 89.8, 0],
                [10, 5, 0, 0, -1, 2.05, 0, 3, -111.3, 0, 34.4, 0, 34.3, 0],
                [20, 1, 0, 0, 1, -0.72, 0, 0.08, 13.4, 0, -83.3, 0, -83.3, 0],
                [10, 8, 0, 0, 1, nan, nan, nan, nan, nan, nan, nan, nan, 2],
            ]
        )
        a = 10
        b = 10
        c = 10
        alpha = 90
        beta = 90
        gamma = 120
        u = [1, 0, 0]
        v = [0, 1, 0]
        ol = OrientedLattice(a, b, c, alpha, beta, gamma)
        ol.setUFromVectors(u, v)

        DE = 0.0
        (
            Ei,
            H,
            K,
            L,
            sign,
            expected_Qx,
            expected_Qy,
            expected_Qz,
            expected_in_plane_kf_angle,
            expected_out_plane_kf_angle,
            expected_in_plane_Q_angle,
            expected_out_plane_Q_angle,
            expected_omega,
            expected_error_code,
        ) = test_case3.T

        HKL = vstack([H, K, L]).T
        pars = {"Ei": Ei, "hkl": HKL, "DeltaE": DE, "sign": sign}
        expected_error_code = [ErrorCodes(ec) for ec in expected_error_code]
        self.run_test(
            ol,
            expected_Qx,
            expected_Qy,
            expected_Qz,
            expected_in_plane_Q_angle,
            expected_out_plane_Q_angle,
            expected_in_plane_kf_angle,
            expected_out_plane_kf_angle,
            expected_omega,
            expected_error_code,
            **pars,
        )

    def test_qangle_NoGC_NoOC_left(self):
        # No geometry constraints, no omega constraints, all parameters are arrays,
        # except for sign=+1
        test_case4 = array(
            [
                [10, 0, 1, 0, 0, -0.72, 0, 0.12, 19, 0, -80.5, 0, -80.5, 0],
                [10, 0, 0, 1, 0, -0.72, 0, 0.12, 19, 0, -80.5, 0, -140.5, 0],
                [10, 0, -1, 0, 0, -0.72, 0, 0.12, 19, 0, -80.5, 0, 99.5, 0],
                [10, 0, 0, -1, 0, -0.72, 0, 0.12, 19, 0, -80.5, 0, 39.5, 0],
                [10, 0, 1, 1, 0, -1.2, 0, 0.36, 33.2, 0, -73.4, 0, -103.4, 0],
                [10, 0, 0.2, 0, 0, -0.14, 0, 0, 3.8, 0, -88.1, 0, -88.1, 0],
                [10, 0, 0.02, 0, 0, -0.015, 0, 5e-05, 0.4, 0, -89.8, 0, -89.8, 0],
                [10, 5, 1, 0, 0, -0.28, 0, 0.67, 10.4, 0, -22.8, 0, -22.7, 0],
                [10, -5, 1, 0, 0, -0.59, 0, -0.43, 12.6, 0, -126.3, 0, -126.3, 0],
                [10, 0, 5, 0, 0, -2.05, 0, 3, 111.3, 0, -34.4, 0, -34.3, 0],
                [10, 0, 1, -0.01, -0.18, -0.71, -0.11, 0.12, 19, 3, -80.3, -9, -79.7, 0],
                [10, 0, 1, 0, -0.6, -0.72, -0.38, 0.15, 19.13, 10, -77.8, -27.5, -77.9, 0],
                [20, 0, 1, 0, 0, -0.72, 0, 0.08, 13.4, 0, -83.3, 0, -83.3, 0],
                [10, 0, 8, 0, 0, nan, nan, nan, nan, nan, nan, nan, nan, 2],
                [10, 20, 1, 0, 0, nan, nan, nan, nan, nan, nan, nan, nan, 2],
            ]
        )

        a = 10
        b = 10
        c = 10
        alpha = 90
        beta = 90
        gamma = 120
        u = [1, 0, 0]
        v = [0, 1, 0]
        ol = OrientedLattice(a, b, c, alpha, beta, gamma)
        ol.setUFromVectors(u, v)

        sign = +1
        (
            Ei,
            DE,
            H,
            K,
            L,
            expected_Qx,
            expected_Qy,
            expected_Qz,
            expected_in_plane_kf_angle,
            expected_out_plane_kf_angle,
            expected_in_plane_Q_angle,
            expected_out_plane_Q_angle,
            expected_omega,
            expected_error_code,
        ) = test_case4.T

        HKL = vstack([H, K, L]).T
        pars = {"Ei": Ei, "hkl": HKL, "DeltaE": DE, "sign": sign}
        expected_error_code = [ErrorCodes(ec) for ec in expected_error_code]
        self.run_test(
            ol,
            expected_Qx,
            expected_Qy,
            expected_Qz,
            expected_in_plane_Q_angle,
            expected_out_plane_Q_angle,
            expected_in_plane_kf_angle,
            expected_out_plane_kf_angle,
            expected_omega,
            expected_error_code,
            **pars,
        )

    def test_qangle_NoGC_NoOC_right(self):
        # No geometry constraints, no omega constraints, all parameters are arrays,
        # except for sign=-1
        test_case5 = array(
            [
                [10, 0, 1, 0, 0, 0.72, 0, 0.12, -19, 0, 80.5, 0, 80.5, 0],
                [10, 0, 0, 1, 0, 0.72, 0, 0.12, -19, 0, 80.5, 0, 20.5, 0],
                [10, 0, -1, 0, 0, 0.72, 0, 0.12, -19, 0, 80.5, 0, -99.5, 0],
                [10, 0, 0, -1, 0, 0.72, 0, 0.12, -19, 0, 80.5, 0, -159.5, 0],
                [10, 0, 1, 1, 0, 1.2, 0, 0.36, -33.2, 0, 73.4, 0, 43.4, 0],
                [10, 0, 0.2, 0, 0, 0.14, 0, 0, -3.8, 0, 88.1, 0, 88.1, 0],
                [10, 0, 0.02, 0, 0, 0.015, 0, 5e-05, -0.4, 0, 89.8, 0, 89.8, 0],
                [10, 5, 1, 0, 0, 0.28, 0, 0.67, -10.4, 0, 22.8, 0, 22.8, 0],
                [10, -5, 1, 0, 0, 0.59, 0, -0.43, -12.6, 0, 126.3, 0, 126.3, 0],
                [10, 0, 5, 0, 0, 2.05, 0, 3, -111.3, 0, 34.4, 0, 34.3, 0],
            ]
        )

        a = 10
        b = 10
        c = 10
        alpha = 90
        beta = 90
        gamma = 120
        u = [1, 0, 0]
        v = [0, 1, 0]
        ol = OrientedLattice(a, b, c, alpha, beta, gamma)
        ol.setUFromVectors(u, v)

        sign = -1
        (
            Ei,
            DE,
            H,
            K,
            L,
            expected_Qx,
            expected_Qy,
            expected_Qz,
            expected_in_plane_kf_angle,
            expected_out_plane_kf_angle,
            expected_in_plane_Q_angle,
            expected_out_plane_Q_angle,
            expected_omega,
            expected_error_code,
        ) = test_case5.T

        HKL = vstack([H, K, L]).T
        pars = {"Ei": Ei, "hkl": HKL, "DeltaE": DE, "sign": sign}
        expected_error_code = [ErrorCodes(ec) for ec in expected_error_code]
        self.run_test(
            ol,
            expected_Qx,
            expected_Qy,
            expected_Qz,
            expected_in_plane_Q_angle,
            expected_out_plane_Q_angle,
            expected_in_plane_kf_angle,
            expected_out_plane_kf_angle,
            expected_omega,
            expected_error_code,
            **pars,
        )

    def test_qangle_GC_NoOC(self):
        # Geometry constraints, no omega constraints, all parameters are arrays
        test_case6 = array(
            [
                [10, 0, 1, 0, 0, 1, -0.72, 0, 0.12, 19, 0, -80.5, 0, -80.5, 0],
                [10, 0, 0, 1, 0, 1, -0.72, 0, 0.12, 19, 0, -80.5, 0, -140.5, 0],
                [10, 0, -1, 0, 0, 1, -0.72, 0, 0.12, 19, 0, -80.5, 0, 99.5, 0],
                [10, 0, 0, -1, 0, 1, -0.72, 0, 0.12, 19, 0, -80.5, 0, 39.5, 0],
                [10, 0, 1, 1, 0, 1, -1.2, 0, 0.36, 33.2, 0, -73.4, 0, -103.4, 0],
                [10, 0, 0.2, 0, 0, 1, -0.14, 0, 0, 3.8, 0, -88.1, 0, -88.1, 4],
                [10, 0, 0.02, 0, 0, 1, nan, nan, nan, nan, nan, nan, nan, nan, 4],
                [10, 5, 1, 0, 0, 1, -0.28, 0, 0.67, 10.4, 0, -22.8, 0, -22.7, 0],
                [10, -5, 1, 0, 0, 1, -0.59, 0, -0.43, 12.6, 0, -126.3, 0, -126.3, 0],
                [10, 0, 5, 0, 0, 1, nan, nan, nan, nan, nan, nan, nan, nan, 4],
                [10, 0, 1, -0.01, -0.18, 1, -0.71, -0.11, 0.12, 19, 3, -80.3, -9, -79.7, 0],
                [10, 0, 1, 0, -0.6, 1, nan, nan, nan, nan, nan, nan, nan, nan, 4],
                [10, 0, 1, 0, 0, -1, nan, nan, nan, nan, nan, nan, nan, nan, 4],
                [10, 0, 0, 1, 0, -1, nan, nan, nan, nan, nan, nan, nan, nan, 4],
                [10, 0, -1, 0, 0, -1, nan, nan, nan, nan, nan, nan, nan, nan, 4],
                [10, 0, 0, -1, 0, -1, nan, nan, nan, nan, nan, nan, nan, nan, 4],
                [10, 0, 1, 1, 0, -1, nan, nan, nan, nan, nan, nan, nan, nan, 4],
                [10, 0, 0.2, 0, 0, -1, nan, nan, nan, nan, nan, nan, nan, nan, 4],
                [10, 0, 0.02, 0, 0, -1, nan, nan, nan, nan, nan, nan, nan, nan, 4],
                [10, 5, 1, 0, 0, -1, nan, nan, nan, nan, nan, nan, nan, nan, 4],
                [10, -5, 1, 0, 0, -1, nan, nan, nan, nan, nan, nan, nan, nan, 4],
                [10, 0, 5, 0, 0, -1, nan, nan, nan, nan, nan, nan, nan, nan, 4],
                [20, 0, 1, 0, 0, 1, -0.72, 0, 0.08, 13.4, 0, -83.3, 0, -83.3, 0],
                [10, 0, 8, 0, 0, 1, nan, nan, nan, nan, nan, nan, nan, nan, 2],
                [10, 20, 1, 0, 0, 1, nan, nan, nan, nan, nan, nan, nan, nan, 2],
            ]
        )

        a = 10
        b = 10
        c = 10
        alpha = 90
        beta = 90
        gamma = 120
        u = [1, 0, 0]
        v = [0, 1, 0]
        ol = OrientedLattice(a, b, c, alpha, beta, gamma)
        ol.setUFromVectors(u, v)

        (
            Ei,
            DE,
            H,
            K,
            L,
            sign,
            expected_Qx,
            expected_Qy,
            expected_Qz,
            expected_in_plane_kf_angle,
            expected_out_plane_kf_angle,
            expected_in_plane_Q_angle,
            expected_out_plane_Q_angle,
            expected_omega,
            expected_error_code,
        ) = test_case6.T

        HKL = vstack([H, K, L]).T
        pars = {
            "Ei": Ei,
            "hkl": HKL,
            "DeltaE": DE,
            "sign": sign,
            "detector_constraints": True,
            "horizontal_extent": [5.0, 65.0],
            "vertical_extent": [-7.5, 7.5],
            "horizontal_extent_low": [-1, 1.0],
            "vertical_extent_low": [-1.0, 1.0],
        }
        expected_error_code = [ErrorCodes(ec) for ec in expected_error_code]
        self.run_test(
            ol,
            expected_Qx,
            expected_Qy,
            expected_Qz,
            expected_in_plane_Q_angle,
            expected_out_plane_Q_angle,
            expected_in_plane_kf_angle,
            expected_out_plane_kf_angle,
            expected_omega,
            expected_error_code,
            **pars,
        )

    def test_qangle_NoGC_OC(self):
        # Geometry constraints, no omega constraints, all parameters are arrays
        test_case7 = array(
            [
                [10, 0, 1, 0, 0, 1, nan, nan, nan, nan, nan, nan, nan, nan, 6],
                [10, 0, 0, 1, 0, 1, nan, nan, nan, nan, nan, nan, nan, nan, 6],
                [10, 0, -1, 0, 0, 1, nan, nan, nan, nan, nan, nan, nan, nan, 6],
                [10, 0, 0, -1, 0, 1, -0.72, 0, 0.12, 19, 0, -80.5, 0, 39.5, 0],
                [10, 0, 1, 1, 0, 1, nan, nan, nan, nan, nan, nan, nan, nan, 6],
                [10, 0, 0.2, 0, 0, 1, nan, nan, nan, nan, nan, nan, nan, nan, 6],
                [10, 0, 0.02, 0, 0, 1, nan, nan, nan, nan, nan, nan, nan, nan, 6],
                [10, 5, 1, 0, 0, 1, nan, nan, nan, nan, nan, nan, nan, nan, 6],
                [10, -5, 1, 0, 0, 1, nan, nan, nan, nan, nan, nan, nan, nan, 6],
                [10, 0, 5, 0, 0, 1, nan, nan, nan, nan, nan, nan, nan, nan, 6],
                [10, 0, 1, -0.01, -0.18, 1, nan, nan, nan, nan, nan, nan, nan, nan, 6],
                [10, 0, 1, 0, -0.6, 1, nan, nan, nan, nan, nan, nan, nan, nan, 6],
                [10, 0, 1, 0, 0, -1, nan, nan, nan, nan, nan, nan, nan, nan, 6],
                [10, 0, 0, 1, 0, -1, 0.72, 0, 0.12, -19, 0, 80.5, 0, 20.5, 0],
                [10, 0, -1, 0, 0, -1, nan, nan, nan, nan, nan, nan, nan, nan, 6],
                [10, 0, 0, -1, 0, -1, nan, nan, nan, nan, nan, nan, nan, nan, 6],
                [10, 0, 1, 1, 0, -1, 1.2, 0, 0.36, -33.2, 0, 73.4, 0, 43.4, 0],
                [10, 0, 0.2, 0, 0, -1, nan, nan, nan, nan, nan, nan, nan, nan, 6],
                [10, 0, 0.02, 0, 0, -1, nan, nan, nan, nan, nan, nan, nan, nan, 6],
                [10, 5, 1, 0, 0, -1, 0.28, 0, 0.67, -10.4, 0, 22.8, 0, 22.8, 0],
                [10, -5, 1, 0, 0, -1, nan, nan, nan, nan, nan, nan, nan, nan, 6],
                [10, 0, 5, 0, 0, -1, 2.05, 0, 3, -111.3, 0, 34.4, 0, 34.3, 0],
                [20, 0, 1, 0, 0, 1, nan, nan, nan, nan, nan, nan, nan, nan, 6],
                [10, 0, 8, 0, 0, 1, nan, nan, nan, nan, nan, nan, nan, nan, 2],
                [10, 20, 1, 0, 0, 1, nan, nan, nan, nan, nan, nan, nan, nan, 2],
            ]
        )

        a = 10
        b = 10
        c = 10
        alpha = 90
        beta = 90
        gamma = 120
        u = [1, 0, 0]
        v = [0, 1, 0]
        ol = OrientedLattice(a, b, c, alpha, beta, gamma)
        ol.setUFromVectors(u, v)

        (
            Ei,
            DE,
            H,
            K,
            L,
            sign,
            expected_Qx,
            expected_Qy,
            expected_Qz,
            expected_in_plane_kf_angle,
            expected_out_plane_kf_angle,
            expected_in_plane_Q_angle,
            expected_out_plane_Q_angle,
            expected_omega,
            expected_error_code,
        ) = test_case7.T

        HKL = vstack([H, K, L]).T
        pars = {"Ei": Ei, "hkl": HKL, "DeltaE": DE, "sign": sign, "goniometer_constraints": True, "goniometer_range": [-20.0, 50]}
        expected_error_code = [ErrorCodes(ec) for ec in expected_error_code]
        self.run_test(
            ol,
            expected_Qx,
            expected_Qy,
            expected_Qz,
            expected_in_plane_Q_angle,
            expected_out_plane_Q_angle,
            expected_in_plane_kf_angle,
            expected_out_plane_kf_angle,
            expected_omega,
            expected_error_code,
            **pars,
        )

    def test_qangle_GC_OC(self):
        # Geometry constraints, no omega constraints, all parameters are arrays
        test_case8 = array(
            [
                [10, 0, 1, 0, 0, 1, nan, nan, nan, nan, nan, nan, nan, nan, 6],
                [10, 0, 0, 1, 0, 1, nan, nan, nan, nan, nan, nan, nan, nan, 6],
                [10, 0, -1, 0, 0, 1, nan, nan, nan, nan, nan, nan, nan, nan, 6],
                [10, 0, 0, -1, 0, 1, -0.72, 0, 0.12, 19, 0, -80.5, 0, 39.5, 0],
                [10, 0, 1, 1, 0, 1, nan, nan, nan, nan, nan, nan, nan, nan, 6],
                [10, 0, 0.2, 0, 0, 1, nan, nan, nan, nan, nan, nan, nan, nan, 4],
                [10, 0, 0.02, 0, 0, 1, nan, nan, nan, nan, nan, nan, nan, nan, 4],
                [10, 5, 1, 0, 0, 1, nan, nan, nan, nan, nan, nan, nan, nan, 6],
                [10, -5, 1, 0, 0, 1, nan, nan, nan, nan, nan, nan, nan, nan, 6],
                [10, 0, 5, 0, 0, 1, nan, nan, nan, nan, nan, nan, nan, nan, 4],
                [10, 0, 1, -0.01, -0.18, 1, nan, nan, nan, nan, nan, nan, nan, nan, 6],
                [10, 0, 1, 0, -0.6, 1, nan, nan, nan, nan, nan, nan, nan, nan, 4],
                [10, 0, 1, 0, 0, -1, nan, nan, nan, nan, nan, nan, nan, nan, 4],
                [10, 0, 0, 1, 0, -1, nan, nan, nan, nan, nan, nan, nan, nan, 4],
                [10, 0, -1, 0, 0, -1, nan, nan, nan, nan, nan, nan, nan, nan, 4],
                [10, 0, 0, -1, 0, -1, nan, nan, nan, nan, nan, nan, nan, nan, 4],
                [10, 0, 1, 1, 0, -1, nan, nan, nan, nan, nan, nan, nan, nan, 4],
                [10, 0, 0.2, 0, 0, -1, nan, nan, nan, nan, nan, nan, nan, nan, 4],
                [10, 0, 0.02, 0, 0, -1, nan, nan, nan, nan, nan, nan, nan, nan, 4],
                [10, 5, 1, 0, 0, -1, nan, nan, nan, nan, nan, nan, nan, nan, 4],
                [10, -5, 1, 0, 0, -1, nan, nan, nan, nan, nan, nan, nan, nan, 4],
                [10, 0, 5, 0, 0, -1, nan, nan, nan, nan, nan, nan, nan, nan, 4],
                [20, 0, 1, 0, 0, 1, nan, nan, nan, nan, nan, nan, nan, nan, 6],
                [10, 0, 8, 0, 0, 1, nan, nan, nan, nan, nan, nan, nan, nan, 2],
                [10, 20, 1, 0, 0, 1, nan, nan, nan, nan, nan, nan, nan, nan, 2],
            ]
        )

        a = 10
        b = 10
        c = 10
        alpha = 90
        beta = 90
        gamma = 120
        u = [1, 0, 0]
        v = [0, 1, 0]
        ol = OrientedLattice(a, b, c, alpha, beta, gamma)
        ol.setUFromVectors(u, v)

        (
            Ei,
            DE,
            H,
            K,
            L,
            sign,
            expected_Qx,
            expected_Qy,
            expected_Qz,
            expected_in_plane_kf_angle,
            expected_out_plane_kf_angle,
            expected_in_plane_Q_angle,
            expected_out_plane_Q_angle,
            expected_omega,
            expected_error_code,
        ) = test_case8.T

        HKL = vstack([H, K, L]).T
        pars = {
            "Ei": Ei,
            "hkl": HKL,
            "DeltaE": DE,
            "sign": sign,
            "detector_constraints": True,
            "horizontal_extent": [5.0, 65.0],
            "vertical_extent": [-7.5, 7.5],
            "horizontal_extent_low": [-1, 1.0],
            "vertical_extent_low": [-1.0, 1.0],
            "goniometer_constraints": True,
            "goniometer_range": [-20.0, 50],
        }
        expected_error_code = [ErrorCodes(ec) for ec in expected_error_code]
        self.run_test(
            ol,
            expected_Qx,
            expected_Qy,
            expected_Qz,
            expected_in_plane_Q_angle,
            expected_out_plane_Q_angle,
            expected_in_plane_kf_angle,
            expected_out_plane_kf_angle,
            expected_omega,
            expected_error_code,
            **pars,
        )

    def test_qangle_NoGC_NoOC_newOL(self):
        # No geometry constraints, no omega constraints, all parameters are arrays
        test_case9 = array(
            [
                [10, 0, 1, 0, 0, 1, -2.02, 0, 1.33, 66.8, 0, -56.6, 0, -56.6, 0],
                [10, 0, 0, 1, 0, 1, -1.01, 0, 0.24, 27.3, 0, -76.4, 0, -136.4, 0],
                [10, 0, -1, 0, 0, 1, -2.02, 0, 1.33, 66.8, 0, -56.6, 0, 123.4, 0],
                [10, 0, 0, -1, 0, 1, -1.01, 0, 0.24, 27.3, 0, -76.4, 0, 43.6, 0],
                [10, 0, 1, 1, 0, 1, -2.2, 0, 2.15, 88.7, 0, -45.7, 0, -62.7, 0],
                [10, 0, 0.2, 0, 0, 1, -0.48, 0, 0.05, 12.6, 0, -83.7, 0, -83.7, 0],
                [10, 0, 0.02, 0, 0, 1, -0.04984, 0, 0.00057, 1.3, 0, -89.4, 0, -89.4, 0],
                [10, 5, 1, 0, 0, 1, -1.52, 0, 1.88, 78.2, 0, -39, 0, -39, 0],
                [10, -5, 1, 0, 0, 1, -2.29, 0, 0.78, 58.3, 0, -71.1, 0, -71.1, 0],
                [10, 0, 1.5, 0, 0, 1, -2.05, 0, 3, 111.3, 0, -34.4, 0, -34.3, 0],
                [10, 0, 1, 0, -0.2, 1, -2.02, -0.13, 1.33, 66.8, 3.3, -56.5, -3, -56.6, 0],
                [10, 0, 1, 0, -0.6, 1, -1.99, -0.38, 1.36, 67.4, 10, -55.7, -9, -55.7, 0],  # Qz
                [10, 0, 1, 0, 0, -1, 2.02, 0, 1.33, -66.8, 0, 56.6, 0, 56.6, 0],
                [10, 0, 0, 1, 0, -1, 1.01, 0, 0.24, -27.3, 0, 76.4, 0, 16.4, 0],
                [10, 0, -1, 0, 0, -1, 2.02, 0, 1.33, -66.8, 0, 56.6, 0, -123.4, 0],
                [10, 0, 0, -1, 0, -1, 1.01, 0, 0.24, -27.3, 0, 76.4, 0, -163.6, 0],
                [10, 0, 1, 1, 0, -1, 2.2, 0, 2.15, -88.7, 0, 45.7, 0, 28.7, 0],
                [10, 0, 0.2, 0, 0, -1, 0.48, 0, 0.05, -12.6, 0, 83.7, 0, 83.7, 0],
                [10, 0, 0.02, 0, 0, -1, 0.04984, 0, 0.00057, -1.3, 0, 89.4, 0, 89.4, 0],
                [10, 5, 1, 0, 0, -1, 1.52, 0, 1.88, -78.2, 0, 39, 0, 39, 0],
                [10, -5, 1, 0, 0, -1, 2.29, 0, 0.78, -58.3, 0, 71.1, 0, 71.1, 0],
                [20, 0, 1, 0, 0, 1, -2.23, 0, 0.94, 45.8, 0, -67.1, 0, -67.1, 0],
                [10, 0, 8, 0, 0, 1, nan, nan, nan, nan, nan, nan, nan, nan, 2],
                [10, 20, 1, 0, 0, 1, nan, nan, nan, nan, nan, nan, nan, nan, 2],
            ]
        )

        a = 3
        b = 7
        c = 10
        alpha = 90
        beta = 90
        gamma = 120
        u = [1, 0, 0]
        v = [0, 1, 0]
        ol = OrientedLattice(a, b, c, alpha, beta, gamma)
        ol.setUFromVectors(u, v)

        (
            Ei,
            DE,
            H,
            K,
            L,
            sign,
            expected_Qx,
            expected_Qy,
            expected_Qz,
            expected_in_plane_kf_angle,
            expected_out_plane_kf_angle,
            expected_in_plane_Q_angle,
            expected_out_plane_Q_angle,
            expected_omega,
            expected_error_code,
        ) = test_case9.T

        HKL = vstack([H, K, L]).T
        pars = {"Ei": Ei, "hkl": HKL, "DeltaE": DE, "sign": sign}
        expected_error_code = [ErrorCodes(ec) for ec in expected_error_code]
        self.run_test(
            ol,
            expected_Qx,
            expected_Qy,
            expected_Qz,
            expected_in_plane_Q_angle,
            expected_out_plane_Q_angle,
            expected_in_plane_kf_angle,
            expected_out_plane_kf_angle,
            expected_omega,
            expected_error_code,
            **pars,
        )


if __name__ == "__main__":
    unittest.main()
