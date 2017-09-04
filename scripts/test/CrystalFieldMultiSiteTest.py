import unittest
from CrystalField import CrystalFieldMultiSite

class CrystalFieldMultiSiteTests(unittest.TestCase):

    def test_ion_property(self):
        cfms = CrystalFieldMultiSite(('Pm', 'Eu'), ('D2', 'C3v'))
        self.assertEqual(cfms.Ions, ('Pm', 'Eu'))

    def test_symmetries_property(self):
        cfms = CrystalFieldMultiSite(('Pm', 'Eu'), ('D2', 'C3v'))
        self.assertEqual(cfms.Symmetries, ('D2', 'C3v'))

    def test_toleranceEnergy_property(self):
        cfms = CrystalFieldMultiSite(('Pm', 'Eu'), ('D2', 'C3v'), ToleranceEnergy=1.0)
        self.assertEqual(cfms.ToleranceEnergy, 1.0)

    def test_toleranceIntensity_property(self):
        cfms = CrystalFieldMultiSite(('Pm', 'Eu'), ('D2', 'C3v'), ToleranceIntensity=6.0)
        self.assertEqual(cfms.ToleranceIntensity, 6.0)

    def test_intensityScaling_property(self):
        cfms = CrystalFieldMultiSite(('Pm', 'Eu'), ('D2', 'C3v'), IntensityScaling0=1.0, IntensityScaling1=1.0)
        self.assertEqual(cfms.ToleranceIntensity, [1.0, 1.0])

    def test_NPeaks_property(self):
        cfms = CrystalFieldMultiSite(('Pm', 'Eu'), ('D2', 'C3v'), NPeaks=11)
        self.assertEqual(cfms.NPeaks, 11)

    def test_temperature_property(self):
        cfms = CrystalFieldMultiSite(('Pm', 'Eu'), ('D2', 'C3v'), Temperature=[44.0, 50.0])
        self.assertEqual(cfms.Temperature, [44.0, 50.0])

    def test_FWHM_property(self):
        cfms = CrystalFieldMultiSite(('Pm', 'Eu'), ('D2', 'C3v'), FWHM=[1.0, 1.0])
        self.assertEqual(cfms.FWHM, [1.0, 1.0])

    def test_ResolutionModel_property(self):
        cfms = CrystalFieldMultiSite(('Pm', 'Eu'), ('D2', 'C3v'), ResolutionModel=([0, 50], [1, 2]))
        self.assertEqual(cfms.ResolutionModel, ([0, 50], [1, 2]))

    def test_fixAllPeaks_property(self):
        cfms = CrystalFieldMultiSite(('Pm', 'Eu'), ('D2', 'C3v'), FixAllPeaks=True)
        self.assertTrue(cfms.FixAllPeaks)

    def test_peakShape_property(self):
        cfms = CrystalFieldMultiSite(('Pm', 'Eu'), ('D2', 'C3v'), peakShape='Lorentzian')
        self.assertEqual(cfms.PeakShape, 'Lorentzian')
