# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from unittest.mock import call, patch

from mantidqtinterfaces.dns.scripts.dnstof import (convert_to_d_e,
                                                            get_sqw, load_data,
                                                            pre_load_data)
from mantidqtinterfaces.dns.tests.helpers_for_testing import \
    get_fake_tof_binning


class DNSScriptsTof(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        pass

    @staticmethod
    @patch('mantidqtinterfaces.dns.scripts.dnstof.'
           'CorrectKiKf')
    @patch('mantidqtinterfaces.dns.scripts.dnstof.'
           'ConvertToDistribution')
    @patch('mantidqtinterfaces.dns.scripts.dnstof.'
           'ConvertUnits')
    def test_convert_to_d_e(mock_convertunits, mock_converttodist,
                            mock_correctkikf):
        convert_to_d_e('abc', 2)
        mock_convertunits.assert_called_once_with('abc',
                                                  Target='DeltaE',
                                                  EMode='Direct',
                                                  EFixed=2,
                                                  OutputWorkspace='abc_dE')
        mock_converttodist.assert_called_once_with('abc_dE')
        mock_correctkikf.assert_called_once_with('abc_dE',
                                                 OutputWorkspace='abc_dE_S')

    @staticmethod
    @patch('mantidqtinterfaces.dns.scripts.dnstof.'
           'mtd')
    @patch('mantidqtinterfaces.dns.scripts.dnstof.'
           'BinMD')
    @patch('mantidqtinterfaces.dns.scripts.dnstof.'
           'MergeMD')
    @patch('mantidqtinterfaces.dns.scripts.dnstof.'
           'ConvertToMD')
    @patch('mantidqtinterfaces.dns.scripts.dnstof.'
           'ConvertToMDMinMaxGlobal')
    def test_get_sqw(mock_converttomdminmax, mock_converttomd,
                     mock_mergemd, mock_binmd, mock_mtd):
        mock_converttomdminmax.return_value = [0, 1]
        mock_mtd.__getitem__.return_value.getNumberOfEntries.return_value = 0
        b = get_fake_tof_binning()
        get_sqw('abc', 'ouname', b)
        calls = [
            call.__getitem__('abc'),
            call.__getitem__().__getitem__(0),
            call.__getitem__().getNumberOfEntries()
        ]
        mock_mtd.assert_has_calls(calls)
        mock_converttomdminmax.assert_called_once_with(
            mock_mtd.__getitem__().__getitem__(), '|Q|', 'Direct')
        mock_converttomd.assert_called_once_with(mock_mtd.__getitem__(),
                                                 QDimensions='|Q|',
                                                 dEAnalysisMode='Direct',
                                                 PreprocDetectorsWS="-",
                                                 MinValues=0,
                                                 MaxValues=1,
                                                 OutputWorkspace='gouname_mde')
        mock_mergemd.assert_not_called()
        mock_binmd.assert_called_once_with(
            InputWorkspace='gouname_mde',
            AlignedDim0='|Q|,0.1849336923669811,2.8349336923669815,106',
            AlignedDim1='DeltaE,-3.1409856698897682,3.1215096823206343,86',
            OutputWorkspace='ouname_sqw')

        mock_mtd.__getitem__.return_value.getNumberOfEntries.return_value = 2
        get_sqw('abc', 'ouname', b)
        mock_mergemd.assert_called_once_with('gouname_mde',
                                             OutputWorkspace='ouname_mde')

    @staticmethod
    @patch('mantidqtinterfaces.dns.scripts.dnstof.'
           'pre_load_data')
    @patch('mantidqtinterfaces.dns.scripts.dnstof.'
           'GroupWorkspaces')
    def test_load_data(mock_groupws, mock_preload):
        data = {-5: 1, -6: 3, 'path': 4}
        p = ''
        prefix = 'a'
        load_data(data, prefix, p)
        calls = [
            call(-6, 'a_1', '', {
                -5: 1,
                -6: 3,
                'path': 4
            }),
            call(-5, 'a_2', '', {
                -5: 1,
                -6: 3,
                'path': 4
            })
        ]
        mock_preload.assert_has_calls(calls)
        mock_groupws.assert_called_once_with(['a_1', 'a_2'],
                                             OutputWorkspace='a')

    @patch('mantidqtinterfaces.dns.scripts.dnstof.'
           'DeleteWorkspaces')
    @patch('mantidqtinterfaces.dns.scripts.dnstof.'
           'MergeRuns')
    @patch('mantidqtinterfaces.dns.scripts.dnstof.'
           'LoadDNSLegacy')
    def test_pre_load_data(self, mock_loadleg, mock_merge, mock_delete):
        data = {-5: [1], -6: [3], 'path': 4}
        p = {'wavelength': 4, 'e_channel': 3, 'delete_raw': False}
        prefix = 'a'
        testv = pre_load_data(-5, prefix, p, data)
        mock_loadleg.assert_called_once_with('4_000001.d_dat',
                                             Normalization='no',
                                             ElasticChannel=3,
                                             Wavelength=4,
                                             OutputWorkspace='ws_000001')
        mock_merge.assert_called_once_with(
            ['ws_000001'],
            SampleLogsSum='mon_sum,duration',
            SampleLogsTimeSeries='deterota,T1,T2,Tsp',
            OutputWorkspace='a')
        mock_delete.assert_not_called()
        self.assertEqual(testv, mock_merge.return_value)
        p = {'wavelength': -4, 'e_channel': 3, 'delete_raw': True}
        mock_loadleg.reset_mock()
        pre_load_data(-5, prefix, p, data)
        mock_loadleg.assert_called_once_with('4_000001.d_dat',
                                             Normalization='no',
                                             ElasticChannel=3,
                                             OutputWorkspace='ws_000001')
        mock_delete.assert_called_once_with(['ws_000001'])


if __name__ == '__main__':
    unittest.main()
