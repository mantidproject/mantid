import unittest

from mantid.api import AlgorithmFactory

class ImportModuleTest(unittest.TestCase):

    def test_import_succeeds(self):
        import mantid
        # Check content
        attrs = dir(mantid)
        self.assertTrue('api' in attrs)
        self.assertTrue('geometry' in attrs)
        self.assertTrue('kernel' in attrs)

    def test_on_import_gui_flag_is_set_to_false_here(self):
        import mantid
        self.assertEquals(False, mantid.__gui__)

    def test_python_algorithms_are_loaded_recursively(self):
        """
        Test needs improving when the old API goes to just check that everything loads okay
        """
        all_algs = AlgorithmFactory.getRegisteredAlgorithms(True)
        self.assertTrue('SNSPowderReduction' in all_algs)
        self.assertTrue('Squares' in all_algs)

if __name__ == '__main__':
    unittest.main()
