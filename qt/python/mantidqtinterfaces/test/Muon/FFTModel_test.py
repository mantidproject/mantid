# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from unittest import mock
from mantidqtinterfaces.Muon.GUI.FrequencyDomainAnalysis.FFT import fft_model


class FFTModelTest(unittest.TestCase):
    def setUp(self):
        self.model = mock.create_autospec(fft_model.FFTModel, spec_set=True, instance=True)
        self.model.setRun = mock.Mock()
        self.model.preAlg = mock.Mock()
        self.model.FFTAlg = mock.Mock()
        self.model.makePhaseQuadTable = mock.Mock()
        self.model.PhaseQuad = mock.Mock()

        # set presenter
        self.wrapper = fft_model.FFTWrapper(self.model)

    def test_execute(self):
        empty = {}
        inputs = {}
        inputs["Run"] = empty
        # inputs["phaseTable"]=None
        inputs["preRe"] = empty
        # inputs["preIm"]=None
        inputs["FFT"] = empty
        self.wrapper.loadData(inputs)
        self.wrapper.execute()
        assert self.model.setRun.call_count == 1
        assert self.model.preAlg.call_count == 1
        assert self.model.FFTAlg.call_count == 1
        assert self.model.makePhaseQuadTable.call_count == 0
        assert self.model.PhaseQuad.call_count == 0

    def test_executeIm(self):
        empty = {}
        inputs = {}
        inputs["Run"] = empty
        inputs["preRe"] = empty
        inputs["preIm"] = empty
        inputs["FFT"] = empty
        self.wrapper.loadData(inputs)
        self.wrapper.execute()
        assert self.model.setRun.call_count == 1
        assert self.model.preAlg.call_count == 2
        assert self.model.FFTAlg.call_count == 1
        assert self.model.makePhaseQuadTable.call_count == 0
        assert self.model.PhaseQuad.call_count == 0

    def test_executeImPhase(self):
        empty = {}
        inputs = {}
        inputs["Run"] = empty
        inputs["preRe"] = empty
        inputs["FFT"] = empty
        empty["newTable"] = True
        empty["axis"] = "x"
        empty["InputWorkspace"] = "MuonAnalysis"
        empty["Instrument"] = "MUSR"
        inputs["phaseTable"] = empty
        self.wrapper.loadData(inputs)
        self.wrapper.execute()
        assert self.model.setRun.call_count == 1
        assert self.model.preAlg.call_count == 1
        assert self.model.FFTAlg.call_count == 1
        assert self.model.makePhaseQuadTable.call_count == 1
        assert self.model.PhaseQuad.call_count == 1

    def test_executeImPhaseNoTable(self):
        empty = {}
        inputs = {}
        inputs["Run"] = empty
        inputs["preRe"] = empty
        inputs["FFT"] = empty
        empty["newTable"] = False
        empty["axis"] = "x"
        empty["Instrument"] = "MUSR"
        empty["InputWorkspace"] = "MuonAnalysis"
        inputs["phaseTable"] = empty
        self.wrapper.loadData(inputs)
        self.wrapper.execute()
        assert self.model.setRun.call_count == 1
        assert self.model.preAlg.call_count == 1
        assert self.model.FFTAlg.call_count == 1
        assert self.model.makePhaseQuadTable.call_count == 0
        assert self.model.PhaseQuad.call_count == 1


if __name__ == "__main__":
    unittest.main()
