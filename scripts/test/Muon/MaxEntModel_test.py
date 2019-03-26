# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.py3compat import mock
from Muon.GUI.FrequencyDomainAnalysis.MaxEnt import maxent_model


class MaxEntModelTest(unittest.TestCase):

    def setUp(self):
        self.model = mock.create_autospec(
            maxent_model.MaxEntModel,
            spec_set=True)
        self.model.setRun = mock.Mock()
        self.model.MaxEntAlg = mock.Mock()
        self.model.makePhaseTable = mock.Mock()

        # set presenter
        self.wrapper = maxent_model.MaxEntWrapper(self.model)

    def test_execute(self):
        empty = {}
        inputs = {}
        inputs["Run"] = empty
        inputs["maxent"] = empty
        self.wrapper.loadData(inputs)
        self.wrapper.execute()
        assert(self.model.setRun.call_count == 1)
        assert(self.model.MaxEntAlg.call_count == 1)
        assert(self.model.makePhaseTable.call_count == 0)

    def testAll_execute(self):
        empty = {}
        inputs = {}
        inputs["Run"] = empty
        inputs["maxent"] = empty
        inputs["phaseTable"] = empty
        self.wrapper.loadData(inputs)
        self.wrapper.execute()
        assert(self.model.setRun.call_count == 1)
        assert(self.model.MaxEntAlg.call_count == 1)
        assert(self.model.makePhaseTable.call_count == 1)

if __name__ == '__main__':
    unittest.main()
