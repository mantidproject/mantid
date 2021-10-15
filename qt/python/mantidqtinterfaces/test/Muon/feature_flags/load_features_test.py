# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock

from mantidqtinterfaces.Muon.GUI.Common.features.load_features import load_features


class LoadFeaturesTest(unittest.TestCase):

    def setUp(self):
        return

    @mock.patch('mantidqtinterfaces.Muon.GUI.Common.features.load_features.get_muon_GUI_config')
    def test_attempt_load(self, config_mock):
        config_mock.return_value = "model_analysis:2 , raw_plots : 0, FFT: 2"
        feature = load_features()
        self.assertEqual(feature, {"model_analysis":2, "raw_plots":0, "FFT":2})

    @mock.patch('mantidqtinterfaces.Muon.GUI.Common.features.load_features.get_muon_GUI_config')
    def test_attempt_invalid_load(self, config_mock):
        config_mock.return_value = "model_analysis:2 , raw_plots 0, FFT: 2"
        feature = load_features()
        self.assertEqual(feature, {})

    @mock.patch('mantidqtinterfaces.Muon.GUI.Common.features.load_features.get_muon_GUI_config')
    def test_attempt_bad_load(self, config_mock):
        config_mock.return_value = ":,dsaf:3,hfsda:gf"
        feature = load_features()
        self.assertEqual(feature, {})


if __name__ == '__main__':
    unittest.main()
