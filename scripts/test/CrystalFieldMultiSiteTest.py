# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import numpy as np
import re
import unittest

import CrystalField
from mantid.simpleapi import CalculateChiSquared, CreateWorkspace, FlatBackground, FunctionFactory, Gaussian, LinearBackground

c_mbsr = 79.5774715459  # Conversion from barn to mb/sr


class CrystalFieldMultiSiteTests(unittest.TestCase):
    def test_init_single_ion(self):
        cfms = CrystalField.CrystalFieldMultiSite(Ions="Pm", Symmetries="D2", Temperatures=[20, 52], FWHM=[1.0, 1.0])
        self.assertEqual(cfms.Ions, ["Pm"])
        self.assertEqual(cfms.Symmetries, ["D2"])
        self.assertEqual(cfms.Temperatures, [20, 52])
        self.assertEqual(cfms.FWHM, [1.0, 1.0])

    def test_init_multi_ions(self):
        cfms = CrystalField.CrystalFieldMultiSite(Ions=("Pm", "Eu"), Symmetries=("D2", "C3v"), Temperatures=[20, 52], FWHM=[1.0, 1.0])
        self.assertEqual(cfms.Ions, ["Pm", "Eu"])
        self.assertEqual(cfms.Symmetries, ["D2", "C3v"])
        self.assertEqual(cfms.Temperatures, [20, 52])
        self.assertEqual(cfms.FWHM, [1.0, 1.0])

    def test_toleranceEnergy_property(self):
        cfms = CrystalField.CrystalFieldMultiSite(("Pm", "Eu"), ("D2", "C3v"), ToleranceEnergy=1.0)
        self.assertEqual(cfms.ToleranceEnergy, 1.0)

    def test_toleranceIntensity_property(self):
        cfms = CrystalField.CrystalFieldMultiSite(("Pm", "Eu"), ("D2", "C3v"), ToleranceIntensity=6.0)
        self.assertEqual(cfms.ToleranceIntensity, 6.0)

    def test_NPeaks_property(self):
        cfms = CrystalField.CrystalFieldMultiSite(("Pm", "Eu"), ("D2", "C3v"), NPeaks=11)
        self.assertEqual(cfms.NPeaks, 11)

    def test_fixAllPeaks_property(self):
        cfms = CrystalField.CrystalFieldMultiSite(("Pm", "Eu"), ("D2", "C3v"), FixAllPeaks=True)
        self.assertTrue(cfms.FixAllPeaks)

    def test_peakShape_property(self):
        cfms = CrystalField.CrystalFieldMultiSite(("Pm", "Eu"), ("D2", "C3v"), PeakShape="Gaussian")
        self.assertEqual(cfms.PeakShape, "Gaussian")

    def test_FWHM_variation_property(self):
        cfms = CrystalField.CrystalFieldMultiSite(("Pm", "Eu"), ("D2", "C3v"), FWHMVariation=0.3)
        self.assertEqual(cfms.FWHMVariation, 0.3)

    def test_init_parameters(self):
        cfms = CrystalField.CrystalFieldMultiSite(
            Ions="Ce", Symmetries="C2", Temperatures=[15], FWHM=[1.0], parameters={"BmolX": 1.0, "B40": -0.02}
        )
        self.assertEqual(cfms.function.getParameterValue("BmolX"), 1.0)
        self.assertEqual(cfms.function.getParameterValue("B40"), -0.02)

    def test_init_parameters_multi_ions(self):
        cfms = CrystalField.CrystalFieldMultiSite(
            Ions=("Pm", "Eu"),
            Symmetries=("D2", "C3v"),
            Temperatures=[20, 52],
            FWHM=[1.0, 1.0],
            parameters={"ion0.B40": -0.02, "ion0.B42": -0.11, "ion1.B42": -0.12},
        )
        self.assertEqual(cfms.function.getParameterValue("ion0.B42"), -0.11)
        self.assertEqual(cfms.function.getParameterValue("ion0.B40"), -0.02)
        self.assertEqual(cfms.function.getParameterValue("ion1.B42"), -0.12)

    def test_peak_values(self):
        cfms = CrystalField.CrystalFieldMultiSite(Ions="Ce", Symmetries="C2", Temperatures=[25], FWHM=[1.5], BmolX=1.0, B40=-0.02)
        self.assertEqual(int(cfms.function.getParameterValue("pk0.Amplitude")), 48)
        self.assertEqual(cfms.function.getParameterValue("pk0.FWHM"), 1.5)
        self.assertEqual(cfms.function.getParameterValue("pk0.PeakCentre"), 0)

    def test_peak_values_gaussian(self):
        cfms = CrystalField.CrystalFieldMultiSite(
            Ions="Ce", Symmetries="C2", Temperatures=[25], FWHM=[1.0], PeakShape="Gaussian", BmolX=1.0, B40=-0.02
        )
        self.assertEqual(int(cfms.function.getParameterValue("pk0.Height")), 45)
        self.assertAlmostEqual(cfms.function.getParameterValue("pk0.Sigma"), 0.42, 2)
        self.assertEqual(cfms.function.getParameterValue("pk0.PeakCentre"), 0)

    def test_peak_values_multi_ions(self):
        cfms = CrystalField.CrystalFieldMultiSite(
            Ions=("Pr", "Nd"), Symmetries=("D2", "C3v"), Temperatures=[20], FWHM=[1.5], parameters={"ion0.B60": -0.02, "ion1.B62": -0.12}
        )
        self.assertEqual(int(cfms.function.getParameterValue("ion0.pk0.Amplitude")), 278)
        self.assertEqual(cfms.function.getParameterValue("ion0.pk0.FWHM"), 1.5)
        self.assertEqual(cfms.function.getParameterValue("ion0.pk1.PeakCentre"), 982.8)
        self.assertEqual(int(cfms.function.getParameterValue("ion1.pk0.Amplitude")), 234)
        self.assertEqual(cfms.function.getParameterValue("ion1.pk0.FWHM"), 1.5)
        self.assertAlmostEqual(cfms.function.getParameterValue("ion1.pk1.PeakCentre"), 1749.981919, 6)

    def test_peak_values_multi_ions_and_spectra(self):
        cfms = CrystalField.CrystalFieldMultiSite(
            Ions=("Pm", "Ce"),
            Symmetries=("D2", "C3v"),
            Temperatures=[20, 52],
            FWHM=[1.0, 1.0],
            parameters={"ion0.B40": -0.02, "ion1.B42": -0.12},
        )
        self.assertEqual(int(cfms.function.getParameterValue("ion0.sp0.pk0.Amplitude")), 308)
        self.assertEqual(cfms.function.getParameterValue("ion0.sp1.pk0.FWHM"), 1.0)
        self.assertEqual(cfms.function.getParameterValue("ion0.sp0.pk1.PeakCentre"), 0)
        self.assertEqual(int(cfms.function.getParameterValue("ion1.sp1.pk0.Amplitude")), 103)
        self.assertEqual(cfms.function.getParameterValue("ion1.sp1.pk0.FWHM"), 1.0)
        self.assertAlmostEqual(cfms.function.getParameterValue("ion1.sp0.pk1.PeakCentre"), 8.519155, 6)

    def test_peak_values_multi_gaussian(self):
        cfms = CrystalField.CrystalFieldMultiSite(
            Ions=("Pm", "Dy"),
            Symmetries=("D2", "C3v"),
            Temperatures=[20, 52],
            FWHM=[1.0, 1.5],
            parameters={"ion0.B40": -0.02, "ion1.B42": -0.12},
            PeakShape="Gaussian",
        )
        self.assertEqual(int(cfms.function.getParameterValue("ion0.sp0.pk0.Height")), 289)
        self.assertAlmostEqual(cfms.function.getParameterValue("ion0.sp1.pk0.Sigma"), 0.64, 2)
        self.assertEqual(cfms.function.getParameterValue("ion0.sp0.pk1.PeakCentre"), 0)
        self.assertEqual(int(cfms.function.getParameterValue("ion1.sp1.pk0.Height")), 1642)
        self.assertAlmostEqual(cfms.function.getParameterValue("ion1.sp1.pk0.Sigma"), 0.64, 2)
        self.assertAlmostEqual(cfms.function.getParameterValue("ion1.sp0.pk1.PeakCentre"), 40.957515, 6)

    def test_get_spectrum_from_list(self):
        cfms = CrystalField.CrystalFieldMultiSite(
            Ions=["Ce"],
            Symmetries=["C2v"],
            Temperatures=[4.0],
            FWHM=[0.1],
            B20=0.035,
            B40=-0.012,
            B43=-0.027,
            B60=-0.00012,
            B63=0.0025,
            B66=0.0068,
            ToleranceIntensity=0.001 * c_mbsr,
        )
        r = [0.0, 1.45, 2.4, 3.0, 3.85]
        x, y = cfms.getSpectrum(r)
        y = y / c_mbsr
        expected_y = [12.474955, 1.190169, 0.122781, 0.042940, 10.837438]
        np.testing.assert_equal(x, r)
        np.testing.assert_almost_equal(y, expected_y, 6)

    def test_get_spectrum_ws(self):
        cfms = CrystalField.CrystalFieldMultiSite(
            ["Ce"],
            ["C2v"],
            B20=0.035,
            B40=-0.012,
            B43=-0.027,
            B60=-0.00012,
            B63=0.0025,
            B66=0.0068,
            Temperatures=[4.0],
            FWHM=[0.1],
            ToleranceIntensity=0.001 * c_mbsr,
        )

        x = np.linspace(0.0, 2.0, 30)
        y = np.zeros_like(x)
        e = np.ones_like(x)
        ws = CreateWorkspace(x, y, e)
        x, y = cfms.getSpectrum(0, ws)
        y = y / c_mbsr
        self.assertAlmostEqual(y[0], 12.474955, 6)
        self.assertAlmostEqual(y[1], 4.300416, 6)
        self.assertAlmostEqual(y[2], 1.452309, 6)
        self.assertAlmostEqual(y[3], 0.692266, 6)
        self.assertAlmostEqual(y[4], 0.401079, 6)
        self.assertAlmostEqual(y[15], 0.050130, 6)
        self.assertAlmostEqual(y[16], 0.054428, 6)
        x, y = cfms.getSpectrum(ws)
        y = y / c_mbsr
        self.assertAlmostEqual(y[0], 12.474955, 6)
        self.assertAlmostEqual(y[1], 4.300416, 6)
        ws = CreateWorkspace(x, y, e, 2)
        x, y = cfms.getSpectrum(ws, 1)
        y = y / c_mbsr
        self.assertAlmostEqual(y[0], 0.050130, 6)
        self.assertAlmostEqual(y[1], 0.054428, 6)

    def test_get_spectrum_xrange(self):
        cfms = CrystalField.CrystalFieldMultiSite(
            ["Ce"],
            ["C2v"],
            B20=0.035,
            B40=-0.012,
            B43=-0.027,
            B60=-0.00012,
            B63=0.0025,
            B66=0.0068,
            Temperatures=[4.0],
            FWHM=[0.1],
            ToleranceIntensity=0.001 * c_mbsr,
        )

        x, y = cfms.getSpectrum(x_range=(0, 3))
        y = y / c_mbsr
        self.assertAlmostEqual(y[0], 12.474954833572541, 6)
        self.assertAlmostEqual(y[1], 11.435723018801308, 6)
        self.assertAlmostEqual(y[2], 9.14934495705407, 6)
        self.assertAlmostEqual(y[3], 6.862946934125062, 6)
        self.assertAlmostEqual(y[4], 5.0846344775777395, 6)
        self.assertAlmostEqual(y[15], 0.5854829967281244, 6)
        self.assertAlmostEqual(y[16], 0.5180575611727711, 6)

    def test_get_spectrum_from_list_multi_spectra(self):
        cfms = CrystalField.CrystalFieldMultiSite(
            Ions=["Ce"],
            Symmetries=["C2v"],
            Temperatures=[4.0, 50.0],
            FWHM=[0.1, 0.2],
            B20=0.035,
            B40=-0.012,
            B43=-0.027,
            B60=-0.00012,
            B63=0.0025,
            B66=0.0068,
        )
        r = [0.0, 1.45, 2.4, 3.0, 3.85]
        x, y = cfms.getSpectrum(0, r)
        y = y / c_mbsr
        expected_y = [12.474946, 1.190160, 0.122785, 0.042940, 10.837170]
        np.testing.assert_equal(x, r)
        np.testing.assert_almost_equal(y, expected_y, 6)

        x, y = cfms.getSpectrum(1, r)
        y = y / c_mbsr
        expected_y = [6.304662, 0.331218, 1.224681, 0.078540, 2.638049]
        np.testing.assert_equal(x, r)
        np.testing.assert_almost_equal(y, expected_y, 6)

    def test_get_spectrum_ws_multi_spectra(self):
        cfms = CrystalField.CrystalFieldMultiSite(
            ["Ce"],
            ["C2v"],
            B20=0.035,
            B40=-0.012,
            B43=-0.027,
            B60=-0.00012,
            B63=0.0025,
            B66=0.0068,
            Temperatures=[4.0, 50.0],
            FWHM=[0.1, 0.2],
        )

        x = np.linspace(0.0, 2.0, 30)
        y = np.zeros_like(x)
        e = np.ones_like(x)
        ws = CreateWorkspace(x, y, e)
        x, y = cfms.getSpectrum(0, ws)
        y = y / c_mbsr
        self.assertAlmostEqual(y[0], 12.474945990071641, 6)
        self.assertAlmostEqual(y[1], 4.3004130214544389, 6)
        self.assertAlmostEqual(y[2], 1.4523079303712476, 6)
        self.assertAlmostEqual(y[3], 0.6922657279528992, 6)
        self.assertAlmostEqual(y[4], 0.40107924259746491, 6)
        self.assertAlmostEqual(y[15], 0.050129858433581413, 6)
        self.assertAlmostEqual(y[16], 0.054427788297191478, 6)
        x, y = cfms.getSpectrum(1, ws)
        y = y / c_mbsr
        self.assertAlmostEqual(y[0], 6.3046623789675627, 6)
        self.assertAlmostEqual(y[1], 4.2753024205094912, 6)
        self.assertAlmostEqual(y[2], 2.1778204115683644, 6)
        self.assertAlmostEqual(y[3], 1.2011173460849718, 6)
        self.assertAlmostEqual(y[4], 0.74036730921135963, 6)
        x, y = cfms.getSpectrum(ws)
        y = y / c_mbsr
        self.assertAlmostEqual(y[0], 12.474945990071641, 6)
        self.assertAlmostEqual(y[1], 4.3004130214544389, 6)
        ws = CreateWorkspace(x, y, e, 2)
        x, y = cfms.getSpectrum(ws, 1)
        y = y / c_mbsr
        self.assertAlmostEqual(y[0], 0.050129858433581413, 6)
        self.assertAlmostEqual(y[1], 0.054427788297191478, 6)

    def test_get_spectrum_list_multi_ion_and_spectra(self):
        params = {
            "ion0.B20": 0.37737,
            "ion0.B22": 3.9770,
            "ion0.B40": -0.031787,
            "ion0.B42": -0.11611,
            "ion0.B44": -0.12544,
            "ion1.B20": 0.37737,
            "ion1.B22": 3.9770,
            "ion1.B40": -0.031787,
            "ion1.B42": -0.11611,
            "ion1.B44": -0.12544,
        }
        cfms = CrystalField.CrystalFieldMultiSite(
            Ions=["Ce", "Pr"], Symmetries=["C2v", "C2v"], Temperatures=[44.0, 50.0], FWHM=[1.1, 1.2], parameters=params
        )
        r = [0.0, 1.45, 2.4, 3.0, 3.85]
        x, y = cfms.getSpectrum(0, r)
        y = y / c_mbsr
        expected_y = [3.904037, 0.744519, 0.274897, 0.175713, 0.106540]
        np.testing.assert_equal(x, r)
        np.testing.assert_almost_equal(y, expected_y, 6)

        x, y = cfms.getSpectrum(1, r)
        y = y / c_mbsr
        expected_y = [3.704726, 0.785600, 0.296255, 0.190176, 0.115650]
        np.testing.assert_equal(x, r)
        np.testing.assert_almost_equal(y, expected_y, 6)

    def test_fit_multi_ion_single_spectrum(self):
        params = {"B20": 0.37737, "B22": 3.9770, "B40": -0.031787, "B42": -0.11611, "B44": -0.12544, "Temperature": 44.0, "FWHM": 1.1}
        cf1 = CrystalField.CrystalField("Ce", "C2v", **params)
        cf2 = CrystalField.CrystalField("Pr", "C2v", **params)
        cf = cf1 + cf2
        r = [0.0, 1.45, 2.4, 3.0, 3.85]
        x, y = cf.getSpectrum(r)
        ws = CrystalField.fitting.makeWorkspace(x, y)

        params = {
            "ion0.B20": 0.37737,
            "ion0.B22": 3.9770,
            "ion0.B40": -0.031787,
            "ion0.B42": -0.11611,
            "ion0.B44": -0.12544,
            "ion1.B20": 0.37737,
            "ion1.B22": 3.9770,
            "ion1.B40": -0.031787,
            "ion1.B42": -0.11611,
            "ion1.B44": -0.12544,
        }
        cf = CrystalField.CrystalFieldMultiSite(
            Ions=["Ce", "Pr"],
            Symmetries=["C2v", "C2v"],
            Temperatures=[44.0],
            FWHM=[1.1],
            ToleranceIntensity=6.0,
            ToleranceEnergy=1.0,
            FixAllPeaks=True,
            parameters=params,
        )

        cf.fix(
            "ion0.BmolX",
            "ion0.BmolY",
            "ion0.BmolZ",
            "ion0.BextX",
            "ion0.BextY",
            "ion0.BextZ",
            "ion0.B40",
            "ion0.B42",
            "ion0.B44",
            "ion0.B60",
            "ion0.B62",
            "ion0.B64",
            "ion0.B66",
            "ion0.IntensityScaling",
            "ion1.BmolX",
            "ion1.BmolY",
            "ion1.BmolZ",
            "ion1.BextX",
            "ion1.BextY",
            "ion1.BextZ",
            "ion1.B40",
            "ion1.B42",
            "ion1.B44",
            "ion1.B60",
            "ion1.B62",
            "ion1.B64",
            "ion1.B66",
            "ion1.IntensityScaling",
        )

        chi2 = CalculateChiSquared(cf.makeSpectrumFunction(), InputWorkspace=ws)[1]

        fit = CrystalField.CrystalFieldFit(Model=cf, InputWorkspace=ws, MaxIterations=10)
        fit.fit()

        self.assertGreater(cf.chi2, 0.0)
        self.assertLess(cf.chi2, chi2)

    def test_fit_multi_ion_and_spectra(self):
        params = {
            "B20": 0.37737,
            "B22": 3.9770,
            "B40": -0.031787,
            "B42": -0.11611,
            "B44": -0.12544,
            "Temperature": [44.0, 50.0],
            "FWHM": [1.1, 0.9],
        }
        cf1 = CrystalField.CrystalField("Ce", "C2v", **params)
        cf2 = CrystalField.CrystalField("Pr", "C2v", **params)
        cf = cf1 + cf2
        r = [0.0, 1.45, 2.4, 3.0, 3.85]
        ws1 = CrystalField.fitting.makeWorkspace(*cf.getSpectrum(0, r))
        ws2 = CrystalField.fitting.makeWorkspace(*cf.getSpectrum(1, r))

        params = {
            "ion0.B20": 0.37737,
            "ion0.B22": 3.9770,
            "ion0.B40": -0.031787,
            "ion0.B42": -0.11611,
            "ion0.B44": -0.12544,
            "ion1.B20": 0.37737,
            "ion1.B22": 3.9770,
            "ion1.B40": -0.031787,
            "ion1.B42": -0.11611,
            "ion1.B44": -0.12544,
        }
        cf = CrystalField.CrystalFieldMultiSite(
            Ions=["Ce", "Pr"],
            Symmetries=["C2v", "C2v"],
            Temperatures=[44.0, 50.0],
            FWHM=[1.0, 1.0],
            ToleranceIntensity=6.0,
            ToleranceEnergy=1.0,
            FixAllPeaks=True,
            parameters=params,
        )

        cf.fix(
            "ion0.BmolX",
            "ion0.BmolY",
            "ion0.BmolZ",
            "ion0.BextX",
            "ion0.BextY",
            "ion0.BextZ",
            "ion0.B40",
            "ion0.B42",
            "ion0.B44",
            "ion0.B60",
            "ion0.B62",
            "ion0.B64",
            "ion0.B66",
            "ion0.IntensityScaling",
            "ion1.BmolX",
            "ion1.BmolY",
            "ion1.BmolZ",
            "ion1.BextX",
            "ion1.BextY",
            "ion1.BextZ",
            "ion1.B40",
            "ion1.B42",
            "ion1.B44",
            "ion1.B60",
            "ion1.B62",
            "ion1.B64",
            "ion1.B66",
            "ion1.IntensityScaling",
            "sp0.IntensityScaling",
            "sp1.IntensityScaling",
        )

        chi2 = CalculateChiSquared(str(cf.function), InputWorkspace=ws1, InputWorkspace_1=ws2)[1]

        fit = CrystalField.CrystalFieldFit(Model=cf, InputWorkspace=[ws1, ws2], MaxIterations=10)
        fit.fit()

        self.assertGreater(cf.chi2, 0.0)
        self.assertLess(cf.chi2, chi2)

    def test_set_background(self):
        cf = CrystalField.CrystalFieldMultiSite(
            Ions="Ce", Symmetries="C2v", Temperatures=[20], FWHM=[1.0], Background="name=LinearBackground,A0=1"
        )
        self.assertEqual('"name=LinearBackground,A0=1"', cf["Background"])
        self.assertEqual(cf.background.param["A0"], 1)
        self.assertEqual(cf.background.background.param["A0"], 1)
        cf.background.param["A0"] = 0
        self.assertEqual(cf.background.background.param["A0"], 0)

    def test_set_background_as_function(self):
        cf = CrystalField.CrystalFieldMultiSite(
            Ions="Ce", Symmetries="C2v", Temperatures=[20], FWHM=[1.0], Background=LinearBackground(A0=1)
        )
        self.assertEqual('"name=LinearBackground,A0=1,A1=0"', cf["Background"])
        self.assertEqual(cf.background.param["A0"], 1)
        self.assertEqual(cf.background.background.param["A0"], 1)
        cf.background.param["A0"] = 0
        self.assertEqual(cf.background.background.param["A0"], 0)

    def test_set_background_with_peak(self):
        cf = CrystalField.CrystalFieldMultiSite(
            Ions="Ce",
            Symmetries="C2v",
            Temperatures=[20],
            FWHM=[1.0],
            Background="name=LinearBackground",
            BackgroundPeak=Gaussian(Height=1),
        )
        self.assertEqual('"name=Gaussian,Height=1,PeakCentre=0,Sigma=0;name=LinearBackground"', cf["Background"])
        self.assertEqual(cf.background.peak.param["Height"], 1)
        self.assertEqual(cf.background.param["f0.Height"], 1)
        self.assertEqual(cf.background.background.param["A0"], 0)
        cf.background.peak.param["Height"] = 0
        cf.background.background.param["A0"] = 1
        self.assertEqual(cf.background.peak.param["Height"], 0)
        self.assertEqual(cf.background.background.param["A0"], 1)

    def test_set_background_peak_only(self):
        cf = CrystalField.CrystalFieldMultiSite(
            Ions="Ce", Symmetries="C2v", Temperatures=[20], FWHM=[1.0], BackgroundPeak=Gaussian(Sigma=1)
        )
        self.assertEqual('"name=Gaussian,Height=0,PeakCentre=0,Sigma=1"', cf["Background"])
        self.assertEqual(cf.background.peak.param["Sigma"], 1)
        self.assertEqual(cf.background.param["Sigma"], 1)
        cf.background.peak.param["Sigma"] = 0
        self.assertEqual(cf.background.param["Sigma"], 0)

    def test_set_background_composite(self):
        cf = CrystalField.CrystalFieldMultiSite(
            Ions="Ce", Symmetries="C2v", Temperatures=[20], FWHM=[1.0], Background=Gaussian(PeakCentre=1) + LinearBackground()
        )
        self.assertEqual('"name=Gaussian,Height=0,PeakCentre=1,Sigma=0;name=LinearBackground,A0=0,A1=0"', cf["Background"])
        cf.background.param["f1.A0"] = 1
        cf.background.param["f0.PeakCentre"] = 0.5
        self.assertEqual(cf.background.param["f1.A0"], 1)
        self.assertEqual(cf.background.param["f0.PeakCentre"], 0.5)

    def test_set_background_composite_as_string(self):
        cf = CrystalField.CrystalFieldMultiSite(
            Ions="Ce",
            Symmetries="C2v",
            Temperatures=[20],
            FWHM=[1.0],
            Background="name=Gaussian,Height=0,PeakCentre=1,Sigma=0;name=LinearBackground,A0=0,A1=0",
        )
        self.assertEqual('"name=Gaussian,Height=0,PeakCentre=1,Sigma=0;name=LinearBackground,A0=0,A1=0"', cf["Background"])
        self.assertEqual(cf.background.param["f0.Sigma"], 0)
        cf.background.param["f1.A0"] = 1
        self.assertEqual(cf.background.param["f1.A0"], 1)

    def test_constraints_single_spectrum(self):
        cf = CrystalField.CrystalFieldMultiSite(
            Ions=["Ce"],
            Symmetries=["C2v"],
            Temperatures=[50],
            FWHM=[0.9],
            B20=0.37737,
            B22=3.9770,
            B40=-0.031787,
            B42=-0.11611,
            B44=-0.12544,
            Background=LinearBackground(A0=1.0),
            BackgroundPeak=Gaussian(Height=10, Sigma=0.3),
        )

        cf.ties(B40="B20/2")
        cf.constraints("IntensityScaling > 0", "B22 < 4")
        cf.constraints("pk0.FWHM < 2.2", "pk1.FWHM >= 0.1")
        cf.ties({"pk2.FWHM": "2*pk1.FWHM", "pk3.FWHM": "2*pk2.FWHM"})
        cf.background.peak.ties(Height=10.1)
        cf.background.peak.constraints("Sigma > 0")
        cf.background.background.ties(A0=0.1)
        cf.background.background.constraints("A1 > 0")

        s = cf.makeSpectrumFunction()
        ties = ",".join(re.findall(r"ties=\((.*?)\)", s))
        constraints = ",".join(re.findall(r"constraints=\((.*?)\)", s))
        self.assertTrue("0<IntensityScaling" in constraints)
        self.assertTrue("B22<4" in constraints)
        self.assertTrue("0<bg.f0.Sigma" in constraints)
        self.assertTrue("0<bg.f1.A1" in constraints)
        self.assertTrue("Height=10.1" in ties)
        self.assertTrue("A0=0.1" in ties)
        self.assertTrue("pk0.FWHM<2.2" in constraints)
        self.assertTrue("0.1<pk1.FWHM" in constraints)
        self.assertTrue("pk2.FWHM=2*pk1.FWHM" in ties)
        self.assertTrue("pk3.FWHM=2*pk2.FWHM" in ties)

        # Test that ties and constraints are correctly defined
        fun = FunctionFactory.createInitialized(s)
        self.assertNotEqual(fun, None)

    def test_constraints_multi_spectrum(self):
        cf = CrystalField.CrystalFieldMultiSite(
            Ions=["Ce"],
            Symmetries=["C2v"],
            Temperatures=[44, 50],
            FWHM=[1.1, 0.9],
            Background=FlatBackground(),
            BackgroundPeak=Gaussian(Height=10, Sigma=0.3),
            B20=0.37737,
            B22=3.9770,
            B40=-0.031787,
            B42=-0.11611,
            B44=-0.12544,
        )

        cf.constraints("sp0.IntensityScaling > 0", "0 < sp1.IntensityScaling < 2", "B22 < 4")

        cf.ties({"sp0.bg.f0.Height": 10.1})
        cf.constraints("sp0.bg.f0.Sigma > 0.1")
        cf.ties({"sp1.bg.f0.Height": 20.2})
        cf.constraints("sp1.bg.f0.Sigma > 0.2")

        cf.ties({"sp1.pk2.FWHM": "2*sp1.pk1.FWHM", "sp1.pk3.FWHM": "2*sp1.pk2.FWHM"})
        cf.constraints("sp0.pk1.FWHM < 2.2")
        cf.constraints("sp1.pk1.FWHM > 1.1", "1 < sp1.pk4.FWHM < 2.2")

        s = str(cf.function)
        ties = ",".join(re.findall(r"ties=\((.*?)\)", s))
        constraints = ",".join(re.findall(r"constraints=\((.*?)\)", s))
        self.assertTrue("0<sp0.IntensityScaling" in constraints)
        self.assertTrue("sp1.IntensityScaling<2" in constraints)
        self.assertTrue("sp0.bg.f0.Height=10.1" in ties)
        self.assertTrue("sp1.bg.f0.Height=20.2" in ties)
        self.assertTrue("0.1<sp0.bg.f0.Sigma" in constraints)
        self.assertTrue("0.2<sp1.bg.f0.Sigma" in constraints)
        self.assertTrue("sp0.pk1.FWHM<2.2" in constraints)
        self.assertTrue("1.1<sp1.pk1.FWHM" in constraints)
        self.assertTrue("1<sp1.pk4.FWHM<2.2" in constraints)
        self.assertTrue("sp1.pk2.FWHM=2*sp1.pk1.FWHM" in ties)
        self.assertTrue("sp1.pk3.FWHM=2*sp1.pk2.FWHM" in ties)

    def test_constraints_multi_spectrum_and_ion(self):
        cf = CrystalField.CrystalFieldMultiSite(
            Ions=["Ce", "Pr"],
            Symmetries=["C2v", "C2v"],
            Temperatures=[44, 50],
            FWHM=[1.1, 0.9],
            Background=FlatBackground(),
            BackgroundPeak=Gaussian(Height=10, Sigma=0.3),
            parameters={"ion0.B20": 0.37737, "ion0.B22": 3.9770, "ion1.B40": -0.031787, "ion1.B42": -0.11611, "ion1.B44": -0.12544},
        )

        cf.constraints("sp0.IntensityScaling > 0", "0 < sp1.IntensityScaling < 2", "ion0.B22 < 4")

        cf.ties({"sp0.bg.f0.Height": 10.1})
        cf.constraints("sp0.bg.f0.Sigma > 0.1")
        cf.ties({"sp1.bg.f0.Height": 20.2})
        cf.constraints("sp1.bg.f0.Sigma > 0.2")

        cf.ties({"ion0.sp1.pk2.FWHM": "2*ion0.sp1.pk1.FWHM", "ion1.sp1.pk3.FWHM": "2*ion1.sp1.pk2.FWHM"})
        cf.constraints("ion0.sp0.pk1.FWHM < 2.2")
        cf.constraints("ion0.sp1.pk1.FWHM > 1.1", "1 < ion0.sp1.pk4.FWHM < 2.2")

        s = str(cf.function)
        ties = ",".join(re.findall(r"ties=\((.*?)\)", s))
        constraints = ",".join(re.findall(r"constraints=\((.*?)\)", s))
        self.assertTrue("0<sp0.IntensityScaling" in constraints)
        self.assertTrue("sp1.IntensityScaling<2" in constraints)
        self.assertTrue("sp0.bg.f0.Height=10.1" in ties)
        self.assertTrue("sp1.bg.f0.Height=20.2" in ties)
        self.assertTrue("0.1<sp0.bg.f0.Sigma" in constraints)
        self.assertTrue("0.2<sp1.bg.f0.Sigma" in constraints)
        self.assertTrue("ion0.sp0.pk1.FWHM<2.2" in constraints)
        self.assertTrue("1.1<ion0.sp1.pk1.FWHM" in constraints)
        self.assertTrue("1<ion0.sp1.pk4.FWHM<2.2" in constraints)
        self.assertTrue("ion0.sp1.pk2.FWHM=2*ion0.sp1.pk1.FWHM" in ties)
        self.assertTrue("ion1.sp1.pk3.FWHM=2*ion1.sp1.pk2.FWHM" in ties)

    def test_add_CrystalField(self):
        cfms = CrystalField.CrystalFieldMultiSite(
            Ions=["Pr"],
            Symmetries=["C2v"],
            Temperatures=[44, 50],
            FWHM=[1.1, 0.9],
            B20=0.37737,
            B22=3.9770,
            B40=-0.031787,
            B42=-0.11611,
            B44=-0.15,
        )

        params = {"B20": 0.37737, "B22": 3.9770, "B40": -0.031787, "B42": -0.11611, "B44": -0.12544}
        cf = CrystalField.CrystalField("Ce", "C2v", **params)

        cf2 = cfms + cf
        self.assertEqual(cf2["ion0.B20"], 0.37737)
        self.assertEqual(cf2["ion1.B20"], 0.37737)
        self.assertEqual(cf2["ion0.B44"], -0.15)
        self.assertEqual(cf2["ion1.B44"], -0.12544)
        self.assertEqual(len(cf2.Ions), 2)
        self.assertTrue("Ce" in cf2.Ions)
        self.assertTrue("Pr" in cf2.Ions)
        self.assertEqual(len(cf2.Symmetries), 2)
        s = str(cf2.function)
        ties = ",".join(re.findall(r"ties=\((.*?)\)", s))
        self.assertTrue(
            "ion1.IntensityScaling=1.0*ion0.IntensityScaling" in ties or "ion0.IntensityScaling=1.0*ion1.IntensityScaling" in ties
        )  # either possible in python 3

    def test_add_CrystalFieldSite(self):
        cfms = CrystalField.CrystalFieldMultiSite(
            Ions=["Pr"],
            Symmetries=["C2v"],
            Temperatures=[44, 50],
            FWHM=[1.1, 0.9],
            B20=0.37737,
            B22=3.9770,
            B40=-0.031787,
            B42=-0.11611,
            B44=-0.15,
        )

        params = {"B20": 0.37737, "B22": 3.9770, "B40": -0.031787, "B42": -0.11611, "B44": -0.12544}
        cf = CrystalField.CrystalField("Ce", "C2v", **params)
        cf = cf * 8
        cf2 = cfms + cf
        self.assertEqual(cf2["ion0.B20"], 0.37737)
        self.assertEqual(cf2["ion1.B20"], 0.37737)
        self.assertEqual(cf2["ion0.B44"], -0.15)
        self.assertEqual(cf2["ion1.B44"], -0.12544)
        self.assertEqual(len(cf2.Ions), 2)
        self.assertTrue("Ce" in cf2.Ions)
        self.assertTrue("Pr" in cf2.Ions)
        self.assertEqual(len(cf2.Symmetries), 2)
        s = str(cf2.function)
        ties = ",".join(re.findall(r"ties=\((.*?)\)", s))
        self.assertTrue("ion0.IntensityScaling=0.125*ion1.IntensityScaling" in ties)

    def test_add_CrystalFieldMultiSite(self):
        cfms1 = CrystalField.CrystalFieldMultiSite(
            Ions=["Pm"], Symmetries=["D2"], Temperatures=[44, 50], FWHM=[1.1, 0.9], B20=0.37737, B40=-0.031787, B42=-0.11611, B44=-0.15
        )
        cfms2 = CrystalField.CrystalFieldMultiSite(
            Ions=["Ce", "Pr"],
            Symmetries=["C2v", "C2v"],
            Temperatures=[44, 50],
            FWHM=[1.1, 0.9],
            parameters={"ion0.B20": 0.34, "ion0.B22": 3.9770, "ion1.B40": -0.03},
        )
        cfms3 = cfms1 + cfms2
        self.assertEqual(len(cfms3.Ions), 3)
        self.assertEqual(len(cfms3.Symmetries), 3)
        self.assertEqual(cfms3["ion0.B20"], 0.37737)
        self.assertEqual(cfms3["ion1.B20"], 0.34)
        self.assertEqual(cfms3["ion1.B22"], 3.9770)
        self.assertEqual(cfms3["ion2.B40"], -0.03)
        s = str(cfms3.function)
        ties = ",".join(re.findall(r"ties=\((.*?)\)", s))
        self.assertTrue(
            "ion1.IntensityScaling=1.0*ion0.IntensityScaling" in ties or "ion0.IntensityScaling=1.0*ion1.IntensityScaling" in ties
        )
        self.assertTrue(
            "ion0.IntensityScaling=1.0*ion2.IntensityScaling" in ties or "ion2.IntensityScaling=1.0*ion0.IntensityScaling" in ties
        )

    def test_multi_ion_intensity_scaling(self):
        params = {"B20": 0.37737, "B22": 3.9770, "B40": -0.031787, "B42": -0.11611, "B44": -0.12544, "Temperature": 44.0, "FWHM": 1.1}
        cf1 = CrystalField.CrystalField("Ce", "C2v", **params)
        cf2 = CrystalField.CrystalField("Pr", "C2v", **params)
        cf = cf1 + cf2
        ties = ",".join(re.findall(r"ties=\((.*?)\)", str(cf.function)))
        self.assertTrue(
            "ion1.IntensityScaling=1.0*ion0.IntensityScaling" in ties or "ion0.IntensityScaling=1.0*ion1.IntensityScaling" in ties
        )  # either possible in python 3
        cf = 2 * cf1 + cf2 * 8

        ties = ",".join(re.findall(r"ties=\((.*?)\)", str(cf.function)))
        self.assertTrue("ion0.IntensityScaling=0.25*ion1.IntensityScaling" in ties)

        cf = 2 * cf1 + cf2
        ties = ",".join(re.findall(r"ties=\((.*?)\)", str(cf.function)))
        self.assertTrue("ion1.IntensityScaling=0.5*ion0.IntensityScaling" in ties)

        cf3 = CrystalField.CrystalField("Tb", "C2v", **params)
        cf = 2 * cf1 + cf2 * 8 + cf3
        ties = ",".join(re.findall(r"ties=\((.*?)\)", str(cf.function)))
        self.assertTrue("ion0.IntensityScaling=0.25*ion1.IntensityScaling" in ties)
        self.assertTrue("ion2.IntensityScaling=0.125*ion1.IntensityScaling" in ties)

        cf = 2 * cf1 + cf2 * 8 + 10 * cf3
        ties = ",".join(re.findall(r"ties=\((.*?)\)", str(cf.function)))
        self.assertTrue("ion0.IntensityScaling=0.2*ion2.IntensityScaling" in ties)
        self.assertTrue("ion1.IntensityScaling=0.8*ion2.IntensityScaling" in ties)

        cf = 2 * cf1 + (cf2 * 8 + 10 * cf3)
        ties = ",".join(re.findall(r"ties=\((.*?)\)", str(cf.function)))
        self.assertTrue("ion0.IntensityScaling=0.2*ion2.IntensityScaling" in ties)
        self.assertTrue("ion1.IntensityScaling=0.8*ion2.IntensityScaling" in ties)

        cf = cf1 + (cf2 * 8 + 10 * cf3)
        ties = ",".join(re.findall(r"ties=\((.*?)\)", str(cf.function)))
        self.assertTrue("ion0.IntensityScaling=0.1*ion2.IntensityScaling" in ties)
        self.assertTrue("ion1.IntensityScaling=0.8*ion2.IntensityScaling" in ties)

        cf4 = CrystalField.CrystalField("Yb", "C2v", **params)
        cf = (2 * cf1 + cf2 * 8) + (10 * cf3 + cf4)
        ties = ",".join(re.findall(r"ties=\((.*?)\)", str(cf.function)))
        self.assertTrue("ion0.IntensityScaling=0.2*ion2.IntensityScaling" in ties)
        self.assertTrue("ion1.IntensityScaling=0.8*ion2.IntensityScaling" in ties)
        self.assertTrue("ion3.IntensityScaling=0.1*ion2.IntensityScaling" in ties)

        cf1.IntensityScaling = 1e2
        cf2.IntensityScaling = 1e2
        cf = cf1 + cf2
        ties = ",".join(re.findall(r"ties=\((.*?)\)", str(cf.function)))
        self.assertTrue(
            "ion1.IntensityScaling=1.0*ion0.IntensityScaling" in ties or "ion0.IntensityScaling=1.0*ion1.IntensityScaling" in ties
        )  # either possible in python 3

    def test_multi_ion_intensity_scaling_multi_spectrum(self):
        params = {
            "B20": 0.37737,
            "B22": 3.9770,
            "B40": -0.031787,
            "B42": -0.11611,
            "B44": -0.12544,
            "Temperature": [44.0, 50.0],
            "FWHM": [1.1, 1.6],
        }
        cf1 = CrystalField.CrystalField("Ce", "C2v", **params)
        cf2 = CrystalField.CrystalField("Pr", "C2v", **params)
        cf = cf1 + cf2
        ties = ",".join(re.findall(r"ties=\((.*?)\)", str(cf.function)))
        self.assertTrue(
            "ion1.IntensityScaling=1.0*ion0.IntensityScaling" in ties or "ion0.IntensityScaling=1.0*ion1.IntensityScaling" in ties
        )  # either possible in python 3

        cf = 2 * cf1 + cf2 * 8
        ties = ",".join(re.findall(r"ties=\((.*?)\)", str(cf.function)))
        self.assertTrue("ion0.IntensityScaling=0.25*ion1.IntensityScaling" in ties)

        cf = 2 * cf1 + cf2
        ties = ",".join(re.findall(r"ties=\((.*?)\)", str(cf.function)))
        self.assertTrue("ion1.IntensityScaling=0.5*ion0.IntensityScaling" in ties)

        cf3 = CrystalField.CrystalField("Tb", "C2v", **params)
        cf = 2 * cf1 + cf2 * 8 + cf3
        ties = ",".join(re.findall(r"ties=\((.*?)\)", str(cf.function)))
        self.assertTrue("ion0.IntensityScaling=0.25*ion1.IntensityScaling" in ties)
        self.assertTrue("ion2.IntensityScaling=0.125*ion1.IntensityScaling" in ties)

        cf = 2 * cf1 + cf2 * 8 + 10 * cf3
        ties = ",".join(re.findall(r"ties=\((.*?)\)", str(cf.function)))
        self.assertTrue("ion0.IntensityScaling=0.2*ion2.IntensityScaling" in ties)
        self.assertTrue("ion1.IntensityScaling=0.8*ion2.IntensityScaling" in ties)

        cf = 2 * cf1 + (cf2 * 8 + 10 * cf3)
        ties = ",".join(re.findall(r"ties=\((.*?)\)", str(cf.function)))
        self.assertTrue("ion0.IntensityScaling=0.2*ion2.IntensityScaling" in ties)
        self.assertTrue("ion1.IntensityScaling=0.8*ion2.IntensityScaling" in ties)

        cf = cf1 + (cf2 * 8 + 10 * cf3)
        ties = ",".join(re.findall(r"ties=\((.*?)\)", str(cf.function)))
        self.assertTrue("ion0.IntensityScaling=0.1*ion2.IntensityScaling" in ties)
        self.assertTrue("ion1.IntensityScaling=0.8*ion2.IntensityScaling" in ties)

        cf4 = CrystalField.CrystalField("Yb", "C2v", **params)
        cf = (2 * cf1 + cf2 * 8) + (10 * cf3 + cf4)
        ties = ",".join(re.findall(r"ties=\((.*?)\)", str(cf.function)))
        self.assertTrue("ion0.IntensityScaling=0.2*ion2.IntensityScaling" in ties)
        self.assertTrue("ion1.IntensityScaling=0.8*ion2.IntensityScaling" in ties)
        self.assertTrue("ion3.IntensityScaling=0.1*ion2.IntensityScaling" in ties)

        cf1.IntensityScaling = [1e2, 1e2]
        cf2.IntensityScaling = [1e2, 1e2]
        cf = cf1 + cf2
        ties = ",".join(re.findall(r"ties=\((.*?)\)", str(cf.function)))
        self.assertTrue(
            "ion1.IntensityScaling=1.0*ion0.IntensityScaling" in ties or "ion0.IntensityScaling=1.0*ion1.IntensityScaling" in ties
        )  # either possible in python 3

    def test_multi_ion_ties_and_fixes(self):
        params = {"B20": 0.37737, "B22": 3.9770, "B40": -0.031787, "B42": -0.11611, "B44": -0.12544, "Temperature": 44.0, "FWHM": 1.1}
        cf1 = CrystalField.CrystalField("Ce", "C2v", **params)
        cf2 = CrystalField.CrystalField("Pr", "C2v", **params)
        cf = cf1 + cf2
        self.assertTrue(cf.function.isFixed(cf.function.getParameterIndex("ion0.BmolX")))
        self.assertTrue(cf.function.isFixed(cf.function.getParameterIndex("ion0.B21")))
        self.assertTrue(cf.function.isFixed(cf.function.getParameterIndex("ion0.B43")))
        self.assertTrue(cf.function.isFixed(cf.function.getParameterIndex("ion0.IB65")))
        self.assertTrue(cf.function.isFixed(cf.function.getParameterIndex("ion1.BmolZ")))
        self.assertTrue(cf.function.isFixed(cf.function.getParameterIndex("ion1.B41")))
        self.assertTrue(cf.function.isFixed(cf.function.getParameterIndex("ion1.B66")))
        self.assertTrue(cf.function.isFixed(cf.function.getParameterIndex("ion1.IB62")))

    def test_fit_gaussian_peaks(self):
        origin = CrystalField.CrystalField(
            "Ce", "C2v", B20=0.37737, B22=3.9770, B40=-0.031787, B42=-0.11611, B44=-0.12544, Temperature=44.0, FWHM=1.1
        )
        x, y = origin.getSpectrum()
        ws = CrystalField.fitting.makeWorkspace(x, y)

        params = {"B20": 0.37737, "B22": 3.9770, "B40": -0.031787, "B42": -0.11611, "B44": -0.12544, "Temperature": 44.0, "FWHM": 1.1}
        cf1 = CrystalField.CrystalField("Ce", "C2v", **params)
        cf2 = CrystalField.CrystalField("Pr", "C2v", **params)
        cf = cf1 + cf2
        cf.PeakShape = "Gaussian"

        chi2 = CalculateChiSquared(cf.makeSpectrumFunction(), InputWorkspace=ws)[1]

        fit = CrystalField.CrystalFieldFit(Model=cf, InputWorkspace=ws, MaxIterations=10)
        fit.fit()

        self.assertGreater(cf.chi2, 0.0)
        self.assertLess(cf.chi2, chi2)

    def test_two_step_fit_multi_ion_single_spectrum(self):
        params = {"B20": 0.37737, "B22": 3.9770, "B40": -0.031787, "B42": -0.11611, "B44": -0.12544, "Temperature": 44.0, "FWHM": 1.1}
        cf1 = CrystalField.CrystalField("Ce", "C2v", **params)
        cf2 = CrystalField.CrystalField("Pr", "C2v", **params)
        cf = cf1 + cf2
        r = [0.0, 1.45, 2.4, 3.0, 3.85]
        x, y = cf.getSpectrum(r)
        ws = CrystalField.fitting.makeWorkspace(x, y)

        params = {
            "ion0.B20": 0.37737,
            "ion0.B22": 3.9770,
            "ion0.B40": -0.031787,
            "ion0.B42": -0.11611,
            "ion0.B44": -0.12544,
            "ion1.B20": 0.37737,
            "ion1.B22": 3.9770,
            "ion1.B40": -0.031787,
            "ion1.B42": -0.11611,
            "ion1.B44": -0.12544,
        }
        cf = CrystalField.CrystalFieldMultiSite(
            Ions=["Ce", "Pr"],
            Symmetries=["C2v", "C2v"],
            Temperatures=[44.0],
            FWHM=[1.1],
            ToleranceIntensity=6.0,
            ToleranceEnergy=1.0,
            FixAllPeaks=True,
            parameters=params,
        )

        cf.fix(
            "ion0.BmolX",
            "ion0.BmolY",
            "ion0.BmolZ",
            "ion0.BextX",
            "ion0.BextY",
            "ion0.BextZ",
            "ion0.B40",
            "ion0.B42",
            "ion0.B44",
            "ion0.B60",
            "ion0.B62",
            "ion0.B64",
            "ion0.B66",
            "ion0.IntensityScaling",
            "ion1.BmolX",
            "ion1.BmolY",
            "ion1.BmolZ",
            "ion1.BextX",
            "ion1.BextY",
            "ion1.BextZ",
            "ion1.B40",
            "ion1.B42",
            "ion1.B44",
            "ion1.B60",
            "ion1.B62",
            "ion1.B64",
            "ion1.B66",
            "ion1.IntensityScaling",
        )

        chi2 = CalculateChiSquared(cf.makeSpectrumFunction(), InputWorkspace=ws)[1]

        fit = CrystalField.CrystalFieldFit(Model=cf, InputWorkspace=ws, MaxIterations=10)
        fit.two_step_fit(OverwriteMaxIterations=[2, 2], OverwriteMinimizers=["BFGS", "BFGS"], Iterations=2)

        self.assertGreater(cf.chi2, 0.0)
        self.assertLess(cf.chi2, chi2)

    def test_two_step_fit_multi_ion_and_spectra(self):
        params = {
            "B20": 0.37737,
            "B22": 3.9770,
            "B40": -0.031787,
            "B42": -0.11611,
            "B44": -0.12544,
            "Temperature": [44.0, 50.0],
            "FWHM": [1.1, 0.9],
        }
        cf1 = CrystalField.CrystalField("Ce", "C2v", **params)
        cf2 = CrystalField.CrystalField("Pr", "C2v", **params)
        cf = cf1 + cf2
        r = [0.0, 1.45, 2.4, 3.0, 3.85]
        ws1 = CrystalField.fitting.makeWorkspace(*cf.getSpectrum(0, r))
        ws2 = CrystalField.fitting.makeWorkspace(*cf.getSpectrum(1, r))

        params = {
            "ion0.B20": 0.37737,
            "ion0.B22": 3.9770,
            "ion0.B40": -0.031787,
            "ion0.B42": -0.11611,
            "ion0.B44": -0.12544,
            "ion1.B20": 0.37737,
            "ion1.B22": 3.9770,
            "ion1.B40": -0.031787,
            "ion1.B42": -0.11611,
            "ion1.B44": -0.12544,
        }
        cf = CrystalField.CrystalFieldMultiSite(
            Ions=["Ce", "Pr"],
            Symmetries=["C2v", "C2v"],
            Temperatures=[44.0, 50.0],
            FWHM=[1.0, 1.0],
            ToleranceIntensity=6.0,
            ToleranceEnergy=1.0,
            FixAllPeaks=True,
            parameters=params,
        )

        cf.fix(
            "ion0.BmolX",
            "ion0.BmolY",
            "ion0.BmolZ",
            "ion0.BextX",
            "ion0.BextY",
            "ion0.BextZ",
            "ion0.B40",
            "ion0.B42",
            "ion0.B44",
            "ion0.B60",
            "ion0.B62",
            "ion0.B64",
            "ion0.B66",
            "ion0.IntensityScaling",
            "ion1.BmolX",
            "ion1.BmolY",
            "ion1.BmolZ",
            "ion1.BextX",
            "ion1.BextY",
            "ion1.BextZ",
            "ion1.B40",
            "ion1.B42",
            "ion1.B44",
            "ion1.B60",
            "ion1.B62",
            "ion1.B64",
            "ion1.B66",
            "ion1.IntensityScaling",
            "sp0.IntensityScaling",
            "sp1.IntensityScaling",
        )

        chi2 = CalculateChiSquared(str(cf.function), InputWorkspace=ws1, InputWorkspace_1=ws2)[1]

        fit = CrystalField.CrystalFieldFit(Model=cf, InputWorkspace=[ws1, ws2], MaxIterations=10)
        fit.two_step_fit(OverwriteMaxIterations=[2, 2], OverwriteMinimizers=["BFGS", "BFGS"], Iterations=2)

        self.assertGreater(cf.chi2, 0.0)
        self.assertLess(cf.chi2, chi2)


if __name__ == "__main__":
    unittest.main()
