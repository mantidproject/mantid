#pylint: disable=invalid-name,no-init,too-many-public-methods,too-many-arguments
from __future__ import (absolute_import, division, print_function)
import stresstesting

from mantid.api import FileFinder, MatrixWorkspace, mtd
import mantid.simpleapi as ms

import math

class LoadExedTest(stresstesting.MantidStressTest):
    def runTest(self):
        rawfile = "V15_0000016544_S000_P01.raw"
        print("Rawfilename:"+rawfile)
        ms.LoadEXED(Filename=rawfile, OutputWorkspace='test')
        self.assertTrue(mtd.doesExist('test'))


if __name__ == '__main__':
    unittest.main()
