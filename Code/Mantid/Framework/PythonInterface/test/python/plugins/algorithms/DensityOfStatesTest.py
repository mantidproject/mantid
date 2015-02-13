import unittest
from mantid import logger
from mantid.simpleapi import DensityOfStates, CheckWorkspacesMatch, Scale, CreateEmptyTableWorkspace


def scipy_not_available():
    ''' Check whether scipy is available on this platform'''
    try:
        import scipy
        return False
    except:
        logger.warning("Skipping DensityOfStatesTest because scipy is unavailable.")
        return True


def skip_if(skipping_criteria):
    '''
        Skip all tests if the supplied functon returns true.
        Python unittest.skipIf is not available in 2.6 (RHEL6) so we'll roll our own.
    '''
    def decorate(cls):
        if skipping_criteria():
            for attr in cls.__dict__.keys():
                if callable(getattr(cls, attr)) and 'test' in attr:
                    delattr(cls, attr)
        return cls
    return decorate


@skip_if(scipy_not_available)
class DensityOfStatesTest(unittest.TestCase):

    def setUp(self):
        self._file_name = 'squaricn.phonon'

    def test_phonon_load(self):
        ws = DensityOfStates(File=self._file_name)
        self.assertEquals(ws.getNumberHistograms(), 1)

    def test_castep_load(self):
        ws = DensityOfStates(File='squaricn.castep')
        self.assertEquals(ws.getNumberHistograms(), 1)

    def test_raman_active(self):
        spec_type = 'Raman_Active'
        ws = DensityOfStates(File=self._file_name, SpectrumType=spec_type)
        self.assertEquals(ws.getNumberHistograms(), 1)

    def test_ir_active(self):
        spec_type = 'IR_Active'
        ws = DensityOfStates(File=self._file_name, SpectrumType=spec_type)
        self.assertEquals(ws.getNumberHistograms(), 1)

    def test_lorentzian_function(self):
        ws = DensityOfStates(File=self._file_name, Function='Lorentzian')
        self.assertEquals(ws.getNumberHistograms(), 1)

    def test_peak_width(self):
        ws = DensityOfStates(File=self._file_name, PeakWidth=0.3)
        self.assertEquals(ws.getNumberHistograms(), 1)

    def test_temperature(self):
        ws = DensityOfStates(File=self._file_name, Temperature=50)
        self.assertEquals(ws.getNumberHistograms(), 1)

    def test_scale(self):
        ws = DensityOfStates(File=self._file_name, Scale=10)
        ref = DensityOfStates(File=self._file_name)
        ref = Scale(ref, Factor=10)

        self.assertEqual(CheckWorkspacesMatch(ws, ref), 'Success!')

    def test_bin_width(self):
        import math

        ref = DensityOfStates(File=self._file_name)
        ws = DensityOfStates(File=self._file_name, BinWidth=2)

        size = ws.blocksize()
        ref_size = ref.blocksize()

        self.assertEquals(size, math.ceil(ref_size/2.0))

    def test_zero_threshold(self):
        import numpy as np

        ws = DensityOfStates(File=self._file_name, ZeroThreshold=20)

        x = ws.readX(0)
        y = ws.readY(0)

        mask = np.where(x < 20)
        self.assertEquals(sum(y[mask]), 0)

    def test_partial(self):
        spec_type = 'DOS'

        ws = DensityOfStates(File=self._file_name, SpectrumType=spec_type, Ions='H,C,O')

        workspaces = ws.getNames()
        self.assertEquals(len(workspaces), 3)

    def test_sum_partial_contributions(self):
        spec_type = 'DOS'
        tolerance = 1e-10

        summed = DensityOfStates(File=self._file_name, SpectrumType=spec_type, Ions='H,C,O', SumContributions=True)
        total = DensityOfStates(File=self._file_name,  SpectrumType=spec_type)

        self.assertEquals(CheckWorkspacesMatch(summed, total, tolerance), 'Success!')

    def test_partial_cross_section_scale(self):
        spec_type = 'DOS'

        ws = DensityOfStates(File=self._file_name, SpectrumType=spec_type, Ions='H,C,O', ScaleByCrossSection='Incoherent')

        workspaces = ws.getNames()
        self.assertEquals(len(workspaces), 3)

    def test_sum_partial_contributions_cross_section_scale(self):
        spec_type = 'DOS'
        tolerance = 1e-10

        summed = DensityOfStates(File=self._file_name, SpectrumType=spec_type, Ions='H,C,O', SumContributions=True, ScaleByCrossSection='Incoherent')
        total = DensityOfStates(File=self._file_name,  SpectrumType=spec_type, ScaleByCrossSection='Incoherent')

        self.assertEquals(CheckWorkspacesMatch(summed, total, tolerance), 'Success!')

    def test_ion_table(self):
        ws = DensityOfStates(File=self._file_name, SpectrumType='IonTable')

        # Build the expected output
        expected = CreateEmptyTableWorkspace()
        expected.addColumn('str', 'Ion')
        expected.addColumn('int', 'Count')
        expected.addRow(['H', 4])
        expected.addRow(['C', 8])
        expected.addRow(['O', 8])

        self.assertEquals(CheckWorkspacesMatch(ws, expected), 'Success!')

    def test_ion_table_castep_error(self):
        """
        Creating an ion table from a castep file is not possible and should fail validation.
        """
        self.assertRaises(RuntimeError, DensityOfStates,
                          File=self._file_name, SpectrumType='IonTable')


if __name__=="__main__":
    unittest.main()
