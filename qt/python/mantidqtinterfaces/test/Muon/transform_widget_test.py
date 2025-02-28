# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from unittest import mock
from mantidqt.utils.qt.testing import start_qapplication

# need to write tests for new GUI

from mantidqtinterfaces.Muon.GUI.FrequencyDomainAnalysis.FFT.fft_widget import FFTWidget
from mantidqtinterfaces.Muon.GUI.FrequencyDomainAnalysis.MaxEnt.maxent_widget import MaxEntWidget
from mantidqtinterfaces.Muon.GUI.FrequencyDomainAnalysis.Transform import transform_view
from mantidqtinterfaces.Muon.GUI.FrequencyDomainAnalysis.Transform import transform_widget
from mantidqtinterfaces.Muon.GUI.FrequencyDomainAnalysis.TransformSelection import transform_selection_view
from mantidqtinterfaces.Muon.GUI.FrequencyDomainAnalysis.TransformSelection import transform_selection_widget
from mantidqtinterfaces.Muon.GUI.Common.test_helpers.context_setup import setup_context


# pass the call not the object to widget


@start_qapplication
class Transform2Test(unittest.TestCase):
    def setUp(self):
        self.context = setup_context(True)
        # create widget
        self.widget = transform_widget.TransformWidget(self.context, FFTWidget, MaxEntWidget)
        self.widget._maxent._presenter = mock.MagicMock()
        self.widget._fft._presenter = mock.MagicMock()
        # create the view
        self.view = mock.create_autospec(transform_view.TransformView, instance=True)
        self.view.getView = mock.Mock()
        self.view.getMethods = mock.Mock(return_value=["FFT", "MaxEnt"])
        self.view.hideAll = mock.Mock()
        self.view.showMethod = mock.Mock()
        self.view.selection = mock.create_autospec(transform_selection_view.TransformSelectionView, spec_set=True, instance=True)
        self.view.selection.changeMethodSignal = mock.Mock()
        # set the mocked view to the widget
        self.widget.mockWidget(self.view)

    def mock_widgets(self):
        self.widget._selector = mock.create_autospec(transform_selection_widget.TransformSelectionWidget, spec_set=True, instance=True)
        self.widget._maxent = mock.create_autospec(MaxEntWidget, spec_set=True, instance=True)
        self.widget._fft = mock.create_autospec(FFTWidget, spec_set=True, instance=True)

    def test_update_view_from_model(self):
        self.mock_widgets()

        self.widget.update_view_from_model()
        self.widget._maxent.update_view_from_model.assert_called_once_with()
        self.widget._fft.update_view_from_model.assert_called_once_with()

    def test_closeEvent(self):
        event = mock.Mock()
        self.mock_widgets()
        self.widget.closeEvent(event)
        self.widget._maxent.closeEvent.assert_called_once_with(event)
        self.widget._fft.closeEvent.assert_called_once_with(event)
        self.widget._selector.closeEvent.assert_called_once_with(event)

    def test_updateDisplay(self):
        self.widget.updateDisplay(1)
        assert self.view.hideAll.call_count == 1
        self.assertEqual(self.view.showMethod.call_count, 1)

    def test_new_data(self):
        self.widget.handle_new_data_loaded()
        self.assertEqual(self.widget._maxent._presenter.runChanged.call_count, 1)
        self.assertEqual(self.widget._fft._presenter.runChanged.call_count, 1)

    def test_new_instrument(self):
        self.mock_widgets()

        self.widget.handle_new_instrument()
        self.widget._maxent.clear.assert_called_once_with()

    def test_handle_new_group_pair(self):
        self.mock_widgets()
        self.widget.handle_new_group_pair()
        self.assertEqual(self.widget._fft.runChanged.call_count, 1)

    def test_disable_view(self):
        self.widget.disable_view()
        self.widget._view.setEnabled.assert_called_once_with(False)

    def test_enable_view(self):
        self.widget.enable_view()
        self.widget._view.setEnabled.assert_called_once_with(True)

    def test_set_up_calculation_observers(self):
        enable = mock.Mock()
        disable = mock.Mock()

        self.widget.set_up_calculation_observers(enable, disable)
        self.widget._maxent._presenter.calculation_finished_notifier.add_subscriber.assert_called_once_with(enable)
        self.widget._maxent._presenter.calculation_started_notifier.add_subscriber.assert_called_once_with(disable)

    def test_new_data_observers(self):
        observer = mock.Mock()

        self.widget.new_data_observer(observer)
        self.widget._maxent._presenter.calculation_finished_notifier.add_subscriber.assert_called_once_with(observer)
        self.widget._fft._presenter.calculation_finished_notifier.add_subscriber.assert_called_once_with(observer)


if __name__ == "__main__":
    unittest.main()
