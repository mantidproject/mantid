import unittest
from mantid.api import Algorithm, DataProcessorAlgorithm

class DataProcessorAlgorithmTest(unittest.TestCase):

    def test_DataProcessorAlgorithm_instance_inherits_Algorithm(self):
        class TestDataProcessor(DataProcessorAlgorithm):
            def PyInit(self):
                pass
            def PyExec(self):
                pass
        # end alg
        alg = TestDataProcessor()
        self.assertTrue(isinstance(alg, Algorithm))

if __name__ == '__main__':
    unittest.main()
