import unittest
import testhelpers

from mantid.api import AlgorithmFactory, PythonAlgorithm

class IsAnAlgorithm(PythonAlgorithm):
    def PyInit(self):
        pass

class NotAnAlgorithm(object):
    pass

class AlgorithmFactoryTest(unittest.TestCase):

    def test_get_algorithm_factory_does_not_return_None(self):
        self.assertTrue(AlgorithmFactory is not None )

    def test_exists_returns_correct_value_for_given_args(self):
        self.assertTrue(AlgorithmFactory.exists('ConvertUnits')) #any version
        self.assertTrue(AlgorithmFactory.exists('ConvertUnits', 1)) #any version
        self.assertTrue(not AlgorithmFactory.exists('ConvertUnits', 100)) #any version

    def test_get_registered_algs_returns_dictionary_of_known_algorithms(self):
        all_algs = AlgorithmFactory.getRegisteredAlgorithms(True)
        self.assertTrue( len(all_algs) > 0 )
        self.assertTrue( 'ConvertUnits' in all_algs )
        # 3 versions of LoadRaw
        self.assertEquals( len(all_algs['LoadRaw']), 3 )
        self.assertEquals( all_algs['LoadRaw'], [1,2,3] )

    def test_algorithm_subscription_with_valid_object_succeeds(self):
        testhelpers.assertRaisesNothing(self, AlgorithmFactory.subscribe, IsAnAlgorithm)

    def test_algorithm_registration_with_invalid_object_throws(self):
        self.assertRaises(ValueError, AlgorithmFactory.subscribe, NotAnAlgorithm)


if __name__ == '__main__':
    unittest.main()
