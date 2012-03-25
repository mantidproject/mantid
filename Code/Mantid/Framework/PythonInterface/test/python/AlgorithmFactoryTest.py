import unittest

from mantid.api import AlgorithmFactory

class AlgorithmFactoryTest(unittest.TestCase):
    
    def test_get_algorithm_factory_does_not_return_None(self):
        self.assertTrue(AlgorithmFactory is not None )
        
    def test_get_registered_algs_returns_dictionary_of_known_algorithms(self):
        all_algs = AlgorithmFactory.getRegisteredAlgorithms(True)
        self.assertTrue( len(all_algs) > 0 )
        self.assertTrue( 'ConvertUnits' in all_algs )
        # 3 versions of LoadRaw
        self.assertEquals( len(all_algs['LoadRaw']), 3 )
        self.assertEquals( all_algs['LoadRaw'], [1,2,3] )

if __name__ == '__main__':
    unittest.main()
