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

    def test_ResolutionModel_property(self):
        cfms = CrystalFieldMultiSite(('Pm', 'Eu'), ('D2', 'C2'), ResolutionModel=[([0, 50], [1, 2]), ([0, 51], [3, 4])])
        self.assertEqual(cfms.ResolutionModel.NumberOfSpectra, 2)

    def test_fixAllPeaks_property(self):
        cfms = CrystalFieldMultiSite(('Pm', 'Eu'), ('D2', 'C3v'), FixAllPeaks=True)
        self.assertTrue(cfms.FixAllPeaks)

    def test_peakShape_property(self):
        cfms = CrystalFieldMultiSite(('Pm', 'Eu'), ('D2', 'C3v'), PeakShape='Gaussian')
        self.assertEqual(cfms.PeakShape, 'Gaussian')

    def test_FWHM_variation_property(self):
        cfms = CrystalFieldMultiSite(('Pm', 'Eu'), ('D2', 'C3v'), FWHMVariation=0.3)
        self.assertEqual(cfms.FWHMVariation, 0.3)

    # def test_init_parameters(self):
    #     cfms = CrystalFieldMultiSite(Ions=('Pm', 'Eu'), Symmetries=('D2', 'C3v'), Temperatures=[20, 52],
    #                                  FWHMs=[1.0, 1.0], parameters={'B40': -0.02, 'B42': -0.11, 'f0.B44': -0.12})

    # def test_physical_properties(self):
    #     from CrystalField import PhysicalProperties
    #     cfms = CrystalFieldMultiSite(Ions=('Pm', 'Eu'), Symmetries=('D2', 'C3v'), Temperatures=[20, 52],
    #                                       FWHMs=[1.0, 1.0], PhysicalProperty=[PhysicalProperties('susc', 'powder'), PhysicalProperties('M(H)', Hdir=[0,1,0])])
