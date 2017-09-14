import numpy as np
import unittest
from CrystalField.CrystalFieldMultiSite import CrystalFieldMultiSite

c_mbsr = 79.5774715459  # Conversion from barn to mb/sr

class CrystalFieldMultiSiteTests(unittest.TestCase):

    def xtest_init_single_ion(self):
        cfms = CrystalFieldMultiSite(Ions='Pm', Symmetries='D2', Temperatures=[20, 52], FWHMs=[1.0, 1.0])
        self.assertEqual(cfms.Ions, ['Pm'])
        self.assertEqual(cfms.Symmetries, ['D2'])
        self.assertEqual(cfms.Temperatures, [20, 52])
        self.assertEqual(cfms.FWHMs, [1.0, 1.0])

    def xtest_init_multi_ions(self):
        cfms = CrystalFieldMultiSite(Ions=('Pm', 'Eu'), Symmetries=('D2', 'C3v'), Temperatures=[20, 52],
                                     FWHMs=[1.0, 1.0])
        self.assertEqual(cfms.Ions, ['Pm', 'Eu'])
        self.assertEqual(cfms.Symmetries, ['D2', 'C3v'])
        self.assertEqual(cfms.Temperatures, [20, 52])
        self.assertEqual(cfms.FWHMs, [1.0, 1.0])

    def xtest_toleranceEnergy_property(self):
        cfms = CrystalFieldMultiSite(('Pm', 'Eu'), ('D2', 'C3v'), ToleranceEnergy=1.0)
        self.assertEqual(cfms.ToleranceEnergy, 1.0)

    def xtest_toleranceIntensity_property(self):
        cfms = CrystalFieldMultiSite(('Pm', 'Eu'), ('D2', 'C3v'), ToleranceIntensity=6.0)
        self.assertEqual(cfms.ToleranceIntensity, 6.0)

    def xtest_NPeaks_property(self):
        cfms = CrystalFieldMultiSite(('Pm', 'Eu'), ('D2', 'C3v'), NPeaks=11)
        self.assertEqual(cfms.NPeaks, 11)

    def xtest_fixAllPeaks_property(self):
        cfms = CrystalFieldMultiSite(('Pm', 'Eu'), ('D2', 'C3v'), FixAllPeaks=True)
        self.assertTrue(cfms.FixAllPeaks)

    def xtest_peakShape_property(self):
        cfms = CrystalFieldMultiSite(('Pm', 'Eu'), ('D2', 'C3v'), PeakShape='Gaussian')
        self.assertEqual(cfms.PeakShape, 'Gaussian')

    def xtest_FWHM_variation_property(self):
        cfms = CrystalFieldMultiSite(('Pm', 'Eu'), ('D2', 'C3v'), FWHMVariation=0.3)
        self.assertEqual(cfms.FWHMVariation, 0.3)

    def xtest_init_parameters(self):
        cfms = CrystalFieldMultiSite(Ions='Ce', Symmetries='C2', Temperatures=[15], FWHMs=[1.0],
                                     parameters={'BmolX': 1.0, 'B40': -0.02})
        self.assertEqual(cfms.function.getParameterValue('BmolX'), 1.0)
        self.assertEqual(cfms.function.getParameterValue('B40'), -0.02)

    def xtest_init_parameters_multi_ions(self):
        cfms = CrystalFieldMultiSite(Ions=('Pm', 'Eu'), Symmetries=('D2', 'C3v'), Temperatures=[20, 52],
                                     FWHMs=[1.0, 1.0], parameters={'ion0.B40': -0.02, 'ion0.B42': -0.11,
                                                                   'ion1.B42': -0.12})
        self.assertEqual(cfms.function.getParameterValue('ion0.B42'), -0.11)
        self.assertEqual(cfms.function.getParameterValue('ion0.B40'), -0.02)
        self.assertEqual(cfms.function.getParameterValue('ion1.B42'), -0.12)

    def xtest_peak_values(self):
        cfms = CrystalFieldMultiSite(Ions='Ce', Symmetries='C2', Temperatures=[25], FWHMs=[1.5],
                                     BmolX=1.0, B40=-0.02)
        self.assertEqual(int(cfms.function.getParameterValue('pk0.Amplitude')), 48)
        self.assertEqual(cfms.function.getParameterValue('pk0.FWHM'), 1.5)
        self.assertEqual(cfms.function.getParameterValue('pk0.PeakCentre'), 0)

    def xtest_peak_values_gaussian(self):
        cfms = CrystalFieldMultiSite(Ions='Ce', Symmetries='C2', Temperatures=[25], FWHMs=[1.0], PeakShape='Gaussian',
                                     BmolX=1.0, B40=-0.02)
        self.assertEqual(int(cfms.function.getParameterValue('pk0.Height')), 45)
        self.assertAlmostEqual(cfms.function.getParameterValue('pk0.Sigma'), 0.42, 2)
        self.assertEqual(cfms.function.getParameterValue('pk0.PeakCentre'), 0)

    def xtest_peak_values_multi_ions(self):
        cfms = CrystalFieldMultiSite(Ions=('Pr', 'Nd'), Symmetries=('D2', 'C3v'), Temperatures=[20],
                                     FWHMs=[1.5], parameters={'ion0.B60': -0.02, 'ion1.B62': -0.12})
        self.assertEqual(int(cfms.function.getParameterValue('ion0.pk0.Amplitude')), 278)
        self.assertEqual(cfms.function.getParameterValue('ion0.pk0.FWHM'), 1.5)
        self.assertEqual(cfms.function.getParameterValue('ion0.pk1.PeakCentre'), 982.8)
        self.assertEqual(int(cfms.function.getParameterValue('ion1.pk0.Amplitude')), 234)
        self.assertEqual(cfms.function.getParameterValue('ion1.pk0.FWHM'), 1.5)
        self.assertAlmostEqual(cfms.function.getParameterValue('ion1.pk1.PeakCentre'), 1749.981919, 6)

    def xtest_peak_values_multi_ions_and_spectra(self):
        cfms = CrystalFieldMultiSite(Ions=('Pm', 'Ce'), Symmetries=('D2', 'C3v'), Temperatures=[20, 52],
                                     FWHMs=[1.0, 1.0], parameters={'ion0.B40': -0.02, 'ion1.B42': -0.12})
        self.assertEqual(int(cfms.function.getParameterValue('ion0.sp0.pk0.Amplitude')), 308)
        self.assertEqual(cfms.function.getParameterValue('ion0.sp1.pk0.FWHM'), 1.0)
        self.assertEqual(cfms.function.getParameterValue('ion0.sp0.pk1.PeakCentre'), 0)
        self.assertEqual(int(cfms.function.getParameterValue('ion1.sp1.pk0.Amplitude')), 103)
        self.assertEqual(cfms.function.getParameterValue('ion1.sp1.pk0.FWHM'), 1.0)
        self.assertAlmostEqual(cfms.function.getParameterValue('ion1.sp0.pk1.PeakCentre'), 8.519155, 6)

    def xtest_peak_values_multi_gaussian(self):
        cfms = CrystalFieldMultiSite(Ions=('Pm', 'Dy'), Symmetries=('D2', 'C3v'), Temperatures=[20, 52],
                                     FWHMs=[1.0, 1.5], parameters={'ion0.B40': -0.02, 'ion1.B42': -0.12},
                                     PeakShape='Gaussian')
        self.assertEqual(int(cfms.function.getParameterValue('ion0.sp0.pk0.Height')), 289)
        self.assertAlmostEqual(cfms.function.getParameterValue('ion0.sp1.pk0.Sigma'), 0.64, 2)
        self.assertEqual(cfms.function.getParameterValue('ion0.sp0.pk1.PeakCentre'), 0)
        self.assertEqual(int(cfms.function.getParameterValue('ion1.sp1.pk0.Height')), 1642)
        self.assertAlmostEqual(cfms.function.getParameterValue('ion1.sp1.pk0.Sigma'), 0.64, 2)
        self.assertAlmostEqual(cfms.function.getParameterValue('ion1.sp0.pk1.PeakCentre'), 40.957515, 6)

    def xtest_get_spectrum_from_list(self):
        cfms = CrystalFieldMultiSite(Ions=['Ce'], Symmetries=['C2v'], Temperatures=[4.0], FWHMs=[0.1],
                                     B20=0.035, B40=-0.012, B43=-0.027, B60=-0.00012, B63=0.0025, B66=0.0068,
                                     ToleranceIntensity=0.001*c_mbsr)
        r = [0.0, 1.45, 2.4, 3.0, 3.85]
        x, y = cfms.getSpectrum(r)
        y = y / c_mbsr
        expected_y = [13.950363, 0.02298, 0.031946, 0.189161, 0.392888]
        np.testing.assert_equal(x, r)
        np.testing.assert_almost_equal(y, expected_y, 6)

    def xtest_get_spectrum_ws(self):
        from mantid.simpleapi import CreateWorkspace
        cfms = CrystalFieldMultiSite(['Ce'], ['C2v'], B20=0.035, B40=-0.012, B43=-0.027, B60=-0.00012, B63=0.0025,
                                     B66=0.0068, Temperatures=[4.0], FWHMs=[0.1], ToleranceIntensity=0.001*c_mbsr)

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

    def xtest_get_spectrum_from_list_multi_spectra(self):
        cfms = CrystalFieldMultiSite(Ions=['Ce'], Symmetries=['C2v'], Temperatures=[4.0, 50.0], FWHMs=[0.1, 0.2],
                                     B20=0.035, B40=-0.012, B43=-0.027, B60=-0.00012, B63=0.0025, B66=0.0068)
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

    def xtest_get_spectrum_ws_multi_spectra(self):
        from mantid.simpleapi import CreateWorkspace
        cfms = CrystalFieldMultiSite(['Ce'], ['C2v'], B20=0.035, B40=-0.012, B43=-0.027, B60=-0.00012, B63=0.0025, B66=0.0068,
                          Temperatures=[4.0, 50.0], FWHMs=[0.1, 0.2])

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

    def xtest_get_spectrum_list_multi_ion_and_spectra(self):
        params = {'ion0.B20': 0.37737, 'ion0.B22': 3.9770, 'ion0.B40': -0.031787, 'ion0.B42': -0.11611,
                  'ion0.B44': -0.12544, 'ion1.B20': 0.37737, 'ion1.B22': 3.9770, 'ion1.B40': -0.031787,
                  'ion1.B42': -0.11611, 'ion1.B44': -0.12544}
        cfms = CrystalFieldMultiSite(Ions=['Ce', 'Pr'], Symmetries=['C2v', 'C2v'], Temperatures=[44.0, 50.0],
                                     FWHMs=[1.1, 1.2], parameters=params)
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

    def xtest_fit_multi_ion_single_spectrum(self):

        from CrystalField.fitting import makeWorkspace
        from CrystalField import CrystalField, CrystalFieldFit
        from mantid.simpleapi import CalculateChiSquared

        params = {'B20': 0.37737, 'B22': 3.9770, 'B40': -0.031787, 'B42': -0.11611, 'B44': -0.12544,
                  'Temperature': 44.0, 'FWHM': 1.1}
        cf1 = CrystalField('Ce', 'C2v', **params)
        cf2 = CrystalField('Pr', 'C2v', **params)
        cf = cf1 + cf2
        x, y = cf.getSpectrum()
        ws = makeWorkspace(x, y)

        params = {'ion0.B20': 0.37737, 'ion0.B22': 3.9770, 'ion0.B40': -0.031787, 'ion0.B42': -0.11611,
                  'ion0.B44': -0.12544, 'ion1.B20': 0.37737, 'ion1.B22': 3.9770, 'ion1.B40': -0.031787,
                  'ion1.B42': -0.11611, 'ion1.B44': -0.12544}
        cf = CrystalFieldMultiSite(Ions=['Ce', 'Pr'], Symmetries=['C2v', 'C2v'], Temperatures=[44.0], FWHMs=[1.1],
                                   ToleranceIntensity=6.0, ToleranceEnergy=1.0, FixAllPeaks=True, parameters=params)

        cf.fix('ion0.BmolX', 'ion0.BmolY', 'ion0.BmolZ', 'ion0.BextX', 'ion0.BextY', 'ion0.BextZ', 'ion0.B40', 
               'ion0.B42', 'ion0.B44', 'ion0.B60', 'ion0.B62', 'ion0.B64', 'ion0.B66', 'ion0.IntensityScaling',
               'ion1.BmolX', 'ion1.BmolY', 'ion1.BmolZ', 'ion1.BextX', 'ion1.BextY', 'ion1.BextZ', 'ion1.B40',
               'ion1.B42', 'ion1.B44', 'ion1.B60', 'ion1.B62', 'ion1.B64', 'ion1.B66', 'ion1.IntensityScaling')

        chi2 = CalculateChiSquared(cf.makeSpectrumFunction(), InputWorkspace=ws)[1]

        fit = CrystalFieldFit(Model=cf, InputWorkspace=ws, MaxIterations=10)
        fit.fit()

        # f = cf.function
        # for i in range(f.nParams()):
        #     if not f.isFixed(i):
        #         print i, f.parameterName(i), f.getParameterValue(i)
        # self.assertFalse(True)

        self.assertTrue(cf.chi2 > 0.0)
        self.assertTrue(cf.chi2 < chi2)

    def xtest_fit_multi_ion_and_spectra(self):
        from CrystalField.fitting import makeWorkspace
        from CrystalField import CrystalField, CrystalFieldFit
        from mantid.simpleapi import CalculateChiSquared

        params = {'B20': 0.37737, 'B22': 3.9770, 'B40': -0.031787, 'B42': -0.11611, 'B44': -0.12544,
                  'Temperature': [44.0, 50.0], 'FWHM': [1.1, 0.9]}
        cf1 = CrystalField('Ce', 'C2v', **params)
        cf2 = CrystalField('Pr', 'C2v', **params)
        cf = cf1 + cf2
        ws1 = makeWorkspace(*cf.getSpectrum(0))
        ws2 = makeWorkspace(*cf.getSpectrum(1))

        params = {'ion0.B20': 0.37737, 'ion0.B22': 3.9770, 'ion0.B40': -0.031787, 'ion0.B42': -0.11611,
                  'ion0.B44': -0.12544, 'ion1.B20': 0.37737, 'ion1.B22': 3.9770, 'ion1.B40': -0.031787,
                  'ion1.B42': -0.11611, 'ion1.B44': -0.12544}
        cf = CrystalFieldMultiSite(Ions=['Ce', 'Pr'], Symmetries=['C2v', 'C2v'], Temperatures=[44.0, 50.0],
                                    FWHMs=[1.0, 1.0], ToleranceIntensity=6.0, ToleranceEnergy=1.0,  FixAllPeaks=True,
                                   parameters=params)

        cf.fix('ion0.BmolX', 'ion0.BmolY', 'ion0.BmolZ', 'ion0.BextX', 'ion0.BextY', 'ion0.BextZ', 'ion0.B40',
               'ion0.B42', 'ion0.B44', 'ion0.B60', 'ion0.B62', 'ion0.B64', 'ion0.B66', 'ion0.IntensityScaling',
               'ion1.BmolX', 'ion1.BmolY', 'ion1.BmolZ', 'ion1.BextX', 'ion1.BextY', 'ion1.BextZ', 'ion1.B40',
               'ion1.B42', 'ion1.B44', 'ion1.B60', 'ion1.B62', 'ion1.B64', 'ion1.B66', 'ion1.IntensityScaling',
               'sp0.IntensityScaling', 'sp1.IntensityScaling')

        chi2 = CalculateChiSquared(str(cf.function), InputWorkspace=ws1, InputWorkspace_1=ws2)[1]

        fit = CrystalFieldFit(Model=cf, InputWorkspace=[ws1, ws2], MaxIterations=10)
        fit.fit()

        self.assertTrue(cf.chi2 > 0.0)
        self.assertTrue(cf.chi2 < chi2)

    def test_set_background(self):
        cf = CrystalFieldMultiSite(Ions='Ce', Symmetries='C2v', Temperatures=[20], FWHMs=[1.0], Background='name=LinearBackground,A0=1')
        self.assertEquals('"name=LinearBackground,A0=1"', cf['Background'])
        self.assertEquals(cf.background.param['A0'], 1)
        self.assertEquals(cf.background.background.param['A0'], 1)
        cf.background.param['A0'] = 0
        self.assertEquals(cf.background.background.param['A0'], 0)

    def test_set_background_as_function(self):
        from mantid.fitfunctions import LinearBackground
        cf = CrystalFieldMultiSite(Ions='Ce', Symmetries='C2v', Temperatures=[20], FWHMs=[1.0],
                                   Background=LinearBackground(A0=1))
        self.assertEquals('"name=LinearBackground,A0=1,A1=0"', cf['Background'])
        self.assertEquals(cf.background.param['A0'], 1)
        self.assertEquals(cf.background.background.param['A0'], 1)
        cf.background.param['A0'] = 0
        self.assertEquals(cf.background.background.param['A0'], 0)

    def test_set_background_with_peak(self):
        from mantid.fitfunctions import Gaussian
        cf = CrystalFieldMultiSite(Ions='Ce', Symmetries='C2v', Temperatures=[20], FWHMs=[1.0], Background='name=LinearBackground', BackgroundPeak=Gaussian(Height=1))
        self.assertEquals('"name=Gaussian,Height=1,PeakCentre=0,Sigma=0;name=LinearBackground"', cf['Background'])
        self.assertEquals(cf.background.peak.param['Height'], 1)
        self.assertEquals(cf.background.param['f0.Height'], 1)
        self.assertEquals(cf.background.background.param['A0'], 0)
        cf.background.peak.param['Height'] = 0
        cf.background.background.param['A0'] = 1
        self.assertEquals(cf.background.peak.param['Height'], 0)
        self.assertEquals(cf.background.background.param['A0'], 1)

    def test_set_background_peak_only(self):
        from mantid.fitfunctions import Gaussian
        cf = CrystalFieldMultiSite(Ions='Ce', Symmetries='C2v', Temperatures=[20], FWHMs=[1.0], BackgroundPeak=Gaussian(Sigma=1))
        self.assertEquals('"name=Gaussian,Height=0,PeakCentre=0,Sigma=1"', cf['Background'])
        self.assertEquals(cf.background.peak.param['Sigma'], 1)
        self.assertEquals(cf.background.param['Sigma'], 1)
        cf.background.peak.param['Sigma'] = 0
        self.assertEquals(cf.background.param['Sigma'], 0)

    def test_set_background_composite(self):
        from mantid.fitfunctions import Gaussian, LinearBackground
        cf = CrystalFieldMultiSite(Ions='Ce', Symmetries='C2v', Temperatures=[20], FWHMs=[1.0],
                                   Background= Gaussian(PeakCentre=1) + LinearBackground())
        self.assertEquals('"name=Gaussian,Height=0,PeakCentre=1,Sigma=0;name=LinearBackground,A0=0,A1=0"', cf['Background'])
        cf.background.param['f1.A0'] = 1
        cf.background.param['f0.PeakCentre'] = 0.5
        self.assertEquals(cf.background.param['f1.A0'], 1)
        self.assertEquals(cf.background.param['f0.PeakCentre'], 0.5)

    def test_set_background_composite_as_string(self):
        cf = CrystalFieldMultiSite(Ions='Ce', Symmetries='C2v', Temperatures=[20], FWHMs=[1.0],
                                   Background='name=Gaussian,Height=0,PeakCentre=1,Sigma=0;name=LinearBackground,A0=0,A1=0')
        self.assertEquals('"name=Gaussian,Height=0,PeakCentre=1,Sigma=0;name=LinearBackground,A0=0,A1=0"', cf['Background'])
        self.assertEquals(cf.background.param['f0.Sigma'], 0)
        cf.background.param['f1.A0'] = 1
        self.assertEquals(cf.background.param['f1.A0'], 1)

    def test_constraints_single_spectrum(self):
        from CrystalField.CrystalFieldMultiSite import CrystalFieldMultiSite
        from mantid.fitfunctions import Gaussian, LinearBackground
        from mantid.simpleapi import FunctionFactory

        cf = CrystalFieldMultiSite(Ions=['Ce'], Symmetries=['C2v'], Temperatures=[50], FWHMs=[0.9],
                                   B20=0.37737, B22=3.9770, B40=-0.031787, B42=-0.11611, B44=-0.12544,
                                   Background=LinearBackground(A0=1.0), BackgroundPeak=Gaussian(Height=10, Sigma=0.3))

        cf.ties(B40='B20/2')
        cf.constraints('IntensityScaling > 0', 'B22 < 4')
        cf.constraints('pk0.FWHM < 2.2', 'pk1.FWHM >= 0.1')
        cf.ties(**{'pk2.FWHM': '2*pk1.FWHM', 'pk3.FWHM': '2*pk2.FWHM'})
        cf.background.peak.ties(Height=10.1)
        cf.background.peak.constraints('Sigma > 0')
        cf.background.background.ties(A0=0.1)
        cf.background.background.constraints('A1 > 0')

        s = cf.makeSpectrumFunction()
        self.assertTrue('0<IntensityScaling' in s)
        self.assertTrue('B22<4' in s)
        self.assertTrue('0<bg.f0.Sigma' in s)
        self.assertTrue('0<bg.f1.A1' in s)
        self.assertTrue('Height=10.1' in s)
        self.assertTrue('A0=0.1' in s)
        self.assertTrue('pk0.FWHM<2.2' in s)
        self.assertTrue('0.1<pk1.FWHM' in s)
        self.assertTrue('pk2.FWHM=2*pk1.FWHM' in s)
        self.assertTrue('pk3.FWHM=2*pk2.FWHM' in s)

        # Test that ties and constraints are correctly defined
        fun = FunctionFactory.createInitialized(s)
        self.assertTrue(fun is not None)

    # def test_constraints_multi_spectrum(self):
    #     from CrystalField import CrystalField, CrystalFieldFit, Background, Function
    #     from mantid.fitfunctions import FlatBackground, Gaussian
    #     from mantid.simpleapi import FunctionFactory
    #
    #     # cf = CrystalField('Ce', 'C2v', B20=0.37737, B22=3.9770, B40=-0.031787, B42=-0.11611, B44=-0.12544,
    #     #                   Temperature=[44.0, 50], FWHM=[1.1, 0.9])
    #     # cf.PeakShape = 'Lorentzian'
    #     # cf.background = Background(peak=Function('Gaussian', Height=10, Sigma=0.3),
    #     #                  background=Function('FlatBackground', A0=1.0))
    #
    #     cf = CrystalFieldMultiSite(Ions=['Ce'], Symmetries=['C2v'], Temperatures=[44, 50], FWHMs=[1.1, 0.9],
    #                                Background=FlatBackground(), BackgroundPeak=Gaussian(Height=10, Sigma=0.3),
    #                                B20=0.37737, B22=3.9770, B40=-0.031787, B42=-0.11611, B44=-0.12544)
    #
    #     # cf.constraints('IntensityScaling0 > 0', '0 < IntensityScaling1 < 2', 'B22 < 4')
    #
    #     for x in range(cf.function.numParams()):
    #         print cf.function.parameterName(x)
    #
    #     cf.background[0].peak.ties(Height=10.1)
    #     cf.background[0].peak.constraints('Sigma > 0.1')
    #     cf.background[1].peak.ties(Height=20.2)
    #     cf.background[1].peak.constraints('Sigma > 0.2')
    #
    #     cf.peaks[1].ties({'f2.FWHM': '2*f1.FWHM', 'f3.FWHM': '2*f2.FWHM'})
    #     cf.peaks[0].constraints('f1.FWHM < 2.2')
    #     cf.peaks[1].constraints('f1.FWHM > 1.1', '1 < f4.FWHM < 2.2')
    #
    #     s = cf.makeMultiSpectrumFunction()
    #
    #     self.assertTrue('0<IntensityScaling0' in s)
    #     self.assertTrue('IntensityScaling1<2' in s)
    #     self.assertTrue('f0.f0.f0.Height=10.1' in s)
    #     self.assertTrue('f1.f0.f0.Height=20.2' in s)
    #     self.assertTrue('0.1<f0.f0.f0.Sigma' in s)
    #     self.assertTrue('0.2<f1.f0.f0.Sigma' in s)
    #     self.assertTrue('f0.f1.FWHM<2.2' in s)
    #     self.assertTrue('1.1<f1.f1.FWHM' in s)
    #     self.assertTrue('1<f1.f4.FWHM<2.2' in s)
    #     self.assertTrue('f1.f2.FWHM=2*f1.f1.FWHM' in s)
    #     self.assertTrue('f1.f3.FWHM=2*f1.f2.FWHM' in s)
    #
    #     fun = FunctionFactory.createInitialized(s)