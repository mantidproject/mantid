# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
DNS script helpers for elastic powder reduction
"""
import unittest
from unittest.mock import patch
from unittest.mock import call

from mantidqtinterfaces.dns_powder_elastic.scripts.md_powder_elastic import \
    background_substraction, fliping_ratio_correction, \
    load_all, load_binned, raise_error, vanadium_correction

from mantidqtinterfaces.dns_powder_tof.helpers.helpers_for_testing import \
    get_fake_elastic_datadic, get_fake_MD_workspace_unique
from mantidqtinterfaces.dns_powder_tof.data_structures.dns_error import \
    DNSError

from mantid.simpleapi import mtd
from mantid.api import IMDHistoWorkspace


class MDPowderElasticTest(unittest.TestCase):

    def setUp(self):
        mtd.clear()

    def test_background_substraction(self):
        get_fake_MD_workspace_unique(name='vana_x_sf', factor=5.2)
        get_fake_MD_workspace_unique(name='vana_x_sf_norm', factor=0.7)
        testv = background_substraction('empty_x_sf')
        self.assertIsNone(testv)
        with self.assertRaises(DNSError) as context:
            background_substraction('vana_x_sf')
        self.assertTrue('No background file ' in str(context.exception))

        get_fake_MD_workspace_unique(name='empty_x_sf', factor=2)
        get_fake_MD_workspace_unique(name='empty_x_sf_norm', factor=1)
        testv = background_substraction('vana_x_sf')
        self.assertIsInstance(testv, IMDHistoWorkspace)
        value = testv.getSignalArray()
        error = testv.getErrorSquaredArray()
        self.assertEqual(value.shape, (5, 5, 1))
        self.assertAlmostEqual(value[0, 0, 0], 236.44444444444446)
        self.assertAlmostEqual(error[0, 0, 0], 620.9163237311386)

    def test_fliping_ratio_correction(self):
        get_fake_MD_workspace_unique(name='vana_x_sf', factor=5.2)
        get_fake_MD_workspace_unique(name='vana_x_sf_norm', factor=0.7)
        testv = fliping_ratio_correction('vana_x_nsf')
        self.assertFalse(testv)
        with self.assertRaises(DNSError) as context:
            fliping_ratio_correction('vana_x_sf')
        self.assertTrue('no matching nsf workspace found '
                        'for' in str(context.exception))

        get_fake_MD_workspace_unique(name='vana_x_nsf', factor=100)
        get_fake_MD_workspace_unique(name='vana_x_nsf_norm', factor=1)
        with self.assertRaises(DNSError) as context:
            fliping_ratio_correction('vana_x_sf')
        self.assertTrue('no matching NiCr workspace found '
                        'for ' in str(context.exception))

        get_fake_MD_workspace_unique(name='nicr_x_sf', factor=200)
        get_fake_MD_workspace_unique(name='nicr_x_sf_norm', factor=1)
        get_fake_MD_workspace_unique(name='nicr_x_nsf', factor=2)
        get_fake_MD_workspace_unique(name='nicr_x_nsf_norm', factor=1)
        testv = fliping_ratio_correction('vana_x_sf')
        self.assertTrue(testv)
        testv = mtd['vana_x_sf']
        self.assertIsInstance(testv, IMDHistoWorkspace)
        value = testv.getSignalArray()
        error = testv.getErrorSquaredArray()
        self.assertEqual(value.shape, (5, 5, 1))
        self.assertAlmostEqual(value[0, 0, 0], 4261.127439724454)
        self.assertAlmostEqual(error[0, 0, 0], 445089.2989742403)
        testv = mtd['vana_x_nsf']
        value = testv.getSignalArray()
        error = testv.getErrorSquaredArray()
        self.assertEqual(value.shape, (5, 5, 1))
        self.assertAlmostEqual(value[0, 0, 0], 272.87256027554577)
        self.assertAlmostEqual(error[0, 0, 0], 448979.2989742403)

    @staticmethod
    @patch('mantidqtinterfaces.dns_powder_elastic.scripts.'
           'md_powder_elastic.load_binned')
    def test_load_all(mock_load_binned):
        data_dict = get_fake_elastic_datadic()
        load_all(data_dict, [0, 1, 2], normalizeto='monitor')
        calls = [call('knso_x_nsf', [0, 1, 2], 'C:/data',
                      range(554574, 554634, 6), 'monitor'),
                 call('knso_x_sf', [0, 1, 2], 'C:/data',
                      range(554573, 554633, 6), 'monitor'),
                 call('knso_y_nsf', [0, 1, 2], 'C:/data',
                      range(554576, 554636, 6), 'monitor'),
                 call('knso_y_sf', [0, 1, 2], 'C:/data',
                      range(554575, 554635, 6), 'monitor'),
                 call('knso_z_nsf', [0, 1, 2], 'C:/data',
                      range(554578, 554638, 6), 'monitor'),
                 call('knso_z_sf', [0, 1, 2], 'C:/data',
                      range(554577, 554637, 6), 'monitor')]
        mock_load_binned.assert_has_calls(calls)

    @patch('mantidqtinterfaces.dns_powder_elastic.scripts.'
           'md_powder_elastic.mtd')
    @patch('mantidqtinterfaces.dns_powder_elastic.scripts.'
           'md_powder_elastic.BinMD')
    @patch('mantidqtinterfaces.dns_powder_elastic.scripts.'
           'md_powder_elastic.LoadDNSSCD')
    def test_load_binned(self, mock_load, mock_binmd, mock_mtd):
        testv = load_binned('knso_x_nsf', [0, 1, 2], 'C:/data',
                            range(554574, 554578, 2), 'monitor')
        mock_load.assert_called_once_with(
            'C:/data_554574.d_dat,'
            ' C:/data_554576.d_dat',
            OutputWorkspace='knso_x_nsf',
            NormalizationWorkspace='knso_x_nsf_norm', Normalization='monitor',
            LoadAs='raw', TwoThetaLimits='0,1')
        calls = [
            call(InputWorkspace='knso_x_nsf', OutputWorkspace='knso_x_nsf',
                 AxisAligned=True,
                 AlignedDim0='Theta,0.0,0.5,2',
                 AlignedDim1='Omega,0.0,359.0,1',
                 AlignedDim2='TOF,424.0,2000.0,1'),
            call(InputWorkspace='knso_x_nsf_norm',
                 OutputWorkspace='knso_x_nsf_norm', AxisAligned=True,
                 AlignedDim0='Theta,0.0,0.5,2',
                 AlignedDim1='Omega,0.0,359.0,1',
                 AlignedDim2='TOF,424.0,2000.0,1')]
        mock_binmd.assert_has_calls(calls)
        mock_mtd.__getitem__.assert_called_once_with('knso_x_nsf')
        self.assertEqual(testv, mock_mtd.__getitem__.return_value)

    def test_raise_error(self):
        with self.assertRaises(DNSError):
            raise_error('123')

    def test_vanadium_correction_1(self):
        get_fake_MD_workspace_unique(name='sample_x_sf', factor=5.2)
        get_fake_MD_workspace_unique(name='sample_x_sf_norm', factor=0.7)
        with self.assertRaises(DNSError) as context:
            vanadium_correction('sample_x_sf',
                                vanaset=None,
                                ignore_vana_fields=False,
                                sum_vana_sf_nsf=False)
        self.assertTrue('No vanadium file for' in str(context.exception))
        get_fake_MD_workspace_unique(name='vana_x_sf', factor=1.3)
        get_fake_MD_workspace_unique(name='vana_x_sf_norm', factor=1.1)
        testv = vanadium_correction('sample_x_sf',
                                    vanaset=None,
                                    ignore_vana_fields=False,
                                    sum_vana_sf_nsf=False)
        self.assertIsInstance(testv, IMDHistoWorkspace)
        testv = mtd['sample_x_sf']
        value = testv.getSignalArray()
        error = testv.getErrorSquaredArray()
        self.assertEqual(value.shape, (5, 5, 1))
        self.assertAlmostEqual(value[0, 0, 0], 5.373219373219373)
        self.assertAlmostEqual(error[0, 0, 0], 1.5231536880603584)
        testv = mtd['sample_x_sf_norm']
        value = testv.getSignalArray()
        error = testv.getErrorSquaredArray()
        self.assertEqual(value.shape, (5, 5, 1))
        self.assertAlmostEqual(value[0, 0, 0], 59.92682926829268)
        self.assertAlmostEqual(error[0, 0, 0], 178.30697465213797)

    def test_vanadium_correction_2(self):
        with self.assertRaises(DNSError) as context:
            vanadium_correction('sample_x_sf',
                                vanaset=None,
                                ignore_vana_fields=True,
                                sum_vana_sf_nsf=False)
        self.assertTrue('Need to give vanadium' in str(context.exception))
        with self.assertRaises(DNSError) as context:
            vanadium_correction('sample_x_sf',
                                vanaset=['x_sf, x_nsf'],
                                ignore_vana_fields=True,
                                sum_vana_sf_nsf=False)
        self.assertTrue('No vanadium file for ' in str(context.exception))
        get_fake_MD_workspace_unique(name='sample_x_sf', factor=5.2)
        get_fake_MD_workspace_unique(name='sample_x_sf_norm', factor=0.7)
        get_fake_MD_workspace_unique(name='vana_x_sf', factor=1.3)
        get_fake_MD_workspace_unique(name='vana_x_sf_norm', factor=1.1)
        get_fake_MD_workspace_unique(name='vana_x_nsf', factor=30.3)
        get_fake_MD_workspace_unique(name='vana_x_nsf_norm', factor=0.5)
        testv = vanadium_correction('sample_x_sf',
                                    vanaset={'x_sf': 1, 'x_nsf': 2},
                                    ignore_vana_fields=True,
                                    sum_vana_sf_nsf=False)
        self.assertIsInstance(testv, IMDHistoWorkspace)

        testv = mtd['sample_x_sf']
        value = testv.getSignalArray()
        error = testv.getErrorSquaredArray()
        self.assertEqual(value.shape, (5, 5, 1))
        self.assertAlmostEqual(value[0, 0, 0], 5.697161388240037)
        self.assertAlmostEqual(error[0, 0, 0], 1.0518834373279087)
        testv = mtd['sample_x_sf_norm']
        value = testv.getSignalArray()
        error = testv.getErrorSquaredArray()
        self.assertEqual(value.shape, (5, 5, 1))
        self.assertAlmostEqual(value[0, 0, 0], 56.51937483545152)
        self.assertAlmostEqual(error[0, 0, 0], 93.60438736358742)

    def test_vanadium_correction_3(self):
        with self.assertRaises(DNSError) as context:
            vanadium_correction('sample_x_sf',
                                vanaset=None,
                                ignore_vana_fields=False,
                                sum_vana_sf_nsf=True)
        self.assertTrue('No vanadium file for x_sf' in str(context.exception))
        get_fake_MD_workspace_unique(name='vana_x_sf', factor=1.3)
        get_fake_MD_workspace_unique(name='vana_x_sf_norm', factor=1.1)
        with self.assertRaises(DNSError) as context:
            vanadium_correction('sample_x_sf',
                                vanaset=None,
                                ignore_vana_fields=False,
                                sum_vana_sf_nsf=True)
        self.assertTrue('No vanadium file for x_nsf' in str(context.exception))
        get_fake_MD_workspace_unique(name='sample_x_sf', factor=5.2)
        get_fake_MD_workspace_unique(name='sample_x_sf_norm', factor=0.7)
        get_fake_MD_workspace_unique(name='vana_x_nsf', factor=30.3)
        get_fake_MD_workspace_unique(name='vana_x_nsf_norm', factor=0.5)
        testv = vanadium_correction('sample_x_sf',
                                    vanaset=None,
                                    ignore_vana_fields=False,
                                    sum_vana_sf_nsf=True)
        self.assertIsInstance(testv, IMDHistoWorkspace)
        testv = mtd['sample_x_sf']
        value = testv.getSignalArray()
        error = testv.getErrorSquaredArray()
        self.assertEqual(value.shape, (5, 5, 1))
        self.assertAlmostEqual(value[0, 0, 0], 5.697161388240037)
        self.assertAlmostEqual(error[0, 0, 0], 1.0518834373279087)
        testv = mtd['sample_x_sf_norm']
        value = testv.getSignalArray()
        error = testv.getErrorSquaredArray()
        self.assertEqual(value.shape, (5, 5, 1))
        self.assertAlmostEqual(value[0, 0, 0], 56.51937483545152)
        self.assertAlmostEqual(error[0, 0, 0], 93.60438736358742)


if __name__ == '__main__':
    unittest.main()
