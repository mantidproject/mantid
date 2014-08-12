import unittest, os

from mantid.kernel import *
from mantid.api import *
from mantid import config

class CreateMappingFileTest(unittest.TestCase):

    def setUp(self):
        self.kwargs = {}
        self.kwargs['Filename'] = ''
        self.kwargs['GroupCount'] = 20
        self.kwargs['SpectraRange'] = '3,53'

    def tearDown(self):
        # Clean up saved map files
        path = os.path.join(config['defaultsave.directory'], self.kwargs['Filename'])
        if os.path.isfile(path):
            try:
                os.remove(path)
            except IOError, _:
                pass

    def test_basic(self):
        CreateMappingFile(**self.kwargs)
        path = os.path.join(config['defaultsave.directory'], self.kwargs['Filename'])
        self.assertTrue(os.path.isfile(path))

if __name__ == '__main__':
    unittest.main()
