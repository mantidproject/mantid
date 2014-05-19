import unittest
from mantid.simpleapi import Load, CreateTransmissionWorkspaceAuto, CreateWorkspace, _set_properties
from mantid.api import *
from mantid.kernel import *


class ChildAlg(PythonAlgorithm):

    def PyInit(self):
        pass

    def PyExec(self):
        pass

AlgorithmFactory.subscribe(ChildAlg)


class ParentAlg(DataProcessorAlgorithm):

    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspace', '', Direction.Output),
                             doc="Name to give the output workspace.")

    def PyExec(self):
        ws_name = self.getProperty("OutputWorkspace").value
        alg = self.createChildAlgorithm('ChildAlg')
        alg.initialize()
        args = {}
        kwargs = {}
        _set_properties(alg, *args, **kwargs)
        alg.execute()

        ws = CreateWorkspace([0, 1, 2], [0, 1, 2], OutputWorkspace=ws_name)
        self.setProperty('OutputWorkspace', ws)

AlgorithmFactory.subscribe(ParentAlg)


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
        self.assertTrue(len(basic_child_algs) > 0)

    def test_disable_history(self):
        ws_name = '__tmp_test_algorithm_history'

        alg = AlgorithmManager.createUnmanaged('ParentAlg')
        alg.initialize()
        alg.setChild(True)
        alg.enableHistoryRecordingForChild(False)
        alg.setProperty("OutputWorkspace", ws_name)
        alg.execute()
        history = alg.getProperty("OutputWorkspace").value.getHistory()

        alg_hists = history.getAlgorithmHistories()
        self.assertEquals(history.size(), 0)
        self.assertEquals(len(alg_hists), 0)

if __name__ == '__main__':
    unittest.main()
