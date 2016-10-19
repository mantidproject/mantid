from __future__ import (absolute_import, division, print_function)
from mantid import *
import os
import unittest

import pearl_routines


class PearlRoutinesTest(unittest.TestCase):
    def xtest(self):
        raise NotImplementedError()

#if __name__ == '__main__':
#    DIRS = config['datasearch.directories'].split(';')
#    CalibDir = os.path.join(DIRS[0] + '/PEARL/Calibration_Test/Calibration/')
#    unittest.main()