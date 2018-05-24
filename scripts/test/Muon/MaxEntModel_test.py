import sys

from Muon import maxent_model

import unittest

if sys.version_info.major == 3:
    from unittest import mock
else:
    import mock


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
