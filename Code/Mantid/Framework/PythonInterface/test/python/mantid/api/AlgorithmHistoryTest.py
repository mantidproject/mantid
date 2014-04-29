import unittest
from mantid.simpleapi import Load, CreateTransmissionWorkspaceAuto, _set_properties
from mantid import AlgorithmManager, mtd


class AlgorithmHistoryTest(unittest.TestCase):

    def test_nested_history(self):
        trans1 = Load('INTER00013463.nxs', OutputWorkspace="trans1")
        #run an algorithm which has multiple layers of history
        out_ws = CreateTransmissionWorkspaceAuto(FirstTransmissionRun=trans1, AnalysisMode="PointDetectorAnalysis")

        history = out_ws.getHistory()

        alg_hists = history.getAlgorithmHistories()
        self.assertEquals(history.size(), 2)
        self.assertEquals(len(alg_hists), 2)

        parent_alg = history.getAlgorithmHistory(1)
        self.assertEquals(parent_alg.name(), "CreateTransmissionWorkspaceAuto")
        self.assertEquals(parent_alg.version(), 1)

        alg = parent_alg.getChildAlgorithm(0)
        self.assertEquals(alg.name(), "CreateTransmissionWorkspace")

        self.assertEquals(parent_alg.childHistorySize(), 1)

        nested_alg = parent_alg.getChildAlgorithmHistory(0)
        self.assertEquals(nested_alg.name(), "CreateTransmissionWorkspace")
        self.assertEquals(nested_alg.version(), 1)

        basic_child_algs = nested_alg.getChildHistories()
        self.assertGreater(len(basic_child_algs), 0)


if __name__ == '__main__':
    unittest.main()
