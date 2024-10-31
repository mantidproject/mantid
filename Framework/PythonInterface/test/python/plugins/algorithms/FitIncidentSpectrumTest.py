# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import numpy as np
from mantid.simpleapi import (
    AnalysisDataService,
    FitIncidentSpectrum,
    ConvertToPointData,
    CreateWorkspace,
    Rebin,
)
from testhelpers import run_algorithm


class FitIncidentSpectrumTest(unittest.TestCase):
    incident_wksp_name = "incident_spectrum_wksp"
    phiMax = 6324
    phiEpi = 786
    alpha = 0.099
    lambda1 = 0.67143
    lambda2 = 0.06075
    lambdaT = 1.58
    binning_default = "0.2,0.01,4.0"

    def setUp(self):
        # Create the workspace to hold the already corrected incident spectrum
        self.incident_wksp = CreateWorkspace(
            OutputWorkspace=self.incident_wksp_name,
            NSpec=1,
            DataX=[0],
            DataY=[0],
            UnitX="Wavelength",
            VerticalAxisUnit="Text",
            VerticalAxisValues="IncidentSpectrum",
        )
        self.incident_wksp = Rebin(InputWorkspace=self.incident_wksp, OutputWorkspace="foobar", Params=self.binning_default)
        self.incident_wksp = ConvertToPointData(InputWorkspace=self.incident_wksp, OutputWorkspace="foobar")
        # Add the incident spectrum to the workspace
        corrected_spectrum = self.generate_incident_spectrum(
            self.incident_wksp.readX(0), self.phiMax, self.phiEpi, self.alpha, self.lambda1, self.lambda2, self.lambdaT
        )
        self.incident_wksp.setY(0, corrected_spectrum)
        self.agl_instance = FitIncidentSpectrum

    def generate_incident_spectrum(self, wavelengths, phi_max, phi_epi, alpha, lambda_1, lambda_2, lambda_T):
        delta_term = 1.0 / (1.0 + np.exp((wavelengths - lambda_1) / lambda_2))
        term1 = phi_max * (lambda_T**4.0 / wavelengths**5.0) * np.exp(-((lambda_T / wavelengths) ** 2.0))
        term2 = phi_epi * delta_term / (wavelengths ** (1 + 2 * alpha))
        return term1 + term2

    def test_fit_cubic_spline_with_gauss_conv_produces_fit_with_same_range_as_binning_for_calc(self):
        binning_for_calc = "0.2,0.1,3.0"
        binning_for_fit = "0.2,0.1,4.0"
        alg_test = run_algorithm(
            "FitIncidentSpectrum",
            InputWorkspace=self.incident_wksp,
            OutputWorkspace="fit_wksp",
            BinningForCalc=binning_for_calc,
            BinningForFit=binning_for_fit,
            FitSpectrumWith="GaussConvCubicSpline",
        )
        self.assertTrue(alg_test.isExecuted())
        fit_wksp = AnalysisDataService.retrieve("fit_wksp")
        self.assertEqual(fit_wksp.readX(0).all(), np.arange(0.2, 3, 0.01).all())

    def test_fit_cubic_spline_produces_fit_with_same_range_as_binning_for_calc(self):
        binning_for_calc = "0.2,0.1,3.0"
        binning_for_fit = "0.2,0.1,4.0"
        alg_test = run_algorithm(
            "FitIncidentSpectrum",
            InputWorkspace=self.incident_wksp,
            OutputWorkspace="fit_wksp",
            BinningForCalc=binning_for_calc,
            BinningForFit=binning_for_fit,
            FitSpectrumWith="CubicSpline",
        )
        self.assertTrue(alg_test.isExecuted())
        fit_wksp = AnalysisDataService.retrieve("fit_wksp")
        self.assertEqual(fit_wksp.readX(0).all(), np.arange(0.2, 3, 0.1).all())

    def test_fit_cubic_spline_via_mantid_produces_fit_with_same_range_as_binning_for_calc(self):
        binning_for_calc = "0.2,0.1,3.0"
        binning_for_fit = "0.2,0.1,4.0"
        alg_test = run_algorithm(
            "FitIncidentSpectrum",
            InputWorkspace=self.incident_wksp,
            OutputWorkspace="fit_wksp",
            BinningForCalc=binning_for_calc,
            BinningForFit=binning_for_fit,
            FitSpectrumWith="CubicSplineViaMantid",
        )
        self.assertTrue(alg_test.isExecuted())
        fit_wksp = AnalysisDataService.retrieve("fit_wksp")
        self.assertEqual(fit_wksp.readX(0).all(), np.arange(0.2, 3, 0.1).all())

    def test_fit_cubic_spline_both_derivatives(self):
        binning_for_calc = "0.2,0.1,3.0"
        binning_for_fit = "0.2,0.1,4.0"
        alg_test = run_algorithm(
            "FitIncidentSpectrum",
            InputWorkspace=self.incident_wksp,
            OutputWorkspace="fit_wksp",
            BinningForCalc=binning_for_calc,
            BinningForFit=binning_for_fit,
            FitSpectrumWith="CubicSpline",
            DerivOrder=2,
        )
        self.assertTrue(alg_test.isExecuted())
        fit_wksp = AnalysisDataService.retrieve("fit_wksp")
        # check values at peak at wavelength~1.0 A
        self.assertAlmostEqual(fit_wksp.readY(0)[8], 43064.09, 2)
        self.assertAlmostEqual(fit_wksp.readY(1)[8], -9772.89, 2)
        self.assertAlmostEqual(fit_wksp.readY(2)[8], -494934.77, 2)


if __name__ == "__main__":
    unittest.main()
