import unittest
from testhelpers import assertRaisesNothing
from mantid.api import (Algorithm, DataProcessorAlgorithm, AlgorithmFactory,
                        AlgorithmManager)

class TestDataProcessor(DataProcessorAlgorithm):
    def PyInit(self):
        pass
    def PyExec(self):
        pass
# end v1 alg

class DataProcessorAlgorithmTest(unittest.TestCase):

    def test_DataProcessorAlgorithm_instance_inherits_Algorithm(self):
        alg = TestDataProcessor()
        self.assertTrue(isinstance(alg, Algorithm))

    def test_class_has_expected_attrbutes(self):
        expected_attrs = \
        [
          "setLoadAlg",
          "setLoadAlgFileProp",
          "setAccumAlg",
          "determineChunk",
          "loadChunk",
          "load",
          "splitInput",
          "forwardProperties",
          "getProcessProperties",
          "assemble",
          "saveNexus",
          "isMainThread",
          "getNThreads"
        ]
        for name in expected_attrs:
            if not hasattr(DataProcessorAlgorithm, name):
                self.fail("DataProcessorAlgorithm does not have expected attribute '%s'" % name)

    def test_inheriting_class_can_be_subscribed_and_created_through_factory(self):
        assertRaisesNothing(self, AlgorithmFactory.subscribe, TestDataProcessor)
        assertRaisesNothing(self, AlgorithmManager.createUnmanaged, "TestDataProcessor")

    def test_class_can_override_standard_algorithm_methods(self):
        class TestDataProcessor(DataProcessorAlgorithm):
            def version(self):
                return 2
            def PyInit(self):
                pass
            def PyExec(self):
                pass
        # end v2 alg

        AlgorithmFactory.subscribe(TestDataProcessor)
        assertRaisesNothing(self, AlgorithmManager.createUnmanaged, "TestDataProcessor", 2)


if __name__ == '__main__':
    unittest.main()
