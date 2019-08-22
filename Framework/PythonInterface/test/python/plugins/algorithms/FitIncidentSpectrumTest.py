# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest
import numpy as np
from mantid.simpleapi import AnalysisDataService, FitIncidentSpectrum, CalculateEfficiencyCorrection, CloneWorkspace, ConvertToPointData, \
    CreateSampleWorkspace, DeleteWorkspace, LoadAscii, Multiply, CreateWorkspace, Rebin, Divide, AlgorithmManager
from testhelpers import run_algorithm


class FitIncidentSpectrumTest(unittest.TestCase):

    incident_wksp_name = 'incident_spectrum_wksp'
    phiMax = 6324
    phiEpi = 786
    alpha = 0.099
    lambda1 = 0.67143
    lambda2 = 0.06075
    lambdaT = 1.58
    binning_default = "0.2,0.01,4.0"

    def setUp(self):
        # Create the workspace to hold the already corrected incident spectrum
        self.incident_wksp = CreateWorkspace(OutputWorkspace=self.incident_wksp_name,
                                             NSpec=1,
                                             DataX=[0],
                                             DataY=[0],
                                             UnitX='Wavelength',
                                             VerticalAxisUnit='Text',
                                             VerticalAxisValues='IncidentSpectrum')
        self.incident_wksp = Rebin(InputWorkspace=self.incident_wksp,
                                   OutputWorkspace="foobar",
                                   Params=self.binning_default)
        self.incident_wksp = ConvertToPointData(InputWorkspace=self.incident_wksp,
                                                OutputWorkspace="foobar")
        # Add the incident spectrum to the workspace
        corrected_spectrum = self.generate_incident_spectrum(self.incident_wksp.readX(0),
                                                             self.phiMax,
                                                             self.phiEpi,
                                                             self.alpha,
                                                             self.lambda1,
                                                             self.lambda2,
                                                             self.lambdaT)
        self.incident_wksp.setY(0, corrected_spectrum)
        self.agl_instance = FitIncidentSpectrum

    def generate_incident_spectrum(self, wavelengths, phi_max, phi_epi, alpha, lambda_1, lambda_2, lambda_T):
        delta_term = 1. / (1. + np.exp((wavelengths - lambda_1) / lambda_2))
        term1 = phi_max * (lambda_T ** 4. / wavelengths ** 5.) * np.exp(-(lambda_T / wavelengths) ** 2.)
        term2 = phi_epi * delta_term / (wavelengths ** (1 + 2 * alpha))
        return term1 + term2

    def test_fit_cubic_spline_with_gauss_conv_produces_fit_with_same_length_as_binning_for_fit(self):
        binning_for_fit = "0.2,0.1,5.0"
        binning_for_calc = "0.2,0.1,3.0"
        alg_test = run_algorithm(
            "FitIncidentSpectrum",
            InputWorkspace=self.incident_wksp,
            OutputWorkspace="fit_wksp",
            BinningForCalc=binning_for_fit,
            BinningForFit=binning_for_calc,
            FitSpectrumWith="GaussConvCubicSpline")
        self.assertTrue(alg_test.isExecuted())
        fit_wksp = AnalysisDataService.retrieve("fit_wksp")
        self.assertEqual(fit_wksp.readX(0).size, np.arange(0.2, 5, 0.1).size)

    def test_fit_cubic_spline_with_gauss_conv(self):
        self.incident_wksp = CreateWorkspace(OutputWorkspace=self.incident_wksp_name,
                                             NSpec=1,
                                             DataX=[0],
                                             DataY=[0],
                                             UnitX='Wavelength',
                                             VerticalAxisUnit='Text',
                                             VerticalAxisValues='IncidentSpectrum')
        binning_for_fit = "0.2,0.1,5.0"
        binning_for_calc = "0.2,0.1,3.0"
        alg_test = run_algorithm(
            "FitIncidentSpectrum",
            InputWorkspace=self.incident_wksp,
            OutputWorkspace="fit_wksp",
            BinningForCalc=binning_for_fit,
            BinningForFit=binning_for_calc,
            FitSpectrumWith="GaussConvCubicSpline")
        self.assertTrue(alg_test.isExecuted())
        fit_wksp = AnalysisDataService.retrieve("fit_wksp")
        print(fit_wksp.readY(0))


if __name__ == '__main__':
    unittest.main()
