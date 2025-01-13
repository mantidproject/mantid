# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.api import AlgorithmProperty, FrameworkManagerImpl, IAlgorithm
from mantid.kernel import Direction


class AlgorithmPropertyTest(unittest.TestCase):
    def test_construction_with_name_produces_input_property(self):
        prop = AlgorithmProperty("TestProperty")

        self.assertEqual(Direction.Input, prop.direction)

    def test_value_method_returns_an_algorithm_type(self):
        # load plugins to register CreateWorkspace
        FrameworkManagerImpl.Instance()
        prop = AlgorithmProperty("TestProperty")
        prop.valueAsStr = '{"name": "CreateWorkspace","parameters": {"OutputWorkspace": "ws", "DataY": "1", "DataX": "1","NSpec": "1"}}'
        alg = prop.value
        self.assertTrue(isinstance(alg, IAlgorithm))
        self.assertEqual("CreateWorkspace", alg.name())


if __name__ == "__main__":
    unittest.main()
