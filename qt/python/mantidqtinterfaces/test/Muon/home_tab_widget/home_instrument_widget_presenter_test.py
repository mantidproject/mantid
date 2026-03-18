# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock

from mantidqtinterfaces.Muon.GUI.Common.home_instrument_widget.home_instrument_widget_presenter import InstrumentWidgetPresenter


class FittingTabModelTest(unittest.TestCase):
    def setUp(self):
        self.model = mock.MagicMock()
        self.view = mock.MagicMock()
        self.presenter = InstrumentWidgetPresenter(self.view, self.model)

    def test_handle_double_pulse_enabled(self):
        self.view.double_pulse_state.return_value = "Double Pulse"

        self.presenter.handle_double_pulse_enabled()

        self.view.double_pulse_edit_enabled.assert_called_once_with(True)
        self.model.set_double_pulse_enabled.assert_called_once_with(True)


if __name__ == "__main__":
    unittest.main(buffer=False, verbosity=2)
