import unittest
from mantid.simpleapi import CreateWorkspace, _set_properties
from mantid.api import (MatrixWorkspaceProperty, AlgorithmFactory, AlgorithmManager,
                        DataProcessorAlgorithm, PythonAlgorithm)
from mantid.kernel import Direction


class ChildAlg(PythonAlgorithm):

    def PyInit(self):
        pass

    def PyExec(self):
        pass

AlgorithmFactory.subscribe(ChildAlg)


class ParentAlg(DataProcessorAlgorithm):
    """Dummy workflow algorithm for testing purposes.
    This just creates an output workspace and runs a child algorithm.
    """

    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty('Workspace', '', Direction.InOut),
                             doc="Name to give the input workspace.")

    def PyExec(self):

        alg = self.createChildAlgorithm('ChildAlg')
        alg.initialize()
        args = {}
        kwargs = {}
        _set_properties(alg, *args, **kwargs)
        alg.execute()

AlgorithmFactory.subscribe(ParentAlg)


class AlgorithmHistoryTest(unittest.TestCase):

    def test_nested_history(self):
        ws_name = '__tmp_test_algorithm_history'
        ws = CreateWorkspace([0, 1, 2], [0, 1, 2], OutputWorkspace=ws_name)
        alg = self._run_algorithm("ParentAlg", Workspace=ws_name)

        history = ws.getHistory()
        alg_hists = history.getAlgorithmHistories()

        self.assertEquals(history.size(), 2)
        self.assertEquals(len(alg_hists), 2)

        parent_alg = history.getAlgorithmHistory(1)

        self.assertEquals(parent_alg.name(), "ParentAlg")
        self.assertEquals(parent_alg.version(), 1)
        self.assertEquals(parent_alg.childHistorySize(), 1)

        child_alg = parent_alg.getChildAlgorithmHistory(0)

        self.assertEquals(child_alg.name(), "ChildAlg")
        self.assertEquals(child_alg.version(), 1)
        self.assertEquals(child_alg.childHistorySize(), 0)

    def test_disable_history(self):
        ws_name = '__tmp_test_algorithm_history'
        ws = CreateWorkspace([0, 1, 2], [0, 1, 2], OutputWorkspace=ws_name)
        alg = self._run_algorithm('ParentAlg', child_algorithm=True, record_history=False, Workspace=ws_name)

        history = ws.getHistory()
        alg_hists = history.getAlgorithmHistories()

        self.assertEquals(history.size(), 1)
        self.assertEquals(len(alg_hists), 1)

    #-------------------------------------------------------------------------
    # Test Helper Functions
    #-------------------------------------------------------------------------

    def _run_algorithm(self, algorithm_name, child_algorithm=False, record_history=True, **kwargs):
        """ Create and run an algorithm not in the simpleapi"""
        alg = AlgorithmManager.create(algorithm_name)
        alg.initialize()
        alg.setChild(child_algorithm)
        alg.enableHistoryRecordingForChild(record_history)
        for key, value in kwargs.iteritems():
            alg.setProperty(key, value)
        alg.execute()
        return alg



if __name__ == '__main__':
    unittest.main()
