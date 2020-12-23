# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest.mock import Mock
from Engineering.gui.engineering_diffraction.presenter import EngineeringDiffractionPresenter
from Engineering.gui.engineering_diffraction.engineering_diffraction_io import EngineeringDiffractionDecoder, \
    EngineeringDiffractionEncoder


class EngineeringDiffractionIOTest(unittest.TestCase):

    def setUp(self):
        self.mock_view = Mock()
        self.presenter = EngineeringDiffractionPresenter(self.mock_view)
        self.mock_view.presenter.return_value = self.presenter
        self.encoder = EngineeringDiffractionEncoder()
        self.decoder = EngineeringDiffractionDecoder()
        self.io_version = 1

    def testBlankGuiEncodes(self):
        test_dic = self.encoder.encode(self.mock_view)
        self.assertEqual(test_dic, {'encoder_version': self.io_version, 'current_tab': 0})


if __name__ == '__main__':
    unittest.main()
