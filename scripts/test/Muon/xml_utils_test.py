# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from Muon.GUI.Common.utilities.xml_utils import load_grouping_from_XML
from mantid import ConfigService
import os


class FittingTabPresenterTest(unittest.TestCase):
    def test_load_grouping_from_XML_correctly_retrieves_default_name(self):
        directory = ConfigService['instrumentDefinition.directory']
        filename = os.path.join(directory, 'Grouping', 'EMU_Detector_Grouping_LF_96.xml')

        groups, pairs, description, default = load_grouping_from_XML(filename)

        self.assertEquals(description, 'emu longitudinal (96 detectors)')
        self.assertEquals(default, 'long')

    def test_load_grouping_copes_with_no_default_name_specified(self):
        directory = ConfigService['instrumentDefinition.directory']
        filename = os.path.join(directory, 'Grouping', 'VISION_Grouping.xml')

        groups, pairs, description, default = load_grouping_from_XML(filename)

        self.assertEquals(description, filename)
        self.assertEquals(default, '')

if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)