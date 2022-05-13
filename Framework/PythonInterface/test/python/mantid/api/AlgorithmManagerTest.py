# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import testhelpers
from mantid.api import (AlgorithmManager, Algorithm,
                        FrameworkManagerImpl, IAlgorithm, FunctionFactory, WorkspaceFactory)
from mantid.simpleapi import (CreateWorkspace, Gaussian)


class AlgorithmManagerTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        # Load the plugins
        FrameworkManagerImpl.Instance()

    def tearDown(self):
        AlgorithmManager.clear()

    def test_create_default_version(self):
        alg = testhelpers.assertRaisesNothing(self, AlgorithmManager.create, "ConvertUnits")
        # Tests
        self.assertNotEqual(alg, None)
        self.assertEqual(alg.name(), "ConvertUnits")
        self.assertEqual(alg.version(), 1)
        self.assertEqual(alg.category(), "Transforms\\Units")
        self.assertEqual(alg.helpURL(), "")

    def test_create_unknown_alg_throws(self):
        self.assertRaises(RuntimeError, AlgorithmManager.create,"DoesNotExist")

    def test_created_alg_isinstance_of_IAlgorithm(self):
        alg = AlgorithmManager.create("ConvertUnits")
        self.assertTrue(isinstance(alg, IAlgorithm))

    def test_unmanaged_cppalg_isinstance_of_Algorithm(self):
        alg = AlgorithmManager.createUnmanaged("ConvertUnits")
        self.assertTrue(isinstance(alg, Algorithm))

    def test_size_reports_number_of_managed_algorithms(self):
        # no longer deterministically possible to have a correct answer for size
        # if test are run multi threaded
        # just check we got an integer back and that it is greater than zero if we
        # have created at least one managed algorithm
        AlgorithmManager.create("ConvertUnits")
        size = AlgorithmManager.size()
        self.assertTrue(isinstance(size, int))
        self.assertGreater(size, 0)

    def test_getAlgorithm_returns_correct_instance(self):
        returned_instance = AlgorithmManager.create("ConvertUnits")
        id = returned_instance.getAlgorithmID()
        mgr_instance = AlgorithmManager.getAlgorithm(id)
        self.assertEqual(id, mgr_instance.getAlgorithmID())

    def test_removeById_removes_correct_instance(self):
        alg = AlgorithmManager.create("ConvertUnits")
        alg2 = AlgorithmManager.create("ConvertUnits")
        AlgorithmManager.removeById(alg.getAlgorithmID())
        self.assertEqual(None, AlgorithmManager.getAlgorithm(alg.getAlgorithmID()))
        self.assertNotEqual(None, AlgorithmManager.getAlgorithm(alg2.getAlgorithmID()))

    def test_runningInstancesOf_returns_python_list(self):
        algs = AlgorithmManager.runningInstancesOf("ConvertUnits")
        self.assertTrue(isinstance(algs, list))

        import threading
        class AlgThread(threading.Thread):
            def __init__(self):
                threading.Thread.__init__(self)
                self.algorithm = AlgorithmManager.create("Pause")
            def run(self):
                self.algorithm.initialize()
                self.algorithm.setProperty("Duration", -1.0) #forever
                self.algorithm.execute()
        # end class
        pause_thread = AlgThread()
        try:
            pause_thread.start()
            while not pause_thread.algorithm.isRunning():
                pass
            # should now be running
            algs = AlgorithmManager.runningInstancesOf("Pause")
            self.assertTrue(isinstance(algs, list))
            self.assertEqual(1, len(algs))
        except:
            pause_thread.algorithm.cancel()
            pause_thread.join()
            raise
        finally:
            pause_thread.algorithm.cancel()
            pause_thread.join()

    def test_clear_removes_all_managed_algorithms(self):
        AlgorithmManager.create("ConvertUnits")
        AlgorithmManager.clear()
        self.assertEqual(0, AlgorithmManager.size())

    def test_clear_after_function_exit_with_set_function_property(self):
        # This test checks that shared_ptrs are handled correctly when using setProperty with a function
        # created using function factory.
        # We deliberately:
        # - Use AlgorithmManger.create
        # - leave AlgorithmManager.clear function to be called by tearDown
        func = FunctionFactory.createFunction('Gaussian')
        alg = AlgorithmManager.create('Fit')
        alg.setProperty('Function', func)

    def test_clear_after_function_exit_with_set_function_property_using_simpleapi(self):
        # This test checks that shared_ptrs are handled correctly when using setProperty with a function
        # created using the simpleapi.
        # We deliberately:
        # - Use AlgorithmManger.create
        # - leave AlgorithmManager.clear function to be called by tearDown
        func = Gaussian()
        alg = AlgorithmManager.create('Fit')
        alg.setProperty('Function', func)

    def test_clear_after_function_exit_with_set_workspace_property(self):
        # This test checks that shared_ptrs are handled correctly when using setProperty with a workspace
        # created using workspace factory.
        # We deliberately:
        # - Use AlgorithmManger.create
        # - leave AlgorithmManager.clear function to be called by tearDown
        ws = WorkspaceFactory.create('Workspace2D', 1, 1, 1)
        alg = AlgorithmManager.create('Rebin')
        alg.setProperty('InputWorkspace', ws)

    def test_clear_after_function_exit_with_set_workspace_property_using_simpleapi(self):
        # This test checks that shared_ptrs are handled correctly when using setProperty with a workspace
        # created using the simpleapi.
        # We deliberately:
        # - Use AlgorithmManger.create
        # - leave AlgorithmManager.clear function to be called by tearDown
        ws = CreateWorkspace([0, 1, 2], [0, 1, 2])
        alg = AlgorithmManager.create('Rebin')
        alg.setProperty('InputWorkspace', ws)


if __name__ == '__main__':
    unittest.main()
