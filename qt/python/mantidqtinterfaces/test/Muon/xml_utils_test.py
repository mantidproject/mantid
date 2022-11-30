# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock
from mantidqtinterfaces.Muon.GUI.Common.utilities.xml_utils import load_grouping_from_XML, save_grouping_to_XML
from mantidqtinterfaces.Muon.GUI.Common.muon_group import MuonGroup
from mantidqtinterfaces.Muon.GUI.Common.muon_pair import MuonPair
from mantid import ConfigService
import os


class FittingTabPresenterTest(unittest.TestCase):
    def test_load_grouping_from_XML_correctly_retrieves_default_name(self):
        directory = ConfigService['instrumentDefinition.directory']
        filename = os.path.join(directory, 'Grouping', 'EMU_Detector_Grouping_LF_96.xml')

        groups, pairs, diffs, description, default = load_grouping_from_XML(filename)

        self.assertEqual(description, 'emu longitudinal (96 detectors)')
        self.assertEqual(default, 'long')

    def test_load_grouping_copes_with_no_default_name_specified(self):
        directory = ConfigService['instrumentDefinition.directory']
        filename = os.path.join(directory, 'Grouping', 'VISION_Grouping.xml')

        groups, diffs, pairs, description, default = load_grouping_from_XML(filename)

        self.assertEqual(description, filename)
        self.assertEqual(default, '')

    @mock.patch('mantidqtinterfaces.Muon.GUI.Common.utilities.xml_utils.ET.parse')
    def test_that_save_and_load_grouping_xml_correctly_stores_and_reads_period_data(self, mock_file_parse):
        groups = [MuonGroup('fwd', [1,2,3], [1,3]), MuonGroup('bwd', [4,5,6], [2,4])]
        pairs = [MuonPair('long', 'fwd', 'bwd')]
        diffs = []
        xml_tree = save_grouping_to_XML(groups, pairs, diffs, 'filename.xml', save=False, description='Bespoke grouping')
        mock_file_parse.return_value = xml_tree

        loaded_groups, loaded_pairs, diffs, loaded_description, loaded_default = load_grouping_from_XML('filename.xml')

        self.assertEqual(loaded_groups[0].periods, groups[0].periods)
        self.assertEqual(loaded_groups[1].periods, groups[1].periods)


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
