#pylint: disable=too-many-public-methods,invalid-name

import unittest
from mantid import logger
from mantid.simpleapi import (SimulatedDensityOfStates, CheckWorkspacesMatch,
                              Scale, CreateEmptyTableWorkspace)


def scipy_not_available():
    ''' Check whether scipy is available on this platform'''
    try:
        import scipy
        return False
    except ImportError:
        logger.warning("Skipping SimulatedDensityOfStatesTest because scipy is unavailable.")
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
class SimulatedDensityOfStatesTest(unittest.TestCase):

    def setUp(self):
        self._phonon_file = 'squaricn.phonon'
        self._castep_file = 'squaricn.castep'

    def test_phonon_load(self):
        wks = SimulatedDensityOfStates(PHONONFile=self._phonon_file)
        self.assertEquals(wks.getNumberHistograms(), 1)

    def test_castep_load(self):
        wks = SimulatedDensityOfStates(CASTEPFile='squaricn.castep')
        self.assertEquals(wks.getNumberHistograms(), 1)

    def test_raman_active(self):
        spec_type = 'Raman_Active'
        wks = SimulatedDensityOfStates(PHONONFile=self._phonon_file,
                                       SpectrumType=spec_type)
        self.assertEquals(wks.getNumberHistograms(), 1)

    def test_ir_active(self):
        spec_type = 'IR_Active'
        wks = SimulatedDensityOfStates(PHONONFile=self._phonon_file,
                                       SpectrumType=spec_type)
        self.assertEquals(wks.getNumberHistograms(), 1)

    def test_lorentzian_function(self):
        wks = SimulatedDensityOfStates(PHONONFile=self._phonon_file,
                                       Function='Lorentzian')
        self.assertEquals(wks.getNumberHistograms(), 1)

    def test_peak_width(self):
        wks = SimulatedDensityOfStates(PHONONFile=self._phonon_file,
                                       PeakWidth='0.3')
        self.assertEquals(wks.getNumberHistograms(), 1)

    def test_peak_width_function(self):
        wks = SimulatedDensityOfStates(PHONONFile=self._phonon_file,
                                       PeakWidth='0.1*energy')
        self.assertEquals(wks.getNumberHistograms(), 1)

    def test_temperature(self):
        wks = SimulatedDensityOfStates(PHONONFile=self._phonon_file,
                                       Temperature=50)
        self.assertEquals(wks.getNumberHistograms(), 1)

    def test_scale(self):
        wks = SimulatedDensityOfStates(PHONONFile=self._phonon_file,
                                       Scale=10)
        ref = SimulatedDensityOfStates(PHONONFile=self._phonon_file)
        ref = Scale(ref, Factor=10)

        self.assertEqual(CheckWorkspacesMatch(wks, ref), 'Success!')

    def test_bin_width(self):
        import math

        ref = SimulatedDensityOfStates(PHONONFile=self._phonon_file)
        wks = SimulatedDensityOfStates(PHONONFile=self._phonon_file,
                                       BinWidth=2)

        size = wks.blocksize()
        ref_size = ref.blocksize()

        self.assertEquals(size, math.ceil(ref_size/2.0))

    def test_zero_threshold(self):
        import numpy as np

        wks = SimulatedDensityOfStates(PHONONFile=self._phonon_file,
                                       ZeroThreshold=20)

        x_data = wks.readX(0)
        y_data = wks.readY(0)

        mask = np.where(x_data < 20)
        self.assertEquals(sum(y_data[mask]), 0)

    def test_partial(self):
        spec_type = 'DOS'

        wks = SimulatedDensityOfStates(PHONONFile=self._phonon_file,
                                       SpectrumType=spec_type,
                                       Ions='H,C,O')

        workspaces = wks.getNames()
        self.assertEquals(len(workspaces), 3)

    def test_sum_partial_contributions(self):
        spec_type = 'DOS'
        tolerance = 1e-10

        summed = SimulatedDensityOfStates(PHONONFile=self._phonon_file,
                                          SpectrumType=spec_type,
                                          Ions='H,C,O',
                                          SumContributions=True)
        total = SimulatedDensityOfStates(PHONONFile=self._phonon_file,
                                         SpectrumType=spec_type)

        self.assertEquals(CheckWorkspacesMatch(summed, total, tolerance),
                          'Success!')

    def test_partial_cross_section_scale(self):
        spec_type = 'DOS'

        wks = SimulatedDensityOfStates(PHONONFile=self._phonon_file,
                                       SpectrumType=spec_type,
                                       Ions='H,C,O',
                                       ScaleByCrossSection='Incoherent')

        workspaces = wks.getNames()
        self.assertEquals(len(workspaces), 3)

    def test_sum_partial_contributions_cross_section_scale(self):
        spec_type = 'DOS'
        tolerance = 1e-10

        summed = SimulatedDensityOfStates(PHONONFile=self._phonon_file,
                                          SpectrumType=spec_type,
                                          Ions='H,C,O',
                                          SumContributions=True,
                                          ScaleByCrossSection='Incoherent')
        total = SimulatedDensityOfStates(PHONONFile=self._phonon_file,
                                         SpectrumType=spec_type,
                                         ScaleByCrossSection='Incoherent')

        self.assertEquals(CheckWorkspacesMatch(summed, total, tolerance),
                          'Success!')

    def test_ion_table(self):
        wks = SimulatedDensityOfStates(PHONONFile=self._phonon_file,
                                       SpectrumType='IonTable')

        # Build the expected output
        expected = CreateEmptyTableWorkspace()
        expected.addColumn('str', 'Ion')
        expected.addColumn('int', 'Count')
        expected.addRow(['H', 4])
        expected.addRow(['C', 8])
        expected.addRow(['O', 8])

        self.assertEquals(CheckWorkspacesMatch(wks, expected), 'Success!')

    def test_ion_table_castep_error(self):
        """
        Creating an ion table from a castep file is not possible and should fail
        validation.
        """
        self.assertRaises(RuntimeError, SimulatedDensityOfStates,
                          PHONONFile=self._phonon_file,
                          SpectrumType='IonTable')

    def test_bond_analysis_file_error(self):
        """
        Bond analysis requires both a CASTEP and PHONON file.
        """
        self.assertRaises(RuntimeError, SimulatedDensityOfStates,
                          PHONONFile=self._phonon_file,
                          SpectrumType='BondAnalysis')
        self.assertRaises(RuntimeError, SimulatedDensityOfStates,
                          CASTEPFile=self._castep_file,
                          SpectrumType='BondAnalysis')

    def test_peak_width_function_error(self):
        """
        Using an invalid peak width function should raise RuntimeError.
        """
        self.assertRaises(RuntimeError, SimulatedDensityOfStates,
                          PHONONFile=self._phonon_file,
                          PeakWidth='10*')


if __name__=="__main__":
    unittest.main()
