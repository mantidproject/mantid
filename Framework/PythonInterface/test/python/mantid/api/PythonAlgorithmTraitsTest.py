# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Defines tests for the traits within Python algorithms
such as name, version etc.
"""

import unittest
import testhelpers

from mantid.api import PythonAlgorithm, Algorithm, IAlgorithm, AlgorithmManager, AlgorithmFactory

########################### Test classes #####################################


class TestPyAlgDefaultAttrs(PythonAlgorithm):
    def PyInit(self):
        pass

    def PyExec(self):
        pass


class TestPyAlgOverriddenAttrs(PythonAlgorithm):
    def version(self):
        return 2

    def category(self):
        return "BestAlgorithms"

    def helpURL(self):
        return "Optional documentation URL"

    def isRunning(self):
        return True

    def PyInit(self):
        pass

    def PyExec(self):
        pass


class TestPyAlgIsRunningReturnsNonBool(PythonAlgorithm):
    def isRunning(self):
        return 1

    def PyInit(self):
        pass

    def PyExec(self):
        pass


class CancellableAlg(PythonAlgorithm):
    is_running = True

    def PyInit(self):
        pass

    def PyExec(self):
        pass

    def isRunning(self):
        return self.is_running

    def cancel(self):
        self.is_running = False


###############################################################################


class PythonAlgorithmTest(unittest.TestCase):
    _registered = None

    def setUp(self):
        if self.__class__._registered is None:
            self.__class__._registered = True
            AlgorithmFactory.subscribe(TestPyAlgDefaultAttrs)
            AlgorithmFactory.subscribe(TestPyAlgOverriddenAttrs)
            AlgorithmFactory.subscribe(TestPyAlgIsRunningReturnsNonBool)
            AlgorithmFactory.subscribe(CancellableAlg)

    def test_managed_alg_is_descendent_of_IAlgorithm(self):
        alg = AlgorithmManager.create("TestPyAlgDefaultAttrs")
        self.assertTrue(isinstance(alg, IAlgorithm))

    def test_unmanaged_alg_is_descendent_of_PythonAlgorithm(self):
        alg = AlgorithmManager.createUnmanaged("TestPyAlgDefaultAttrs")
        self.assertTrue(isinstance(alg, PythonAlgorithm))
        self.assertTrue(isinstance(alg, Algorithm))
        self.assertTrue(isinstance(alg, IAlgorithm))

    def test_alg_with_default_attrs(self):
        testhelpers.assertRaisesNothing(self, AlgorithmManager.createUnmanaged, "TestPyAlgDefaultAttrs")
        alg = AlgorithmManager.createUnmanaged("TestPyAlgDefaultAttrs")
        testhelpers.assertRaisesNothing(self, alg.initialize)

        self.assertEqual(alg.name(), "TestPyAlgDefaultAttrs")
        self.assertEqual(alg.version(), 1)
        self.assertEqual(alg.category(), "PythonAlgorithms")
        self.assertEqual(alg.isRunning(), False)
        testhelpers.assertRaisesNothing(self, alg.cancel)

    def test_alg_with_overridden_attrs(self):
        testhelpers.assertRaisesNothing(self, AlgorithmManager.createUnmanaged, "TestPyAlgOverriddenAttrs")
        alg = AlgorithmManager.createUnmanaged("TestPyAlgOverriddenAttrs")
        self.assertEqual(alg.name(), "TestPyAlgOverriddenAttrs")
        self.assertEqual(alg.version(), 2)
        self.assertEqual(alg.category(), "BestAlgorithms")
        self.assertEqual(alg.helpURL(), "Optional documentation URL")

    def test_alg_can_be_cancelled(self):
        alg = AlgorithmManager.createUnmanaged("CancellableAlg")
        self.assertTrue(alg.isRunning())
        alg.cancel()
        self.assertTrue(not alg.isRunning())

    # --------------------------- Failure cases --------------------------------------------
    def test_isRunning_returning_non_bool_raises_error(self):
        alg = AlgorithmManager.createUnmanaged("TestPyAlgIsRunningReturnsNonBool")
        # boost.python automatically downcasts to the most available type
        # meaning that type(alg)=TestPyAlgIsRunningReturnsNonBool and not the interface
        # so that any method lookup doesn't go through the base class automatically.
        # Here we simulate how it would be called on C++ framework side
        base_running_attr = getattr(IAlgorithm, "isRunning")
        self.assertRaises(RuntimeError, base_running_attr, alg)


if __name__ == "__main__":
    unittest.main()
