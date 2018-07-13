from __future__ import (absolute_import, division, print_function)

import unittest
from testhelpers import WorkspaceCreationHelper

class ComponentInfoTest(unittest.TestCase):

    _ws = None

    def setUp(self):
        if self.__class__._ws is None:
            self.__class__._ws = WorkspaceCreationHelper.create2DWorkspaceWithFullInstrument(2, 1, False) # no monitors
            self.__class__._ws.getSpectrum(0).clearDetectorIDs()

if __name__ == '__main__':
    unittest.main()
