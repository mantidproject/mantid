import unittest
from CrystalField.CrystalFieldMultiSite import CrystalFieldMultiSite


class CrystalFieldMultiSiteTests(unittest.TestCase):

    def test_init_single_ion(self):
        cfms = CrystalFieldMultiSite(Ions='Pm', Symmetries='D2', Temperatures=[20,52], FWHMs=[1.0, 1.0])
        self.assertEqual(cfms.Ions, ['Pm'])
        self.assertEqual(cfms.Symmetries, ['D2'])
        self.assertEqual(cfms.Temperatures, [20,52])
        self.assertEqual(cfms.FWHMs, [1.0, 1.0])

    def test_init_multiple_ions(self):
        cfms = CrystalFieldMultiSite(Ions=('Pm', 'Eu'), Symmetries=('D2', 'C3v'), Temperatures=[20,52], FWHMs=[1.0, 1.0])
        self.assertEqual(cfms.Ions, ['Pm', 'Eu'])
        self.assertEqual(cfms.Symmetries, ['D2', 'C3v'])
        self.assertEqual(cfms.Temperatures, [20,52])
        self.assertEqual(cfms.FWHMs, [1.0, 1.0])

    def test_toleranceEnergy_property(self):
        cfms = CrystalFieldMultiSite(('Pm', 'Eu'), ('D2', 'C3v'), ToleranceEnergy=1.0)
        self.assertEqual(cfms.ToleranceEnergy, 1.0)

    def test_toleranceIntensity_property(self):
        cfms = CrystalFieldMultiSite(('Pm', 'Eu'), ('D2', 'C3v'), ToleranceIntensity=6.0)
        self.assertEqual(cfms.ToleranceIntensity, 6.0)

    def test_NPeaks_property(self):
        cfms = CrystalFieldMultiSite(('Pm', 'Eu'), ('D2', 'C3v'), NPeaks=11)
        self.assertEqual(cfms.NPeaks, 11)

    def test_fixAllPeaks_property(self):
        cfms = CrystalFieldMultiSite(('Pm', 'Eu'), ('D2', 'C3v'), FixAllPeaks=True)
        self.assertTrue(cfms.FixAllPeaks)

    def test_peakShape_property(self):
        cfms = CrystalFieldMultiSite(('Pm', 'Eu'), ('D2', 'C3v'), PeakShape='Gaussian')
        self.assertEqual(cfms.PeakShape, 'Gaussian')

    def test_FWHM_variation_property(self):
        cfms = CrystalFieldMultiSite(('Pm', 'Eu'), ('D2', 'C3v'), FWHMVariation=0.3)
        self.assertEqual(cfms.FWHMVariation, 0.3)

    def test_init_parameters(self):
        cfms = CrystalFieldMultiSite(Ions='Ce', Symmetries='C2', Temperatures=[15], FWHMs=[1.0],
                                     parameters={'BmolX': 1.0, 'B40':-0.02})
        self.assertEqual(cfms.function.getParameterValue('BmolX'), 1.0)
        self.assertEqual(cfms.function.getParameterValue('B40'), -0.02)

    def test_init_parameters_multiple_ions(self):
        cfms = CrystalFieldMultiSite(Ions=('Pm', 'Eu'), Symmetries=('D2', 'C3v'), Temperatures=[20, 52],
                                     FWHMs=[1.0, 1.0], parameters={'ion0.B40': -0.02, 'ion0.B42': -0.11,
                                                                   'ion1.B42': -0.12})
        self.assertEqual(cfms.function.getParameterValue('ion0.B42'), -0.11)
        self.assertEqual(cfms.function.getParameterValue('ion0.B40'), -0.02)
        self.assertEqual(cfms.function.getParameterValue('ion1.B42'), -0.12)

    def test_peak_values(self):
        cfms = CrystalFieldMultiSite(Ions='Ce', Symmetries='C2', Temperatures=[25], FWHMs=[1.5],
                                     BmolX=1.0, B40=-0.02)
        self.assertEqual(int(cfms.function.getParameterValue('pk0.Amplitude')), 310)
        self.assertEqual(cfms.function.getParameterValue('pk0.FWHM'), 1.5)
        self.assertEqual(cfms.function.getParameterValue('pk0.PeakCentre'), 0)

    def test_peak_values_multiple_ions(self):
        cfms = CrystalFieldMultiSite(Ions=('Pr', 'Nd'), Symmetries=('D2', 'C3v'), Temperatures=[20],
                                     FWHMs=[1.5], parameters={'ion0.B60': -0.02, 'ion0.B62': -0.11,
                                                                   'ion1.B62': -0.12, 'ion1.B64': -0.3})
        self.assertEqual(int(cfms.function.getParameterValue('ion0.pk0.Amplitude')), 618)
        self.assertEqual(cfms.function.getParameterValue('ion0.pk0.FWHM'), 1.5)
        self.assertEqual(cfms.function.getParameterValue('ion0.pk1.PeakCentre'), 0)
        self.assertEqual(int(cfms.function.getParameterValue('ion1.pk0.Amplitude')), 632)
        self.assertEqual(cfms.function.getParameterValue('ion1.pk0.FWHM'), 1.5)
        self.assertEqual(cfms.function.getParameterValue('ion1.pk1.PeakCentre'), 0)

    def test_peak_values_multiple_ions_and_spectra(self):
        cfms = CrystalFieldMultiSite(Ions=('Pm', 'Ce'), Symmetries=('D2', 'C3v'), Temperatures=[20, 52],
                                     FWHMs=[1.0, 1.0], parameters={'ion0.B40': -0.02, 'ion0.B42': -0.11,
                                                                   'ion1.B42': -0.12, 'ion1.B44': -0.3})
        self.assertEqual(int(cfms.function.getParameterValue('ion0.sp0.pk0.Amplitude')), 347)
        self.assertEqual(cfms.function.getParameterValue('ion0.sp1.pk0.FWHM'), 1.0)
        self.assertEqual(cfms.function.getParameterValue('ion0.sp0.pk1.PeakCentre'), 0)
        self.assertEqual(int(cfms.function.getParameterValue('ion1.sp1.pk0.Amplitude')), 310)
        self.assertEqual(cfms.function.getParameterValue('ion1.sp1.pk0.FWHM'), 1.0)
        self.assertEqual(cfms.function.getParameterValue('ion1.sp0.pk1.PeakCentre'), 0)
