# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from unittest import mock
from Muon.GUI.Common.utilities import load_utils
from Muon.GUI.Common import thread_model
from Muon.GUI.Common.test_helpers.context_setup import setup_context
from Muon.GUI.FrequencyDomainAnalysis.MaxEnt import maxent_presenter_new
from Muon.GUI.FrequencyDomainAnalysis.MaxEnt import maxent_view_new
from Muon.GUI.FrequencyDomainAnalysis.MaxEnt import maxent_model


def test(inputs):
    inputs["OutputPhaseTable"] = "test"

class MaxEntPresenterTest(unittest.TestCase):
    def setUp(self):
        self.context = setup_context(True)
        self.context.data_context.instrument = 'MUSR'

        self.context.gui_context.update({'RebinType': 'None'})
        self.model=mock.create_autospec(maxent_model.MaxEntWrapper, spec_set=True)
        self.model.cancel = mock.Mock()

        # View
        self.view = mock.create_autospec(maxent_view_new.MaxEntView, spec_set=True)
        # signals
        # needed for connect in presenter
        self.view.maxEntButtonSignal = mock.Mock()
        self.view.cancelSignal = mock.Mock()
        # functions
        self.view.addItems = mock.MagicMock()
        self.view.deactivateCalculateButton = mock.Mock()
        self.view.activateCalculateButton = mock.Mock()
        self.view.input_workspace = mock.Mock(return_value="TEST0000001")

        # Load
        self.load=mock.create_autospec(load_utils.LoadUtils,spec_set=True)
        self.load.getCurrentWS=mock.Mock(return_value=["TEST00000001",["fwd","bkwd"]])
        self.load.hasDataChanged = mock.MagicMock(return_value=False)

        # Presenter
        self.presenter = maxent_presenter_new.MaxEntPresenter(self.view, self.context)

        # make thread
        self.thread = mock.create_autospec(thread_model.ThreadModel)
        self.thread.start = mock.Mock()
        self.thread.started = mock.Mock()
        self.thread.finished = mock.Mock()
        self.thread.setInputs = mock.Mock()
        self.thread.loadData = mock.Mock()
        self.thread.threadWrapperSetup = mock.Mock()
        self.thread.threadWrapperTearDown = mock.Mock()

    def test_connects(self):
        self.assertEqual(1,self.view.cancelSignal.connect.call_count)
        self.view.cancelSignal.connect.assert_called_with(self.presenter.cancel)

        self.assertEqual(1,self.view.maxEntButtonSignal.connect.call_count)
        self.view.maxEntButtonSignal.connect.assert_called_with(self.presenter.handleMaxEntButton)

    def test_activate(self):
        self.presenter.activate()
        self.assertEqual(1,self.view.activateCalculateButton.call_count)

    def test_deactivate(self):
        self.presenter.deactivate()
        self.assertEqual(1,self.view.deactivateCalculateButton.call_count)

    def test_cancel(self):
        self.presenter.maxent_alg = self.model
        self.presenter.get_parameters_for_maxent_calculation = mock.MagicMock(return_value={'InputWorkspace':'TEST000001'})

        self.presenter.createThread = lambda *args: self.thread
        self.presenter.calculation_started_notifier = mock.MagicMock()

        self.presenter.handleMaxEntButton()
        self.presenter.cancel()
        #self.presenter.handleFinished()

        self.assertEqual(1,self.view.activateCalculateButton.call_count)

    def test_maxent_button(self):
        self.presenter.createThread = lambda *args:self.thread
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
        self.presenter.calculation_finished_notifier.notify_subscribers.called_with("TEST000001")

    def test_handle_error(self):
        self.presenter.handle_error("Error message")

        self.assertEqual(1, self.view.activateCalculateButton.call_count)
        self.assertEqual(1, self.view.warning_popup.call_count)
        self.view.warning_popup.assert_called_with("Error message")

    # def test_dataHasChanged(self):
    #     self.load.hasDataChanged = mock.MagicMock(return_value=True)
    #     self.presenter.handleMaxEntButton()
    #     assert(self.view.initMaxEntInput.call_count==0)
    #
    # def test_activateButton(self):
    #     self.presenter.activate()
    #     assert(self.view.activateCalculateButton.call_count==1)
    #
    # def test_deactivateButton(self):
    #     self.presenter.deactivate()
    #     assert(self.view.deactivateCalculateButton.call_count==1)
    #
    # def test_updatePhaseOptions(self):
    #     self.view.addOutputPhases = mock.MagicMock(side_effect = test)
    #     self.view.addPhaseTableToGUI = mock.MagicMock()
    #     self.presenter.thread = mock.MagicMock()
    #     self.presenter.phaseTableAdded = mock.Mock(return_value = True)
    #     self.view.getPhaseTableIndex = mock.Mock(return_value = 2)
    #     self.view.setPhaseTableIndex = mock.Mock()
    #     self.view.getPhaseTableOptions = mock.Mock(return_value = [])
    #
    #     inputs = {}
    #     self.view.addOutputPhases(inputs)
    #     self.presenter.handleFinished()
    #
    #     self.assertEqual(inputs["OutputPhaseTable"],"test")
    #     self.assertEqual(self.view.getPhaseTableOptions.call_count, 1)
    #     self.assertEqual(self.view.getPhaseTableIndex.call_count, 1)
    #     self.assertEqual(self.view.addPhaseTableToGUI.call_count, 1)
    #     self.assertEqual(self.view.setPhaseTableIndex.call_count, 1)
    #     self.view.setPhaseTableIndex.assert_called_with(2)
    #
    # def test_phaseOptionAlreadyExists(self):
    #     self.view.addOutputPhases = mock.MagicMock(side_effect = test)
    #     self.view.addPhaseTableToGUI = mock.MagicMock()
    #     self.presenter.thread = mock.MagicMock()
    #     self.presenter.phaseTableAdded = mock.Mock(return_value = True)
    #     self.view.getPhaseTableIndex = mock.Mock(return_value = 2)
    #     self.view.setPhaseTableIndex = mock.Mock()
    #     self.view.getPhaseTableOptions = mock.Mock(return_value = ["test"])
    #
    #     inputs = {}
    #     self.view.addOutputPhases(inputs)
    #     self.presenter.handleFinished()
    #
    #     self.assertEqual(inputs["OutputPhaseTable"],"test")
    #     self.assertEqual(self.view.getPhaseTableOptions.call_count, 1)
    #     self.assertEqual(self.view.getPhaseTableIndex.call_count, 0)
    #     self.assertEqual(self.view.addPhaseTableToGUI.call_count, 0)
    #     self.assertEqual(self.view.setPhaseTableIndex.call_count, 0)
    #
    # def test_noUpdatePhaseOptions(self):
    #     self.view.addOutputPhases = mock.MagicMock(side_effect = test)
    #     self.view.addPhaseTableToGUI = mock.MagicMock()
    #     self.presenter.thread = mock.MagicMock()
    #     self.presenter.phaseTableAdded = mock.Mock(return_value = False)
    #     self.view.getPhaseTableIndex = mock.Mock(return_value = 2)
    #     self.view.setPhaseTableIndex = mock.Mock()
    #     self.view.getPhaseTableOptions = mock.Mock(return_value = [])
    #
    #     inputs = {}
    #     self.view.addOutputPhases(inputs)
    #     self.presenter.handleFinished()
    #
    #     self.assertEqual(inputs["OutputPhaseTable"],"test")
    #     self.assertEqual(self.view.getPhaseTableOptions.call_count, 1)
    #     self.assertEqual(self.view.getPhaseTableIndex.call_count, 0)
    #     self.assertEqual(self.view.addPhaseTableToGUI.call_count, 0)
    #     self.assertEqual(self.view.setPhaseTableIndex.call_count, 0)
    #
    # def test_cancel(self):
    #     return
    #     self.presenter.createThread = lambda *args: self.thread
    #
    #     self.presenter.handleMaxEntButton()
    #     self.presenter.cancel()
    #     #self.assertEqual(self.presenter.handleFinished().call_count, 1)
    #     #self.assertEqual(1,self.view.activateCalculateButton.call_count)


if __name__ == '__main__':
    unittest.main()
