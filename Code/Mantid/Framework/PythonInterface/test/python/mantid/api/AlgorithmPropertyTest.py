import unittest
from mantid.api import AlgorithmProperty, IAlgorithm
from mantid.kernel import Direction

class AlgorithmPropertyTest(unittest.TestCase):

    def test_construction_with_name_produces_input_property(self):
        prop = AlgorithmProperty("TestProperty")

        self.assertEquals(Direction.Input, prop.direction)

    def test_value_method_returns_an_algorithm_type(self):
        prop = AlgorithmProperty("TestProperty")
        prop.valueAsStr = 'CreateWorkspace(OutputWorkspace="ws",DataY="1",DataX="1",NSpec=1'

        alg = prop.value
        self.assertTrue(isinstance(alg,IAlgorithm))
        self.assertEquals("CreateWorkspace",alg.name())


if __name__ == '__main__':
    unittest.main()
