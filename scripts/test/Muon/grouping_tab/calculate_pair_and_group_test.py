# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock
from Muon.GUI.Common.calculate_pair_and_group import _get_MuonGroupingCounts_parameters, \
    _get_MuonGroupingAsymmetry_parameters, _get_MuonPairingAsymmetry_parameters
from Muon.GUI.Common.muon_group import MuonGroup
from Muon.GUI.Common.muon_pair import MuonPair


class TestCalculateMuonGroupPair(unittest.TestCase):
    def test_parameters_correct_for_MuonGroupingCounts(self):
        group = MuonGroup('fwd', [1, 2, 3, 4, 5], [1, 2])
        periods = [1]

        params = _get_MuonGroupingCounts_parameters(group, periods)

        self.assertEquals(params, {'GroupName': 'fwd', 'Grouping': '1,2,3,4,5', 'SummedPeriods': [1]})

    def test_parameters_correct_for_estimate_muon_asymmetry(self):
        group = MuonGroup('fwd', [1, 2, 3, 4, 5], [1, 2])
        periods = [1]
        run = [62260]
        context = mock.MagicMock()
        context.gui_context = {'GroupRangeMin': 0.0, 'GroupRangeMax': 15.0}

        params = _get_MuonGroupingAsymmetry_parameters(context, group, run, periods)

        self.assertEquals(params, {'AsymmetryTimeMax': 15.0,
                                   'AsymmetryTimeMin': 0.0,
                                   'GroupName': 'fwd',
                                   'Grouping': '1,2,3,4,5',
                                   'SummedPeriods': [1]})

    def test_parameters_correct_for_pairing_asymmetry(self):
        pair = MuonPair('long1', 'group_1', 'group2', 1.0)

        params = _get_MuonPairingAsymmetry_parameters(pair, 'group_1_counts', 'group_2_counts')

        self.assertEqual(params, {'Alpha': '1.0',
                                  'InputWorkspace1': 'group_1_counts',
                                  'InputWorkspace2': 'group_2_counts',
                                  'PairName': 'long1',
                                  'SpecifyGroupsManually': False})


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
