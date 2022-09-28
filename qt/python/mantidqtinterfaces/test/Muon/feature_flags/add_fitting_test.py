# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock
from mantidqtinterfaces.Muon.GUI.Common.features.add_fitting import AddFitting
from mantidqtinterfaces.Muon.GUI.Common.fitting_tab_widget.fitting_tab_widget import FittingTabWidget


class AddFittingTest(unittest.TestCase):

    def setUp(self):
        self.GUI = mock.Mock()
        self.GUI.fitting_tab = mock.MagicMock(autospec=FittingTabWidget)

    def test_get_features_success(self):
        test = {"fit_wizard":1}
        AddFitting(self.GUI, test)
        self.GUI.fitting_tab.show_fit_script_generator.assert_called_once_with()

    def test_get_features_fail(self):
        test = {"fit_wizard":0}
        AddFitting(self.GUI, test)
        self.GUI.fitting_tab.show_fit_script_generator.assert_not_called()


if __name__ == '__main__':
    unittest.main()
