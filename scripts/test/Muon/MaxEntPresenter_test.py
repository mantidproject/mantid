# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest

from mantid.py3compat import mock
from Muon.GUI.Common.utilities import load_utils
from Muon.GUI.Common import thread_model
from Muon.GUI.FrequencyDomainAnalysis.MaxEnt import maxent_presenter
from Muon.GUI.FrequencyDomainAnalysis.MaxEnt import maxent_view
from Muon.GUI.FrequencyDomainAnalysis.MaxEnt import maxent_model


def test(inputs):
    inputs["OutputPhaseTable"] = "test"


class MaxEntPresenterTest(unittest.TestCase):
    def setUp(self):
        self.load=mock.create_autospec(load_utils.LoadUtils,spec_set=True)
        self.load.getCurrentWS=mock.Mock(return_value=["TEST00000001",["fwd","bkwd"]])
        self.load.hasDataChanged = mock.MagicMock(return_value=False)

        self.model=mock.create_autospec(maxent_model.MaxEntModel,spec_set=True)

        self.view=mock.create_autospec(maxent_view.MaxEntView,spec_set=True)
        #signals
        #needed for connect in presenter
        self.view.maxEntButtonSignal=mock.Mock()
        self.view.cancelSignal=mock.Mock()
        self.view.phaseSignal=mock.Mock()
        # functions
        self.view.addItems=mock.MagicMock()
        self.view.initMaxEntInput=mock.Mock(return_value={"InputWorkspace":"testWS","EvolChi":"out",
                                            "ReconstructedData":"out2","ReconstructedImage":"out3","EvolAngle":"out4"})
        self.view.deactivateCalculateButton=mock.Mock()
        self.view.activateCalculateButton=mock.Mock()
        self.view.usePhases = mock.Mock(return_value=False)
         #set presenter
        self.presenter=maxent_presenter.MaxEntPresenter(self.view,self.model,self.load)

        # make thread
        self.thread=mock.create_autospec(thread_model.ThreadModel)
        self.thread.start=mock.Mock()
        self.thread.started=mock.Mock()
        self.thread.finished=mock.Mock()
        self.thread.setInputs=mock.Mock()
        self.thread.loadData=mock.Mock()
        self.thread.threadWrapperSetup = mock.Mock()
        self.thread.threadWrapperTearDown = mock.Mock()

    def test_connects(self):
        assert(self.view.cancelSignal.connect.call_count==1)
        self.view.cancelSignal.connect.assert_called_with(self.presenter.cancel)

        assert(self.view.maxEntButtonSignal.connect.call_count==1)
        self.view.maxEntButtonSignal.connect.assert_called_with(self.presenter.handleMaxEntButton)

        assert(self.view.phaseSignal.connect.call_count==1)
        self.view.phaseSignal.connect.assert_called_with(self.presenter.handlePhase)

    def test_button(self):
        self.presenter.createThread = lambda *args:self.thread

        self.presenter.handleMaxEntButton()
        assert(self.view.initMaxEntInput.call_count==1)
        assert(self.thread.start.call_count==1)

        assert(self.thread.threadWrapperSetUp.call_count==1)

    def test_dataHasChanged(self):
        self.load.hasDataChanged = mock.MagicMock(return_value=True)
        self.presenter.handleMaxEntButton()
        assert(self.view.initMaxEntInput.call_count==0)

    def test_activateButton(self):
        self.presenter.activate()
        assert(self.view.activateCalculateButton.call_count==1)

    def test_deactivateButton(self):
        self.presenter.deactivate()
        assert(self.view.deactivateCalculateButton.call_count==1)

    def test_updatePhaseOptions(self):
        self.view.addOutputPhases = mock.MagicMock(side_effect = test)
        self.view.addPhaseTableToGUI = mock.MagicMock()
        self.presenter.thread = mock.MagicMock()
        self.presenter.phaseTableAdded = mock.Mock(return_value = True)
        self.view.getPhaseTableIndex = mock.Mock(return_value = 2)
        self.view.setPhaseTableIndex = mock.Mock()
        self.view.getPhaseTableOptions = mock.Mock(return_value = [])

        inputs = {}
        self.view.addOutputPhases(inputs)
        self.presenter.handleFinished()

        self.assertEquals(inputs["OutputPhaseTable"],"test")
        self.assertEquals(self.view.getPhaseTableOptions.call_count, 1)
        self.assertEquals(self.view.getPhaseTableIndex.call_count, 1)
        self.assertEquals(self.view.addPhaseTableToGUI.call_count, 1)
        self.assertEquals(self.view.setPhaseTableIndex.call_count, 1)
        self.view.setPhaseTableIndex.assert_called_with(2)

    def test_phaseOptionAlreadyExists(self):
        self.view.addOutputPhases = mock.MagicMock(side_effect = test)
        self.view.addPhaseTableToGUI = mock.MagicMock()
        self.presenter.thread = mock.MagicMock()
        self.presenter.phaseTableAdded = mock.Mock(return_value = True)
        self.view.getPhaseTableIndex = mock.Mock(return_value = 2)
        self.view.setPhaseTableIndex = mock.Mock()
        self.view.getPhaseTableOptions = mock.Mock(return_value = ["test"])

        inputs = {}
        self.view.addOutputPhases(inputs)
        self.presenter.handleFinished()

        self.assertEquals(inputs["OutputPhaseTable"],"test")
        self.assertEquals(self.view.getPhaseTableOptions.call_count, 1)
        self.assertEquals(self.view.getPhaseTableIndex.call_count, 0)
        self.assertEquals(self.view.addPhaseTableToGUI.call_count, 0)
        self.assertEquals(self.view.setPhaseTableIndex.call_count, 0)

    def test_noUpdatePhaseOptions(self):
        self.view.addOutputPhases = mock.MagicMock(side_effect = test)
        self.view.addPhaseTableToGUI = mock.MagicMock()
        self.presenter.thread = mock.MagicMock()
        self.presenter.phaseTableAdded = mock.Mock(return_value = False)
        self.view.getPhaseTableIndex = mock.Mock(return_value = 2)
        self.view.setPhaseTableIndex = mock.Mock()
        self.view.getPhaseTableOptions = mock.Mock(return_value = [])

        inputs = {}
        self.view.addOutputPhases(inputs)
        self.presenter.handleFinished()

        self.assertEquals(inputs["OutputPhaseTable"],"test")
        self.assertEquals(self.view.getPhaseTableOptions.call_count, 1)
        self.assertEquals(self.view.getPhaseTableIndex.call_count, 0)
        self.assertEquals(self.view.addPhaseTableToGUI.call_count, 0)
        self.assertEquals(self.view.setPhaseTableIndex.call_count, 0)

if __name__ == '__main__':
    unittest.main()
