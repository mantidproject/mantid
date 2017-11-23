from __future__ import (absolute_import, division, print_function)

import sys

from  Muon import load_utils
from  Muon import maxent_presenter
from  Muon import maxent_view
from  Muon import maxent_model
from  Muon import thread_model

import unittest
if sys.version_info.major == 3:
    from unittest import mock
else:
    import mock


class MaxEntPresenterTest(unittest.TestCase):
    def setUp(self):
        self.load=mock.create_autospec(load_utils.LoadUtils,spec_set=True)
        self.load.getCurrentWS=mock.Mock(return_value=["TEST00000001",["fwd","bkwd"]])

        self.model=mock.create_autospec(maxent_model.MaxEntModel,spec_set=True)

        self.view=mock.create_autospec(maxent_view.MaxEntView,spec_set=True)
        #signals
        #needed for connect in presenter
        self.view.maxEntButtonSignal=mock.Mock()
        # functions
        self.view.addItems=mock.MagicMock()
        self.view.initMaxEntInput=mock.Mock(return_value={"InputWorkspace":"testWS","EvolChi":"out",
                                            "ReconstructedData":"out2","ReconstructedImage":"out3","EvolAngle":"out4"})
        self.view.addRaw=mock.Mock()
        self.view.isRaw=mock.Mock(return_value=False)
        self.view.deactivateButton=mock.Mock()
        self.view.activateButton=mock.Mock()
         #set presenter
        self.presenter=maxent_presenter.MaxEntPresenter(self.view,self.model,self.load)

        # make thread
        self.thread=mock.create_autospec(thread_model.ThreadModel)
        self.thread.start=mock.Mock()
        self.thread.started=mock.Mock()
        self.thread.finished=mock.Mock()
        self.thread.setInputs=mock.Mock()
        self.thread.loadData=mock.Mock()

        self.presenter.createThread=mock.Mock(return_value=self.thread)

    def test_buttonWithRaw(self):
        self.view.isRaw=mock.Mock(return_value=True)
        self.presenter.handleMaxEntButton()
        assert(self.view.initMaxEntInput.call_count==1)
        assert(self.view.isRaw.call_count==1)
        assert(self.view.addRaw.call_count==5)
        assert(self.thread.start.call_count==1)

    def test_buttonWithoutRaw(self):
        self.view.isRaw=mock.Mock(return_value=False)
        self.presenter.handleMaxEntButton()
        assert(self.view.initMaxEntInput.call_count==1)
        assert(self.view.isRaw.call_count==1)
        assert(self.view.addRaw.call_count==0)
        assert(self.thread.start.call_count==1)

    def test_activateButton(self):
        self.presenter.activate()
        assert(self.view.activateButton.call_count==1)

    def test_deactivateButton(self):
        self.presenter.deactivate()
        assert(self.view.deactivateButton.call_count==1)


if __name__ == '__main__':
    unittest.main()
