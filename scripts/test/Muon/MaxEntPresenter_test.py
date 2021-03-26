# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from unittest import mock
from Muon.GUI.Common import thread_model
from Muon.GUI.Common.test_helpers.context_setup import setup_context
from Muon.GUI.FrequencyDomainAnalysis.MaxEnt import maxent_presenter_new
from Muon.GUI.FrequencyDomainAnalysis.MaxEnt import maxent_view_new
from Muon.GUI.FrequencyDomainAnalysis.MaxEnt import maxent_model


class MaxEntPresenterTest(unittest.TestCase):
    def setUp(self):
        self.context = setup_context(True)
        self.context.data_context.instrument = 'MUSR'
        self.context.gui_context.update({'RebinType': 'None'})

        # Model
        self.model = mock.create_autospec(maxent_model.MaxEntWrapper, spec_set=True)

        # View
        self.view = mock.create_autospec(maxent_view_new.MaxEntView, spec_set=True)
        # signals
        # needed for connect in presenter
        self.view.maxEntButtonSignal = mock.Mock()
        self.view.cancelSignal = mock.Mock()

        # Default Values
        self.view.input_workspace = "TEST00000001"
        self.view.phase_table = "Construct"
        self.view.num_points = 2048
        self.view.inner_iterations = 10
        self.view.outer_iterations = 10
        self.view.double_pulse = False
        self.view.lagrange_multiplier = 1
        self.view.maximum_field = 1000
        self.view.maximum_entropy_constant = 0.1
        self.view.fit_dead_times = False

        # Presenter
        self.presenter = maxent_presenter_new.MaxEntPresenter(self.view, self.context)

        # make thread
        self.thread = mock.create_autospec(thread_model.ThreadModel)

    def test_connects(self):
        self.assertEqual(1, self.view.cancelSignal.connect.call_count)
        self.view.cancelSignal.connect.assert_called_with(self.presenter.cancel)

        self.assertEqual(1, self.view.maxEntButtonSignal.connect.call_count)
        self.view.maxEntButtonSignal.connect.assert_called_with(self.presenter.handleMaxEntButton)

    def test_activate(self):
        self.presenter.activate()
        self.assertEqual(1, self.view.activateCalculateButton.call_count)

    def test_deactivate(self):
        self.presenter.deactivate()
        self.assertEqual(1, self.view.deactivateCalculateButton.call_count)

    def test_clear(self):
        self.presenter.clear()

        self.assertEqual(2, self.view.addItems.call_count)
        self.assertEqual(2, self.view.update_phase_table_combo.call_count)
        self.assertRaises(KeyError, lambda: self.presenter.get_parameters_for_maxent_calculation()['InputPhaseTable'])

    def test_cancel(self):
        self.presenter.maxent_alg = self.model

        self.presenter.cancel()

        self.assertEqual(1, self.model.cancel.call_count)

    def test_maxent_button(self):
        self.presenter.createThread = lambda *args: self.thread
        self.presenter.calculation_started_notifier = mock.MagicMock()

        self.presenter.handleMaxEntButton()

        self.assertEqual(1, self.thread.threadWrapperSetUp.call_count)
        self.assertEqual(1, self.presenter.calculation_started_notifier.notify_subscribers.call_count)
        self.assertEqual(1, self.thread.start.call_count)

    def test_handle_finsihed(self):
        self.presenter.calculation_finished_notifier = mock.MagicMock()
        self.presenter._maxent_output_workspace_name = "TEST0000001"

        self.presenter.handleFinished()

        self.assertEqual(1, self.view.activateCalculateButton.call_count)
        self.assertEqual(1, self.presenter.calculation_finished_notifier.notify_subscribers.call_count)
        self.presenter.calculation_finished_notifier.notify_subscribers.assert_called_with("TEST0000001")

    def test_handle_error(self):
        self.presenter.handle_error("Error message")

        self.assertEqual(1, self.view.activateCalculateButton.call_count)
        self.assertEqual(1, self.view.warning_popup.call_count)
        self.view.warning_popup.assert_called_with("Error message")


if __name__ == '__main__':
    unittest.main()
