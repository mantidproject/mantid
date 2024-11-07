# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.simpleapi import CreateWorkspace, Rebin, set_properties
from mantid.api import MatrixWorkspaceProperty, AlgorithmFactory, AlgorithmManager, DataProcessorAlgorithm, PythonAlgorithm
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
        self.declareProperty(MatrixWorkspaceProperty("Workspace", "", Direction.InOut), doc="Name to give the input workspace.")

    def PyExec(self):
        alg = self.createChildAlgorithm("ChildAlg")
        alg.initialize()
        args = {}
        kwargs = {}
        set_properties(alg, *args, **kwargs)
        alg.execute()


AlgorithmFactory.subscribe(ParentAlg)


class AlgorithmHistoryTest(unittest.TestCase):
    def test_nested_history(self):
        ws_name = "__tmp_test_algorithm_history"
        ws = CreateWorkspace([0, 1, 2], [0, 1, 2], OutputWorkspace=ws_name)
        self._run_algorithm("ParentAlg", Workspace=ws_name)

        history = ws.getHistory()
        alg_hists = history.getAlgorithmHistories()

        self.assertEqual(history.size(), 2)
        self.assertEqual(len(alg_hists), 2)

        parent_alg = history.getAlgorithmHistory(1)

        self.assertEqual(parent_alg.name(), "ParentAlg")
        self.assertEqual(parent_alg.version(), 1)
        self.assertEqual(parent_alg.childHistorySize(), 1)

        child_alg = parent_alg.getChildAlgorithmHistory(0)

        self.assertEqual(child_alg.name(), "ChildAlg")
        self.assertEqual(child_alg.version(), 1)
        self.assertEqual(child_alg.childHistorySize(), 0)

    def test_disable_history(self):
        ws_name = "__tmp_test_algorithm_history"
        ws = CreateWorkspace([0, 1, 2], [0, 1, 2], OutputWorkspace=ws_name)
        self._run_algorithm("ParentAlg", child_algorithm=True, record_history=False, Workspace=ws_name)

        history = ws.getHistory()
        alg_hists = history.getAlgorithmHistories()

        self.assertEqual(history.size(), 1)
        self.assertEqual(len(alg_hists), 1)

    def test_storeInADSFalse_workspace(self):
        ws = CreateWorkspace([1, 2, 3, 4, 5, 6, 7, 8, 9, 10], [1, 2, 3, 4, 5, 6, 7, 8, 9], StoreInADS=False)
        result = Rebin(ws, "1, 3, 10", Power=0.5)

        history = result.getHistory()
        create_workspace_history = history.getAlgorithmHistory(0)

        self.assertFalse(create_workspace_history.getStoreInADS())
        self._check_property_value(create_workspace_history, "OutputWorkspace", "ws")

        rebin_history = history.getAlgorithmHistory(1)
        self._check_property_value(rebin_history, "InputWorkspace", "ws")

    # -------------------------------------------------------------------------
    # Test Helper Functions
    # -------------------------------------------------------------------------

    def _run_algorithm(self, algorithm_name, child_algorithm=False, record_history=True, **kwargs):
        """Create and run an algorithm not in the simpleapi"""
        alg = AlgorithmManager.create(algorithm_name)
        alg.initialize()
        alg.setChild(child_algorithm)
        alg.enableHistoryRecordingForChild(record_history)
        for key, value in kwargs.items():
            alg.setProperty(key, value)
        alg.execute()
        return alg

    def _check_property_value(self, algorithm_history, name, value):
        properties = algorithm_history.getProperties()
        for prop in properties:
            if prop.name() == name:
                self.assertEqual(prop.value(), value)
                return
        self.fail(f"Property {name} not found in algorithm history")


if __name__ == "__main__":
    unittest.main()
