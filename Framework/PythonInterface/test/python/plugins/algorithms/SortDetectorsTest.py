# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.simpleapi import *
from mantid.api import *
from testhelpers import *
from numpy import *


class SortDetectorsTest(unittest.TestCase):

    def testSortDetectors(self):
        w=WorkspaceCreationHelper.create2DWorkspaceWithFullInstrument(10,2,False,False)
        AnalysisDataService.add('w',w)
        MoveInstrumentComponent(w,DetectorID=3,X=0.,Y=0,Z=-7.,RelativePosition=0)
        x=SortDetectors(w)
        DeleteWorkspace(w)
        self.assertTrue(array_equal(x[0],array([2])))
        self.assertTrue(array_equal(x[1],array([7.])))
        self.assertTrue(array_equal(x[2],array([0, 1, 3, 4, 5, 6, 7, 8, 9])))
        self.assertTrue(x[3][0]==5.)

if __name__ == '__main__':
    unittest.main()
