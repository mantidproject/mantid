# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.py3compat import mock
from Muon.GUI.Common.utilities import load_utils
from Muon.GUI.Common import thread_model
from Muon.GUI.FrequencyDomainAnalysis.FFT import fft_presenter_new
from Muon.GUI.FrequencyDomainAnalysis.FFT import fft_view
from Muon.GUI.FrequencyDomainAnalysis.FFT import fft_model
from Muon.GUI.Common.contexts.muon_context import MuonContext


class FFTPresenterTest(unittest.TestCase):

    def setUp(self):
        self.load = mock.create_autospec(MuonContext, spec_set=True)

        self.view = mock.create_autospec(fft_view.FFTView, spec_set=True)
        # signals
        self.view.tableClickSignal = mock.Mock(return_value=[3, 1])
        self.view.phaseCheckSignal = mock.Mock(return_value=True)
        # needed for connect in presenter
        self.view.buttonSignal = mock.Mock()
        self.view.tableClickSignal = mock.Mock()
        self.view.phaseCheckSignal = mock.Mock()
        self.view.changed = mock.MagicMock()
        self.view.changedHideUnTick = mock.MagicMock()
        self.view.initFFTInput = mock.Mock(
            return_value={
                "InputWorkspace": "testWS",
                "OutputWorkspace": "muon"})
        self.view.addFFTComplex = mock.Mock(
            return_value={"InputImWorkspace": "MuonFFT"})
        self.view.addFFTShift = mock.Mock()
        self.view.addRaw = mock.Mock()
        self.view.getFFTRePhase = mock.Mock()
        self.view.getFFTImPhase = mock.Mock()
        self.view.getWS = mock.Mock(return_value="MUSR00023456")
        self.view.getFirstGoodData = mock.Mock(return_value=0.1)
        self.view.getLastGoodData = mock.Mock(return_value=15.)
        self.view.getImBoxRow = mock.Mock(return_value=3)
        self.view.getShiftBoxRow = mock.Mock(return_value=5)
        self.view.isRaw = mock.Mock(return_value=True)
        self.view.isComplex = mock.Mock(return_value=True)
        self.view.isAutoShift = mock.Mock(return_value=True)
        self.view.setPhaseBox = mock.Mock()
        self.view.isNewPhaseTable = mock.Mock(return_value=True)
        self.view.activateButton = mock.Mock()
        self.view.deactivateButton = mock.Mock()
        # setup model
        self.model1 = mock.create_autospec(fft_model.FFTModel, spec_set=False)
        self.model = mock.create_autospec(fft_model.FFTWrapper, spec_set=False)

        # set presenter
        self.presenter = fft_presenter_new.FFTPresenter(
            self.view, self.model, self.load)

        # mock thread
        self.thread = mock.create_autospec(thread_model.ThreadModel)
        self.thread.start = mock.Mock()
        self.thread.started = mock.Mock()
        self.thread.finished = mock.Mock()
        self.thread.setInputs = mock.Mock()
        self.thread.loadData = mock.Mock()
        self.presenter.createThread = mock.Mock(return_value=self.thread)

    def sendSignal(self):
        row, col = self.view.tableClickSignal()
        self.presenter.tableClicked(row, col)

    def test_connects(self):
        assert(self.view.tableClickSignal.connect.call_count==1)
        self.view.tableClickSignal.connect.assert_called_with(self.presenter.tableClicked)

        assert(self.view.buttonSignal.connect.call_count==1)
        self.view.buttonSignal.connect.assert_called_with(self.presenter.handleButton)

        assert(self.view.phaseCheckSignal.connect.call_count==1)
        self.view.phaseCheckSignal.connect.assert_called_with(self.presenter.phaseCheck)

    def test_ImBox(self):
        self.view.tableClickSignal = mock.Mock(return_value=[3, 1])
        self.sendSignal()
        assert(self.view.changedHideUnTick.call_count == 1)
        assert(self.view.changed.call_count == 0)

    def test_shiftBox(self):
        self.view.tableClickSignal = mock.Mock(return_value=[5, 1])
        self.sendSignal()
        assert(self.view.changed.call_count == 1)
        assert(self.view.changedHideUnTick.call_count == 0)

    def test_buttonNotRawAndNoIm(self):
        self.view.isAutoShift = mock.Mock(return_value=True)
        self.view.isComplex = mock.Mock(return_value=False)
        self.view.isRaw = mock.Mock(return_value=False)
        self.presenter.handleButton()
        assert(self.view.initFFTInput.call_count == 1)
        assert(self.view.addFFTComplex.call_count == 0)
        assert(self.view.addFFTShift.call_count == 0)
        assert(self.view.setPhaseBox.call_count == 1)
        assert(self.view.getFirstGoodData.call_count == 0)
        assert(self.view.getLastGoodData.call_count == 0)
        assert(self.presenter.thread.start.call_count == 1)

    def test_buttonNotRawAndIm(self):
        self.view.isAutoShift = mock.Mock(return_value=True)
        self.view.isComplex = mock.Mock(return_value=True)
        self.view.isRaw = mock.Mock(return_value=False)
        self.presenter.handleButton()
        assert(self.view.initFFTInput.call_count == 1)
        assert(self.view.addFFTComplex.call_count == 1)
        assert(self.view.addFFTShift.call_count == 0)
        assert(self.view.setPhaseBox.call_count == 1)
        assert(self.view.getFirstGoodData.call_count == 0)
        assert(self.view.getLastGoodData.call_count == 0)
        assert(self.presenter.thread.start.call_count == 1)

    def test_buttonRawAndIm(self):
        self.view.isAutoShift = mock.Mock(return_value=True)
        self.view.isComplex = mock.Mock(return_value=True)
        self.view.isRaw = mock.Mock(return_value=True)
        self.presenter.handleButton()
        assert(self.view.initFFTInput.call_count == 1)
        assert(self.view.addFFTComplex.call_count == 1)
        assert(self.view.addFFTShift.call_count == 0)
        assert(self.view.setPhaseBox.call_count == 1)
        assert(self.view.getFirstGoodData.call_count == 0)
        assert(self.view.getLastGoodData.call_count == 0)
        assert(self.presenter.thread.start.call_count == 1)

    def test_buttonRawAndNoIm(self):
        self.view.isAutoShift = mock.Mock(return_value=True)
        self.view.isComplex = mock.Mock(return_value=False)
        self.view.isRaw = mock.Mock(return_value=True)
        self.presenter.handleButton()
        assert(self.view.initFFTInput.call_count == 1)
        assert(self.view.addFFTComplex.call_count == 0)
        assert(self.view.addFFTShift.call_count == 0)
        assert(self.view.setPhaseBox.call_count == 1)
        assert(self.view.getFirstGoodData.call_count == 0)
        assert(self.view.getLastGoodData.call_count == 0)
        assert(self.presenter.thread.start.call_count == 1)

    def test_buttonNoShiftNotRawAndNoIm(self):
        self.view.isAutoShift = mock.Mock(return_value=False)
        self.view.isComplex = mock.Mock(return_value=False)
        self.view.isRaw = mock.Mock(return_value=False)
        self.presenter.handleButton()
        assert(self.view.initFFTInput.call_count == 1)
        assert(self.view.addFFTComplex.call_count == 0)
        assert(self.view.addFFTShift.call_count == 1)
        assert(self.view.setPhaseBox.call_count == 1)
        assert(self.view.getFirstGoodData.call_count == 0)
        assert(self.view.getLastGoodData.call_count == 0)
        assert(self.presenter.thread.start.call_count == 1)

    def test_buttonNoShiftNotRawAndIm(self):
        self.view.isAutoShift = mock.Mock(return_value=False)
        self.view.isComplex = mock.Mock(return_value=True)
        self.view.isRaw = mock.Mock(return_value=False)
        self.presenter.handleButton()
        assert(self.view.initFFTInput.call_count == 1)
        assert(self.view.addFFTComplex.call_count == 1)
        assert(self.view.addFFTShift.call_count == 1)
        assert(self.view.setPhaseBox.call_count == 1)
        assert(self.view.getFirstGoodData.call_count == 0)
        assert(self.view.getLastGoodData.call_count == 0)
        assert(self.presenter.thread.start.call_count == 1)

    def test_buttonNoShiftRawAndIm(self):
        self.view.isAutoShift = mock.Mock(return_value=False)
        self.view.isComplex = mock.Mock(return_value=True)
        self.view.isRaw = mock.Mock(return_value=True)
        self.presenter.handleButton()
        self.assertEquals(self.view.initFFTInput.call_count, 1)
        self.assertEquals(self.view.addFFTComplex.call_count,1)
        self.assertEquals(self.view.addFFTShift.call_count, 1)
        self.assertEquals(self.view.setPhaseBox.call_count, 1)
        self.assertEquals(self.view.getFirstGoodData.call_count, 0)
        self.assertEquals(self.view.getLastGoodData.call_count, 0)
        self.assertEquals(self.presenter.thread.start.call_count, 1)

    def test_buttonNoShiftRawAndNoIm(self):
        self.view.isAutoShift = mock.Mock(return_value=False)
        self.view.isComplex = mock.Mock(return_value=False)
        self.view.isRaw = mock.Mock(return_value=True)
        self.presenter.handleButton()
        assert(self.view.initFFTInput.call_count == 1)
        assert(self.view.addFFTComplex.call_count == 0)
        assert(self.view.addFFTShift.call_count == 1)
        assert(self.view.setPhaseBox.call_count == 1)
        assert(self.view.getFirstGoodData.call_count == 0)
        assert(self.view.getLastGoodData.call_count == 0)
        assert(self.presenter.thread.start.call_count == 1)

    def test_buttonPhaseQuad(self):
        self.view.getWS = mock.Mock(return_value="PhaseQuad")
        self.view.isComplex = mock.Mock(return_value=False)
        self.presenter.handleButton()
        assert(self.view.initFFTInput.call_count == 1)
        assert(self.view.addFFTComplex.call_count == 0)
        assert(self.view.addFFTShift.call_count == 0)
        assert(self.view.setPhaseBox.call_count == 1)
        assert(self.view.getFirstGoodData.call_count == 1)
        assert(self.view.getLastGoodData.call_count == 1)
        assert(self.view.getFFTRePhase.call_count == 1)
        assert(self.view.getFFTImPhase.call_count == 0)
        assert(self.presenter.thread.start.call_count == 1)

    def test_buttonImPhaseQuad(self):
        self.view.getWS = mock.Mock(return_value="PhaseQuad")
        self.view.isComplex = mock.Mock(return_value=True)
        self.presenter.handleButton()
        assert(self.view.initFFTInput.call_count == 1)
        assert(self.view.addFFTComplex.call_count == 1)
        assert(self.view.addFFTShift.call_count == 0)
        assert(self.view.setPhaseBox.call_count == 1)
        assert(self.view.getFirstGoodData.call_count == 1)
        assert(self.view.getLastGoodData.call_count == 1)
        assert(self.view.getFFTRePhase.call_count == 1)
        assert(self.view.getFFTImPhase.call_count == 1)
        assert(self.presenter.thread.start.call_count == 1)

    def test_buttonPhaseQuadNoTable(self):
        self.view.getWS = mock.Mock(return_value="PhaseQuad")
        self.view.isComplex = mock.Mock(return_value=False)
        self.view.isNewPhaseTable = mock.Mock(return_value=False)
        self.presenter.handleButton()
        assert(self.view.initFFTInput.call_count == 1)
        assert(self.view.addFFTComplex.call_count == 0)
        assert(self.view.addFFTShift.call_count == 0)
        assert(self.view.setPhaseBox.call_count == 1)
        assert(self.view.getFirstGoodData.call_count == 1)
        assert(self.view.getLastGoodData.call_count == 1)
        assert(self.view.getFFTRePhase.call_count == 1)
        assert(self.view.getFFTImPhase.call_count == 0)
        assert(self.presenter.thread.start.call_count == 1)

    def test_buttonImPhaseQuadNoTable(self):
        self.view.getWS = mock.Mock(return_value="PhaseQuad")
        self.view.isNewPhaseTable = mock.Mock(return_value=False)
        self.view.isComplex = mock.Mock(return_value=True)
        self.presenter.handleButton()
        assert(self.view.initFFTInput.call_count == 1)
        assert(self.view.addFFTComplex.call_count == 1)
        assert(self.view.addFFTShift.call_count == 0)
        assert(self.view.setPhaseBox.call_count == 1)
        assert(self.view.getFirstGoodData.call_count == 1)
        assert(self.view.getLastGoodData.call_count == 1)
        assert(self.view.getFFTRePhase.call_count == 1)
        assert(self.view.getFFTImPhase.call_count == 1)
        assert(self.presenter.thread.start.call_count == 1)

    def test_activateButton(self):
        self.presenter.activate()
        assert(self.view.activateButton.call_count == 1)

    def test_deactivateButton(self):
        self.presenter.deactivate()
        assert(self.view.deactivateButton.call_count == 1)


if __name__ == '__main__':
    unittest.main()