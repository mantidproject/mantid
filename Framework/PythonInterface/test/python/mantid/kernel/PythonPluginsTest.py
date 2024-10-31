# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import os
import shutil
import sys

import mantid.kernel.plugins as plugins
from mantid.api import AlgorithmFactory, AlgorithmManager

__TESTALG__ = """from mantid.api import PythonAlgorithm, AlgorithmFactory

class TestPyAlg(PythonAlgorithm):

    def PyInit(self):
        pass

    def PyExec(self):
        pass

AlgorithmFactory.subscribe(TestPyAlg)
"""


class PythonPluginsTest(unittest.TestCase):
    def setUp(self):
        # Make a test directory and test plugin
        self._testdir = os.path.join(os.getcwd(), "PythonPluginsTest_TmpDir")
        try:
            os.mkdir(self._testdir)
        except OSError:
            pass  # Already exists, maybe it was not removed when a test failed?
        filename = os.path.join(self._testdir, "TestPyAlg.py")
        if not os.path.exists(filename):
            plugin = open(filename, "w")
            plugin.write(__TESTALG__)
            plugin.close()

    def tearDown(self):
        try:
            shutil.rmtree(self._testdir)
        except shutil.Error:
            pass

    def test_loading_python_algorithm_increases_registered_algs_by_one(self):
        loaded = plugins.load(self._testdir)
        self.assertGreater(len(loaded), 0)
        expected_name = "TestPyAlg"
        # Has the name appear in the module dictionary
        self.assertTrue(expected_name in sys.modules)
        # Do we have the registered algorithm
        algs = AlgorithmFactory.getRegisteredAlgorithms(True)
        self.assertTrue(expected_name in algs)
        # Can it be created?
        try:
            test_alg = AlgorithmManager.createUnmanaged(expected_name)
            self.assertEqual(expected_name, test_alg.name())
            self.assertEqual(1, test_alg.version())
        except RuntimeError:
            self.fail("Failed to create plugin algorithm from the manager: '%s' " % s)


if __name__ == "__main__":
    unittest.main()
