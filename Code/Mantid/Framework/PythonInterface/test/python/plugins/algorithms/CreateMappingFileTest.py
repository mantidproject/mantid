import unittest, os

from mantid import config
from mantid.simpleapi import *

class CreateMappingFileTest(unittest.TestCase):

    def setUp(self):
        self.kwargs = {}
        self.kwargs['Filename'] = 'CreateMappingFile_Test.map'
        self.kwargs['GroupCount'] = 7
        self.kwargs['SpectraRange'] = [7, 48]

    def tearDown(self):
        # Clean up saved map files
        path = os.path.join(config['defaultsave.directory'], self.kwargs['Filename'])
        if os.path.isfile(path):
            try:
                os.remove(path)
            except IOError, _:
                pass

    def _find_file(self, filename):
        for directory in config['datasearch.directories'].split(';'):
            path = os.path.join(directory, filename)
            if os.path.exists(path):
                return path
        return None

    def test_basic(self):
        test_file = os.path.join(config['defaultsave.directory'], self.kwargs['Filename'])

        CreateMappingFile(**self.kwargs)
        self.assertTrue(os.path.isfile(test_file))

        known_good_filename = self._find_file('CreateMappingFile_Sample.map')
        self.assertTrue(known_good_filename is not None)

        import filecmp
        self.assertTrue(filecmp.cmp(known_good_filename, test_file, shallow=False))

if __name__ == '__main__':
    unittest.main()
