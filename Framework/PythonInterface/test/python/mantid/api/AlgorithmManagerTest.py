import unittest
import testhelpers
from mantid.api import (AlgorithmManager, IAlgorithm, Algorithm, AlgorithmProxy)

import sys

class AlgorithmManagerTest(unittest.TestCase):

    def test_create_default_version(self):
        alg = testhelpers.assertRaisesNothing(self, AlgorithmManager.create, "ConvertUnits")
        # Tests
        self.assertNotEqual(alg, None)
        self.assertEquals(alg.name(), "ConvertUnits")
        self.assertEquals(alg.version(), 1)
        self.assertEquals(alg.category(), "Transforms\\Units")

    def test_create_unknown_alg_throws(self):
        self.assertRaises(RuntimeError, AlgorithmManager.create,"DoesNotExist")

    def test_created_alg_isinstance_of_IAlgorithm(self):
        alg = AlgorithmManager.create("ConvertUnits")
        self.assertTrue(isinstance(alg, IAlgorithm))

    def test_managed_cppalg_isinstance_of_AlgorithmProxy(self):
        alg = AlgorithmManager.create("ConvertUnits")
        self.assertTrue(isinstance(alg, AlgorithmProxy))

    def test_unmanaged_cppalg_isinstance_of_Algorithm(self):
        alg = AlgorithmManager.createUnmanaged("ConvertUnits")
        self.assertTrue(isinstance(alg, Algorithm))

    def test_size_reports_number_of_managed_algorithms(self):
        old_size = AlgorithmManager.size()
        new_alg = AlgorithmManager.create("ConvertUnits")
        new_size = AlgorithmManager.size()
        self.assertTrue(new_size == old_size + 1)

    def test_getAlgorithm_returns_correct_instance(self):
        returned_instance = AlgorithmManager.create("ConvertUnits")
        id = returned_instance.getAlgorithmID()
        mgr_instance = AlgorithmManager.getAlgorithm(id)
        self.assertEquals(id, mgr_instance.getAlgorithmID())

    def test_removeById_removes_correct_instance(self):
        alg = AlgorithmManager.create("ConvertUnits")
        alg2 = AlgorithmManager.create("ConvertUnits")
        AlgorithmManager.removeById(alg.getAlgorithmID())
        self.assertEquals(None, AlgorithmManager.getAlgorithm(alg.getAlgorithmID()))
        self.assertNotEqual(None, AlgorithmManager.getAlgorithm(alg2.getAlgorithmID()))

    def test_newestInstanceOf_returns_correct_instance(self):
        alg = AlgorithmManager.create("ConvertUnits")
        alg2 = AlgorithmManager.create("ConvertUnits")
        alg3 = AlgorithmManager.newestInstanceOf("ConvertUnits")

        self.assertEquals(alg2.getAlgorithmID(), alg3.getAlgorithmID())
        self.assertNotEqual(alg.getAlgorithmID(), alg3.getAlgorithmID())

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
            self.assertEquals(1, len(algs))
        except:
            pause_thread.algorithm.cancel()
            pause_thread.join()
            raise
        finally:
            pause_thread.algorithm.cancel()
            pause_thread.join()


    def test_clear_removes_all_managed_algorithms(self):
        AlgorithmManager.clear()
        new_size = AlgorithmManager.size()
        self.assertEquals(0, new_size)


if __name__ == '__main__':
    unittest.main()
